/*
  interface.c - low-level ST7789S bus primitives (appkit-tc275 port,
  TK012F6 240x240 module).

  3-wire serial (the module connector has no D/C pin): every byte is a
  9-bit frame - the D/C bit (0 = command, 1 = data) is clocked first, then
  the 8 data bits, MSB first. The line is set while SCL is low/idle-high
  and the panel latches on the rising edge (SPI mode 3 timing).

  Drive method - QSPI2 hardware:

    QSPI2 cannot produce native 9-bit frames (DL is bit-programmable but
    the driver keeps frames byte-aligned for the packed-stream trick), so
    frames are packed into 16-bit words: the panel only observes SCL/SDA
    and counts its own 9-bit boundaries, so the packed stream is
    transparent to it. At the end of a burst the pending bits (< 9, so no
    extra frame can complete) are padded into a final word and discarded
    by the panel when CS rises. This is the same scheme the STM32 ports
    (f722 @ 54 MHz, l4r5/u575 @ 40 MHz) use.

    SCLK = P15.6 (QSPI2 SCLK2B), SDA/MOSI = P15.5 (QSPI2 MTSR2A),
    CS = P14.2 (GPIO software CS). Default 20 MHz SCK (LCD_QSPI_BAUDRATE);
    the QSPI module clock is the 200 MHz fMAX domain.

    FIFO protocol (per iLLD IfxQspi_SpiMaster): a burst is fed as
    BACON(LAST=0) + data words, and before the final data word a
    BACON(LAST=1) entry; completion is polled via the PT2 (end of frame)
    event flag.
*/

#include "interface.h"
#include "lcd.h"
#include "IfxQspi.h"
#include "IfxScuWdt.h"
#include <stdio.h>
#include <string.h>

/* Default SCK: 28.57 MHz (200 MHz fMAX / 7) - the highest stable clock
 * found for this module on this board (33.33 and 50 MHz proved
 * electrically marginal; 25 MHz is the conservative fallback). Override
 * via make CFLAGS+=-DLCD_QSPI_BAUDRATE=25000000.0 (or 33333333.0). */
#ifndef LCD_QSPI_BAUDRATE
#define LCD_QSPI_BAUDRATE 28571428.0F
#endif
#ifndef LCD_QSPI_BAUDRATE
#define LCD_QSPI_BAUDRATE 25000000.0F
#endif

/* 16-bit words of packed bitstream per flush. */
#define SPI_TX_WORDS 256U

static Ifx_QSPI *g_qspi = &MODULE_QSPI2;

static uint32_t s_bacon;         /* channel BACON value (16-bit frames) */
static uint16_t s_tx_words[SPI_TX_WORDS];
static uint16_t s_tx_len;

/* 9-bit frame accumulator: frames are appended MSB-first into a bitstream
 * that drains into 16-bit SPI words. */
static uint32_t s_acc;
static uint32_t s_nbits;

static void spi_hw_init(void)
{
    /* Enable the module (ENDINIT-protected write). */
    uint16 password = IfxScuWdt_getCpuWatchdogPassword();
    IfxScuWdt_clearCpuEndinit(password);
    IfxQspi_setEnableModuleRequest(g_qspi);
    IfxScuWdt_setCpuEndinit(password);

    /* GLOBALCON: master mode, time quantum for the target baud, timeout. */
    Ifx_QSPI_GLOBALCON globalcon;
    globalcon.U          = 0;
    globalcon.B.TQ       = IfxQspi_calculateTimeQuantumLength(g_qspi, LCD_QSPI_BAUDRATE);
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

    /* Start the module (GLOBALCON.EN = RUN) - the iLLD SpiMaster does
     * the same at the end of initModule. */
    IfxQspi_run(g_qspi);

    /* Pins: SCLK2B = P15.6, MTSR2A = P15.5. No MRST (the module's MISO
     * is not wired) and no SLSO (CS is GPIO P14.2, owned by lcd.h). */
    IfxQspi_initSclkOutPin(&IfxQspi2_SCLK_P15_6_OUT, IfxPort_OutputMode_pushPull,
                           IfxPort_PadDriver_cmosAutomotiveSpeed3);
    IfxQspi_initMtsrOutPin(&IfxQspi2_MTSR_P15_5_OUT, IfxPort_OutputMode_pushPull,
                           IfxPort_PadDriver_cmosAutomotiveSpeed3);

    /* Channel 0: SPI mode 3 (idle-high clock, shift on the leading
     * edge), MSB first, 16-bit frames, hardware CS off. */
    SpiIf_ChMode mode;
    SpiIf_ChConfig chConfig;
    memset(&mode, 0, sizeof(mode));
    mode.clockPolarity  = SpiIf_ClockPolarity_idleHigh;
    mode.shiftClock     = SpiIf_ShiftClock_shiftTransmitDataOnLeadingEdge;
    mode.dataHeading    = SpiIf_DataHeading_msbFirst;
    mode.dataWidth      = 16U;
    mode.csActiveLevel  = Ifx_ActiveState_low;

    memset(&chConfig, 0, sizeof(chConfig));
    chConfig.baudrate   = LCD_QSPI_BAUDRATE;
    chConfig.mode       = mode;

    g_qspi->ECON[0].U = IfxQspi_calculateExtendedConfigurationValue(g_qspi, 0, &chConfig);
    s_bacon           = IfxQspi_calculateBasicConfigurationValue(g_qspi, IfxQspi_ChannelId_0,
                                                                 &mode, LCD_QSPI_BAUDRATE);

    IfxQspi_clearAllEventFlags(g_qspi);
}

/* Feed the packed 16-bit words through the QSPI2 FIFO (blocking until the
 * frame completes). FIFO protocol follows IfxQspi_SpiMaster: BACON entry
 * before the first word, a BACON(LAST=1) entry before the final word, and
 * the FIFO is 4 entries deep (BACON entries included). */
static void spi_send_stream(void)
{
    uint16_t       *p         = s_tx_words;
    uint32_t        remaining = s_tx_len;
    boolean         first     = TRUE;

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
                IfxQspi_write16(g_qspi, IfxQspi_ChannelId_0, p, count - 1U);
            }
            IfxQspi_writeBasicConfigurationEndStream(g_qspi, s_bacon);
            IfxQspi_writeTransmitFifo(g_qspi, p[count - 1U]);
        }
        else
        {
            IfxQspi_write16(g_qspi, IfxQspi_ChannelId_0, p, count);
        }
        p += count;
    }
    s_tx_len = 0U;

    /* Wait for the end of the frame (PT2 = EOF event), then clear it. */
    while (!g_qspi->STATUS.B.PT2F)
    {
    }
    g_qspi->FLAGSCLEAR.B.PT2C = 1;
}

/* Push all buffered 16-bit words through QSPI2 (blocking). */
void SPI_HW_Flush(void)
{
    if (s_tx_len != 0U)
    {
        spi_send_stream();
    }
}

void LCD_UseHwBus(void)
{
    s_acc   = 0U;
    s_nbits = 0U;
    s_tx_len = 0U;

    spi_hw_init();
}

/* Active QSPI2 baud in kHz (for the info page), e.g. 20000 = 20 MHz. */
unsigned long LCD_HwSpiKHz(void)
{
    return (unsigned long)(IfxQspi_calcRealBaudrate(g_qspi, IfxQspi_ChannelId_0) / 1000.0F);
}

/* Append one 9-bit frame (dc, byte) to the packed bitstream. */
static void hw_put_frame(uint8_t dc, uint8_t byte)
{
    s_acc = (s_acc << 9U) | ((uint32_t)(dc != 0U) << 8U) | (uint32_t)byte;
    s_nbits += 9U;

    while (s_nbits >= 16U)
    {
        s_nbits -= 16U;
        s_tx_words[s_tx_len++] = (uint16_t)((s_acc >> s_nbits) & 0xFFFFU);
        if (s_tx_len >= SPI_TX_WORDS)
        {
            SPI_HW_Flush();
        }
    }
    s_acc &= (s_nbits != 0U) ? ((1U << s_nbits) - 1U) : 0U;
}

/* Pad the pending bits (< 9, cannot complete an extra frame - the panel
 * discards them when CS rises) into a final word, then transmit. */
static void hw_pad_flush(void)
{
    if (s_nbits > 0U)
    {
        s_tx_words[s_tx_len++] =
            (uint16_t)((s_acc << (16U - s_nbits)) & 0xFFFFU);
        s_nbits = 0U;
        s_acc   = 0U;
    }
    SPI_HW_Flush();
}

/* ---------------- byte-level transfers ---------------- */

/* Write a register/command: 9-bit frame with D/C = 0. */
void WriteComm(uint16_t data)
{
    SPI_HW_Flush();
    s_acc   = 0U;                    /* frame starts bit-aligned */
    s_nbits = 0U;
    LCD_CS_CLR;

    hw_put_frame(0U, (uint8_t)data);
    hw_pad_flush();
    LCD_CS_SET;
}

/* Write a data byte: 9-bit frame with D/C = 1. */
void WriteData(uint16_t data)
{
    SPI_HW_Flush();
    s_acc   = 0U;
    s_nbits = 0U;
    LCD_CS_CLR;

    hw_put_frame(1U, (uint8_t)data);
    hw_pad_flush();
    LCD_CS_SET;
}

/* One 16-bit color as two D/C=1 frames, no CS toggling (the callers -
 * the TEST_STAND fill bodies - hold CS low around whole fills). */
void SendData(uint32_t color)
{
    hw_put_frame(1U, (uint8_t)(color >> 8));
    hw_put_frame(1U, (uint8_t)color);
    SPI_HW_Flush();
}

/* Fast raw 8-bit data byte as a D/C=1 frame: caller manages CS
 * (LCD_BeginData/LCD_EndData frame the burst). */
void LCD_WriteDataFast(uint8_t data)
{
    hw_put_frame(1U, data);
}

/* Begin/end a raster burst: CS held low across the bytes. */
void LCD_BeginData(void)
{
    SPI_HW_Flush();
    s_acc   = 0U;                    /* start the burst bit-aligned */
    s_nbits = 0U;
    LCD_CS_CLR;
}
void LCD_EndData(void)
{
    hw_pad_flush();
    s_acc   = 0U;
    s_nbits = 0U;
    LCD_CS_SET;
}
