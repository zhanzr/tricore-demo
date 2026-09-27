/*
  interface.c - low-level NV3030B bus primitives (appkit-tc234 port,
  TK018F3716 240x284 module).

  Transport: QSPI2 hardware, 8-bit frames, SPI mode 3 (idle-high
  clock, shift on leading edge), MSB first, ~25 MHz SCK. The module's
  DC pin carries no framing in the NV3030B wrapped-command protocol,
  and the MISO pin is not wired on this module - write-only.

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
#include "IfxScuWdt.h"
#include "serial.h"
#include <string.h>

/* Panel SCK. Overridable at build time via
 * make CFLAGS+=-DLCD_QSPI_BAUDRATE=25000000.0 (etc.).
 * The live value is held in s_baudrate so the bring-up sweep can retune
 * the channel at runtime without rebuilding.
 *
 * 50 MHz is the fastest rate QSPI2 can generate here (fMAX = 200 MHz) and
 * the clock sweep confirmed it works: the fill time tracks the clock all
 * the way up and the panel pattern stays clean. */
#ifndef LCD_QSPI_BAUDRATE
#define LCD_QSPI_BAUDRATE 50000000.0F
#endif

/* Sweep ladder for the bring-up test: start very slow and step up to the
 * fastest rate the QSPI can generate (~50 MHz). A step "passes" when the
 * frame completes and the FPS loop keeps running; a marginal clock shows
 * up as garbage on the panel, so the operator confirms the last good one. */
#define LCD_SWEEP_STEPS   10U
static const uint32_t s_sweep_khz[LCD_SWEEP_STEPS] = {
    1000U, 2000U, 5000U, 10000U, 15000U, 20000U, 25000U, 30000U, 40000U, 50000U
};

/* Fastest rate the ladder uses. The time quantum (TQ) must be sized for
 * THIS, not for the slowest rate: TQ sets the QSPI time quanta
 * (TQspi = fMAX / (TQ + 1)) and the channel dividers are integer
 * multiples of it, so a coarse quantum cannot express high baud rates at
 * all. Sizing TQ for 1 MHz forced TQspi = 40 MHz and clamped every
 * request above 10 MHz back to 10 MHz (the sweep showed "set 50000 kHz ->
 * real 10000 kHz"). Sizing for the top rate gives TQ = 0, i.e.
 * TQspi = 200 MHz and ~5 ns resolution. */
#define LCD_QSPI_BAUDRATE_MAX 50000000.0F

/* Bring-up diagnostic: print the baud actually programmed into QSPI2.
 * The QSPI divider is computed from IfxScuCcu_getMaxFrequency(), so a
 * mismatch between that assumption and the real module clock shows up
 * here as an SCK very different from the requested rate. */
#ifndef LCD_QSPI_TRACE
#define LCD_QSPI_TRACE 1
#endif

/* Bytes of pixel data accumulated before one QSPI2 burst. */
#define SPI_TX_BUF_SIZE 512U

static Ifx_QSPI *g_qspi = &MODULE_QSPI2;

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
    SpiIf_ChMode mode;
    SpiIf_ChConfig chConfig;

    memset(&mode, 0, sizeof(mode));
    mode.clockPolarity  = SpiIf_ClockPolarity_idleHigh;
    mode.shiftClock     = SpiIf_ShiftClock_shiftTransmitDataOnLeadingEdge;
    mode.dataHeading    = SpiIf_DataHeading_msbFirst;
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

/* Retune the running QSPI2 to a new SCK (bring-up sweep). */
void LCD_HwSetBaudrate(uint32_t khz)
{
    if (khz == 0U)
    {
        return;
    }

    SPI_HW_Flush();
    spi_apply_baud((float32)khz * 1000.0F);

#if LCD_QSPI_TRACE
    PRINTF("[QSPI] set %lu kHz -> real %lu kHz\r\n",
           (unsigned long)khz, LCD_HwSpiKHz());
#endif
}

uint32_t LCD_SweepCount(void)
{
    return LCD_SWEEP_STEPS;
}

uint32_t LCD_SweepKhz(uint32_t index)
{
    return (index < LCD_SWEEP_STEPS) ? s_sweep_khz[index] : 0U;
}

/* Configure + init QSPI2 (once). */
static void spi_hw_init(void)
{
    if (s_spi_ready != 0U)
    {
        return;
    }

    /* Enable the module (ENDINIT-protected write). */
    uint16 password = IfxScuWdt_getCpuWatchdogPassword();
    IfxScuWdt_clearCpuEndinit(password);
    IfxQspi_setEnableModuleRequest(g_qspi);
    IfxScuWdt_setCpuEndinit(password);

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

    /* Pins: SCLK2B = P15.6, MTSR2A = P15.5. No MRST (module MISO not
     * wired) and no SLSO (CS is GPIO P15.2, software controlled). */
    IfxQspi_initSclkOutPin(&IfxQspi2_SCLK_P15_6_OUT, IfxPort_OutputMode_pushPull,
                           IfxPort_PadDriver_cmosAutomotiveSpeed3);
    IfxQspi_initMtsrOutPin(&IfxQspi2_MTSR_P15_5_OUT, IfxPort_OutputMode_pushPull,
                           IfxPort_PadDriver_cmosAutomotiveSpeed3);

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

/* Public init: configure + init QSPI2 (idempotent). */
void LCD_UseHwBus(void)
{
    spi_hw_init();
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
