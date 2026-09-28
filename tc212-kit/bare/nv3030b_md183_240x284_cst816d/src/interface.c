/*
  interface.c - low-level NV3030B bus primitives (appkit-tc234 port,
  TK018F3716 240x284 module).

  Transport: QSPI1 hardware, 8-bit frames, SPI mode 3 (idle-high
  clock, shift on leading edge), MSB first, 40 MHz SCK.

  WRITE-ONLY BY DESIGN: the driver never reads from the panel. Most
  modules from this vendor do not bring MISO out to the connector, so
  relying on it would tie the driver to the few boards that do. The
  NV3030B wrapped-command protocol needs no readback (no status polling,
  no RAM read), so nothing here depends on MISO even though this
  particular module happens to wire it to P11.3.

  Wrapped-command framing (vendor-verbatim): WriteComm raises CS
  (settle), lowers it, then streams the 4-byte prefix 02 00 <cmd> 00;
  the frame stays open (CS low) so parameter/pixel bytes append
  contiguously. CS rises only at CS_SET()/LCD_EndData(). Because CS
  is a plain GPIO, the QSPI sub-frame boundaries inside one CS-low
  transaction are invisible to the panel (SCLK pauses between
  FIFO-fed bursts without edges - no spurious bits).

  QSPI FIFO protocol (per iLLD IfxQspi_SpiMaster): a burst is fed as
  BACON(LAST=0) + data words, with a BACON(LAST=1) entry before the
  final data word; completion is polled via the PT2 (end of frame)
  event flag.
*/

#include "interface.h"
#include "lcd.h"
#include "IfxQspi.h"
#include "IfxScuCcu.h"
#include "IfxScuWdt.h"
#include "serial.h"
#include <string.h>

/* Panel SCK. Overridable at build time via
 * make CFLAGS+=-DLCD_QSPI_BAUDRATE=20000000.0 (etc.).
 *
 * MEASURED LIMITS on this module:
 *   20 MHz - reliable (many consecutive clean loops)
 *   40 MHz - reliable (validated)  <- default
 *   50 MHz - NOT usable: transfers complete and the fill timing is correct,
 *            but detailed content is intermittently corrupted. Marginal
 *            signal integrity, not a driver bug.
 *
 * ALWAYS validate a rate with the checkerboard (stress_pattern), never with
 * solid fills: those have no information content, so bit errors are
 * invisible and the fill timing stays perfect while the data is wrong. */
#ifndef LCD_QSPI_BAUDRATE
#define LCD_QSPI_BAUDRATE 40000000.0F
#endif

/* SCLK/MOSI pad slew rate. Speed1 is the SLOWEST edge, which reduces
 * overshoot/ringing on the module's flex cable + connector and is the first
 * thing to try when a high SPI clock corrupts data. Speed4 is the fastest.
 * Override with make CFLAGS+=-DLCD_QSPI_PAD_DRIVER=IfxPort_PadDriver_cmosAutomotiveSpeed2 */
#ifndef LCD_QSPI_PAD_DRIVER
#define LCD_QSPI_PAD_DRIVER IfxPort_PadDriver_cmosAutomotiveSpeed1
#endif

/* Sizing cap for the QSPI time quantum (TC212 fMAX = 133 MHz). Only used`r`n * to size the quantum, never exceeded. */
#define LCD_QSPI_BAUDRATE_MAX 50000000.0F

/* Bring-up diagnostic: print the baud actually programmed into QSPI1.
 * The QSPI divider is computed from IfxScuCcu_getMaxFrequency(), so a
 * mismatch between that assumption and the real module clock shows up
 * here as an SCK very different from the requested rate. */
#ifndef LCD_QSPI_TRACE
#define LCD_QSPI_TRACE 1
#endif

/* Bytes of pixel data accumulated before one QSPI1 burst. */
#define SPI_TX_BUF_SIZE 512U

static Ifx_QSPI *g_qspi = &MODULE_QSPI1;

static uint32_t s_bacon;         /* channel BACON value (8-bit frames) */
static uint8_t  s_tx_buf[SPI_TX_BUF_SIZE];
static uint16_t s_tx_len;
static uint8_t  s_spi_ready;
static float32  s_baudrate = LCD_QSPI_BAUDRATE;   /* live SCK target */

void CS_SET(void)
{
    LCD_CS_SET;
}
void CS_CLR(void)
{
    LCD_CS_CLR;
}

/* (Re)program channel 0 for the requested baud. Safe to call at runtime:
 * ECON/BACON are ordinary channel registers and only take effect on the
 * next transfer, which is why the sweep can retune without a reset. */
static void spi_apply_baud(float32 baudrate)
{
    IfxQspi_chMode mode;
    IfxQspi_chConfig chConfig;

    memset(&mode, 0, sizeof(mode));
    mode.clockPolarity  = IfxQspi_ClockPolarity_idleHigh;
    mode.shiftClock     = IfxQspi_ShiftClock_shiftTransmitDataOnLeadingEdge;
    mode.dataHeading    = IfxQspi_DataHeading_msbFirst;
    mode.dataWidth      = 8U;
    mode.csActiveLevel  = Ifx_ActiveState_low;

    memset(&chConfig, 0, sizeof(chConfig));
    chConfig.baudrate   = baudrate;
    chConfig.mode       = mode;

    /* Time quantum must give fine enough resolution for the fastest rate
     * the sweep uses (see LCD_QSPI_BAUDRATE_MAX). Sizing it for the slow
     * end silently caps the achievable baud. */
    g_qspi->GLOBALCON.B.TQ = IfxQspi_calculateTimeQuantumLength(g_qspi, LCD_QSPI_BAUDRATE_MAX);

    g_qspi->ECON[0].U = IfxQspi_calculateExtendedConfigurationValue(g_qspi, 0, &chConfig);
    s_bacon           = IfxQspi_calculateBasicConfigurationValue(g_qspi, IfxQspi_ChannelId_0,
                                                                 &mode, baudrate);
    s_baudrate = baudrate;
}

/* Configure + init QSPI1 (once). */
static void spi_hw_init(void)
{
    if (s_spi_ready != 0U)
    {
        return;
    }

    /* Enable the module (ENDINIT-protected write).
     *
     * Force an explicit disable -> enable cycle. CLC.DISR is only a
     * *request* and the module acknowledges it via CLC.DISS; while the
     * module is still gated, writes to GLOBALCON/ECON are silently lost.
     * The module's state also differs between reset sources - a debugger
     * reset leaves it configured from the previous run, a power-on reset
     * starts from scratch - so doing the cycle explicitly makes this init
     * independent of how we got here. */
    uint16 password = IfxScuWdt_getCpuWatchdogPassword();

    IfxScuWdt_clearCpuEndinit(password);
    g_qspi->CLC.B.DISR = 1;                       /* request disable */
    IfxScuWdt_setCpuEndinit(password);

    {
        uint32 guard = 0;
        while (g_qspi->CLC.B.DISS == 0U)          /* wait until really gated */
        {
            if (++guard > 10000000UL) { break; }
        }
    }

    IfxScuWdt_clearCpuEndinit(password);
    IfxQspi_setEnableModuleRequest(g_qspi);       /* request run */
    IfxScuWdt_setCpuEndinit(password);

    {
        uint32 guard = 0;
        while (g_qspi->CLC.B.DISS != 0U)          /* wait until really running */
        {
            if (++guard > 10000000UL) { break; }
        }
    }

    /* GLOBALCON: master mode, time quantum sized for the top sweep rate. */
    Ifx_QSPI_GLOBALCON globalcon;
    globalcon.U          = 0;
    globalcon.B.TQ       = IfxQspi_calculateTimeQuantumLength(g_qspi, LCD_QSPI_BAUDRATE_MAX);
    globalcon.B.EXPECT   = IfxQspi_ExpectTimeout_2097152;
    globalcon.B.MS       = IfxQspi_Mode_master;
    globalcon.B.RESETS   = 0x7U;
    g_qspi->GLOBALCON.U  = globalcon.U;

    /* GLOBALCON1: no FIFO/interrupt requests; PT2 event = end of frame,
     * used to poll burst completion. */
    Ifx_QSPI_GLOBALCON1 globalcon1;
    globalcon1.U           = 0;
    globalcon1.B.PT2       = IfxQspi_PhaseTransitionEvent_endOfFrame;
    g_qspi->GLOBALCON1.U   = globalcon1.U;

    /* Start the QSPI (GLOBALCON.EN = RUN) - without this the module
     * stays paused, the FIFO never drains and the first frame hangs. */
    IfxQspi_run(g_qspi);

    /* Pins: SCLK1B = P11.6, MTSR1B = P11.9. MRST is deliberately NOT
     * initialised: this module does wire MISO to P11.3, but the driver is
     * write-only by design so it must not depend on it (most modules from
     * this vendor do not bring MISO out at all). No SLSO either - CS is the
     * GPIO P11.2, software controlled.
     * Pad slew is configurable (LCD_QSPI_PAD_DRIVER) - see the note there. */
    IfxQspi_initSclkOutPin(&IfxQspi1_SCLK_P11_6_OUT, IfxPort_OutputMode_pushPull,
                           LCD_QSPI_PAD_DRIVER);
    IfxQspi_initMtsrOutPin(&IfxQspi1_MTSR_P11_9_OUT, IfxPort_OutputMode_pushPull,
                           LCD_QSPI_PAD_DRIVER);

    spi_apply_baud(LCD_QSPI_BAUDRATE);

    IfxQspi_clearAllEventFlags(g_qspi);
    s_spi_ready = 1U;

#if LCD_QSPI_TRACE
    PRINTF("[QSPI] target=%lu kHz  real=%lu kHz  moduleClk=%lu MHz  TQ=%u\r\n",
           (unsigned long)(LCD_QSPI_BAUDRATE / 1000.0F),
           LCD_HwSpiKHz(),
           (unsigned long)(IfxScuCcu_getMaxFrequency() / 1000000.0F),
           (unsigned)globalcon.B.TQ);
#endif
}

/* Public init: Configure + init QSPI1 (idempotent). */
void LCD_UseHwBus(void)
{
    spi_hw_init();
}

/* Dump the QSPI module + clock tree state. Used by the boot diagnostic to
 * compare a cold power-on against a debugger reset: if the panel works
 * after flashing but not after re-powering, the difference is in here. */
void LCD_BusDump(void)
{
    Ifx_QSPI *q = g_qspi;

    PRINTF("[QSPI] CLC=0x%08lX GLB=0x%08lX GLB1=0x%08lX ECON0=0x%08lX STATUS=0x%08lX\r\n",
           (unsigned long)q->CLC.U, (unsigned long)q->GLOBALCON.U,
           (unsigned long)q->GLOBALCON1.U, (unsigned long)q->ECON[0].U,
           (unsigned long)q->STATUS.U);
    PRINTF("[QSPI] DISS=%u EN=%u TQ=%u fmax=%lu MHz spb=%lu MHz sri=%lu MHz\r\n",
           (unsigned)q->CLC.B.DISS,
           (unsigned)q->GLOBALCON.B.EN,
           (unsigned)q->GLOBALCON.B.TQ,
           (unsigned long)(IfxScuCcu_getMaxFrequency() / 1000000.0F),
           (unsigned long)(IfxScuCcu_getSpbFrequency() / 1000000.0F),
           (unsigned long)(IfxScuCcu_getSriFrequency() / 1000000.0F));
    PRINTF("[QSPI] real=%lu kHz  (requested %lu kHz)\r\n",
           LCD_HwSpiKHz(), (unsigned long)(s_baudrate / 1000.0F));
}

/* Active QSPI2 baud in kHz (for the info page). */
unsigned long LCD_HwSpiKHz(void)
{
    return (unsigned long)(IfxQspi_calcRealBaudrate(g_qspi, IfxQspi_ChannelId_0) / 1000.0F);
}

/* Feed the buffered 8-bit bytes through the QSPI2 FIFO (blocking until
 * the frame completes). FIFO protocol follows IfxQspi_SpiMaster:
 * BACON entry before the first byte, a BACON(LAST=1) entry before the
 * final byte, and the FIFO is 4 entries deep (BACON entries included). */
static void spi_send_stream(void)
{
    uint8_t *p         = s_tx_buf;
    uint32_t remaining = s_tx_len;
    boolean  first     = TRUE;

    /* Nothing buffered -> nothing to transmit, and waiting for the end of
     * frame below would block forever (no frame is running). This is the
     * guard for the "first QSPI2 transfer in LCD_Init hangs" failure. */
    if (remaining == 0U)
    {
        return;
    }

    while (remaining > 0U)
    {
        int32_t space = (int32_t)IFXQSPI_HWFIFO_DEPTH -
                        (int32_t)IfxQspi_getTransmitFifoLevel(g_qspi);
        if (space <= 0)
        {
            continue;   /* FIFO full - it drains while the frame runs */
        }
        if (first)
        {
            space--;    /* reserve a slot for the BACON entry */
        }
        if (remaining == (uint32_t)space)
        {
            space--;    /* reserve a slot for the final BACON(LAST=1) */
        }
        if (space <= 0)
        {
            continue;
        }

        uint32_t count = ((uint32_t)space < remaining) ? (uint32_t)space : remaining;
        remaining -= count;

        if (first)
        {
            IfxQspi_writeBasicConfigurationBeginStream(g_qspi, s_bacon);
            first = FALSE;
        }

        if (remaining == 0U)
        {
            if (count > 1U)
            {
                IfxQspi_write8(g_qspi, IfxQspi_ChannelId_0, p, count - 1U);
            }
            IfxQspi_writeBasicConfigurationEndStream(g_qspi, s_bacon);
            IfxQspi_writeTransmitFifo(g_qspi, p[count - 1U]);
        }
        else
        {
            IfxQspi_write8(g_qspi, IfxQspi_ChannelId_0, p, count);
        }
        p += count;
    }
    s_tx_len = 0U;

    /* Wait for the end of the frame (PT2 = EOF event), then clear it.
     * Bounded so a future misconfiguration shows up as a diagnostic
     * instead of a silent freeze. */
    {
        uint32_t guard = 0;
        while (!g_qspi->STATUS.B.PT2F)
        {
            if (++guard > 200000000UL)
            {
                PRINTF("[QSPI] PT2F TIMEOUT STATUS=0x%08lX FIFO=%u\r\n",
                       (unsigned long)g_qspi->STATUS.U,
                       (unsigned)IfxQspi_getTransmitFifoLevel(g_qspi));
                break;
            }
        }
    }
    g_qspi->FLAGSCLEAR.B.PT2C = 1;
}

/* Push all buffered bytes through QSPI2 (blocking). */
void SPI_HW_Flush(void)
{
    if (s_tx_len != 0U)
    {
        spi_send_stream();
    }
}

/* ---------------- byte-level transfers ------------------------------- */

/* NV3030B wrapped command: CS high (settle), CS low, then the 4-byte
 * prefix 02 00 <cmd> 00. The frame stays open for the data bytes that
 * follow (vendor-verbatim framing).
 *
 * The prefix MUST be loaded into s_tx_buf before the flush: the reference
 * (nano-f411) sends it with a direct "send now" call, but this port
 * buffers everything, so forgetting the copy leaves s_tx_len == 0 - the
 * flush then transmits nothing and blocks forever waiting for an end of
 * frame that never comes (this is exactly the "freeze on the first QSPI2
 * transfer inside LCD_Init" symptom). */
void WriteComm(uint16_t data)
{
    SPI_HW_Flush();
    CS_SET();
    for (volatile int d = 0; d < 20; d++) { }   /* CS high settle */
    CS_CLR();

    s_tx_buf[s_tx_len++] = 0x02U;
    s_tx_buf[s_tx_len++] = 0x00U;
    s_tx_buf[s_tx_len++] = (uint8_t)data;
    s_tx_buf[s_tx_len++] = 0x00U;

    spi_send_stream();
}

/* Write one data byte into the open frame. */
void WriteData(uint16_t data)
{
    s_tx_buf[s_tx_len++] = (uint8_t)data;
    if (s_tx_len >= SPI_TX_BUF_SIZE)
    {
        SPI_HW_Flush();
    }
}

/* Stream one RGB565 pixel color as two bytes into the open frame. */
void SendData(uint32_t color)
{
    s_tx_buf[s_tx_len++] = (uint8_t)(color >> 8);
    s_tx_buf[s_tx_len++] = (uint8_t)color;
    if (s_tx_len >= (SPI_TX_BUF_SIZE - 2U))
    {
        SPI_HW_Flush();
    }
}

/* Fast raw 8-bit data byte: streams into the open frame. */
void LCD_WriteDataFast(uint8_t data)
{
    s_tx_buf[s_tx_len++] = data;
    if (s_tx_len >= SPI_TX_BUF_SIZE)
    {
        SPI_HW_Flush();
    }
}

/* Solid-color bulk burst into the open frame: feed the ring in
 * buffered chunks (the QSPI FIFO protocol handles the framing). */
void LCD_FillBulk(uint32_t color, uint32_t pixels)
{
    uint8_t hi = (uint8_t)(color >> 8);
    uint8_t lo = (uint8_t)color;

    while (pixels-- != 0U)
    {
        s_tx_buf[s_tx_len++] = hi;
        s_tx_buf[s_tx_len++] = lo;
        if (s_tx_len >= SPI_TX_BUF_SIZE)
        {
            SPI_HW_Flush();
        }
    }
    SPI_HW_Flush();
}

/* Begin/end a raster burst. The caller issues WriteComm(0x2C) first;
 * BeginData just makes sure CS is low and the burst stays in one frame;
 * EndData closes it. */
void LCD_BeginData(void)
{
    SPI_HW_Flush();
    CS_CLR();
}
void LCD_EndData(void)
{
    SPI_HW_Flush();
    CS_SET();
}
