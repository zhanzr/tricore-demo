/**********************************************************************************************************************
 * \file ee.c
 * \brief AT25128N (128 Kbit / 16 KiB SPI EEPROM) driver over QSPI0 SPI master.
 * \description Pins: CS=P33.5 (QSPI0 SLSO7), SCLK=P20.11, MOSI=P20.14 (MTSR),
 * MISO=P20.12 (MRST). SPI mode 0, 8 MHz. 256 pages x 64 bytes.
 *********************************************************************************************************************/

#include "ee.h"
#include "IfxQspi_SpiMaster.h"
#include "IfxQspi.h"
#include "IfxCpu.h"
#include "IfxSrc.h"
#include "IfxStm.h"
#include "string.h"

/* QSPI0 service request nodes (TC22x SRC map): TX=100, RX=101, ERR=102.
 * The interrupt priority value equals the vector slot (SRN). */
#define QSPI0_SRN_TX    100u
#define QSPI0_SRN_RX    101u
#define QSPI0_SRN_ERR   102u

#define EE_BAUDRATE     2000000u
#define EE_RD_CHUNK     256u
#define EE_WRITE_TIMEOUT_US  20000u

static IfxQspi_SpiMaster g_qspi;
static IfxQspi_SpiMaster_Channel g_qspiChannel;

/* ISRs driving the interrupt-based SpiMaster exchange */
IFX_INTERRUPT(QSPI0_TX_ISR, 0, QSPI0_SRN_TX);
void QSPI0_TX_ISR(void) { IfxQspi_SpiMaster_isrTransmit(&g_qspi); }
IFX_INTERRUPT(QSPI0_RX_ISR, 0, QSPI0_SRN_RX);
void QSPI0_RX_ISR(void) { IfxQspi_SpiMaster_isrReceive(&g_qspi); }
IFX_INTERRUPT(QSPI0_ER_ISR, 0, QSPI0_SRN_ERR);
void QSPI0_ER_ISR(void) { IfxQspi_SpiMaster_isrError(&g_qspi); }

static void ee_transfer(const uint8 *tx, uint8 *rx, uint32 len)
{
    IfxQspi_SpiMaster_exchange(&g_qspiChannel, tx, rx, len);
    while (IfxQspi_SpiMaster_getStatus(&g_qspiChannel) == IfxQspi_Status_busy)
    {
    }
}

void EE_init(void)
{
    /* Module config */
    IfxQspi_SpiMaster_Config cfg;
    IfxQspi_SpiMaster_initModuleConfig(&cfg, &MODULE_QSPI0);
    cfg.mode            = IfxQspi_Mode_master;
    cfg.maximumBaudrate = (float32)EE_BAUDRATE;
    cfg.txPriority      = QSPI0_SRN_TX;
    cfg.rxPriority      = QSPI0_SRN_RX;
    cfg.erPriority      = QSPI0_SRN_ERR;
    cfg.isrProvider     = IfxCpu_Irq_getTos(IfxCpu_getCoreIndex());

    static const IfxQspi_SpiMaster_Pins pins = {
        &IfxQspi0_SCLK_P20_11_OUT, IfxPort_OutputMode_pushPull,  /* SCLK */
        &IfxQspi0_MTSR_P20_14_OUT, IfxPort_OutputMode_pushPull,  /* MOSI */
        &IfxQspi0_MRSTA_P20_12_IN, IfxPort_InputMode_pullDown,   /* MISO */
        IfxPort_PadDriver_cmosAutomotiveSpeed1
    };
    cfg.pins = &pins;
    IfxQspi_SpiMaster_initModule(&g_qspi, &cfg);

    /* Channel config: QSPI0, CS on SLSO7 = P33.5, 2 MHz, SPI mode 0
     * (CPOL=0 idleLow, CPH=0 via shiftTransmitDataOnTrailingEdge).
     * autoCS=FALSE -> driver drives CS via GPIO (software CS), which is
     * required for reliable EEPROM CS timing. */
    IfxQspi_SpiMaster_ChannelConfig ccfg;
    IfxQspi_SpiMaster_initChannelConfig(&ccfg, &g_qspi);
    ccfg.ch.baudrate = (float32)EE_BAUDRATE;
    ccfg.ch.mode.clockPolarity = IfxQspi_ClockPolarity_idleLow;
    ccfg.ch.mode.shiftClock    = IfxQspi_ShiftClock_shiftTransmitDataOnTrailingEdge;
    ccfg.ch.mode.autoCS        = FALSE;

    static const IfxQspi_SpiMaster_Output sls = {
        &IfxQspi0_SLSO7_P33_5_OUT, IfxPort_OutputMode_pushPull, IfxPort_PadDriver_cmosAutomotiveSpeed1
    };
    ccfg.sls.output = sls;
    IfxQspi_SpiMaster_initChannel(&g_qspiChannel, &ccfg);

    /* Wire the SRC interrupt service requests to vector slots 100/101/102 */
    IfxSrc_init(&SRC_QSPI_QSPI0_TX, IfxCpu_Irq_getTos(IfxCpu_getCoreIndex()), QSPI0_SRN_TX);
    IfxSrc_init(&SRC_QSPI_QSPI0_RX, IfxCpu_Irq_getTos(IfxCpu_getCoreIndex()), QSPI0_SRN_RX);
    IfxSrc_init(&SRC_QSPI_QSPI0_ERR, IfxCpu_Irq_getTos(IfxCpu_getCoreIndex()), QSPI0_SRN_ERR);
    IfxSrc_enable(&SRC_QSPI_QSPI0_TX);
    IfxSrc_enable(&SRC_QSPI_QSPI0_RX);
    IfxSrc_enable(&SRC_QSPI_QSPI0_ERR);
}

uint8 EE_readStatus(void)
{
    uint8 tx[2] = { EE_CMD_RDSR, 0x00 };
    uint8 rx[2] = { 0x00, 0x00 };
    ee_transfer(tx, rx, 2);
    return rx[1];
}

void EE_waitReady(void)
{
    uint32 t0 = IfxStm_get(&MODULE_STM0);
    uint32 timeout = EE_WRITE_TIMEOUT_US * (IfxStm_getFrequency(&MODULE_STM0) / 1000000u);

    while (((EE_readStatus() & EE_SR_WIP) != 0) &&
           (IfxStm_get(&MODULE_STM0) - t0) < timeout)
    {
    }
}

void EE_writeEnable(void)
{
    uint8 cmd[1] = { EE_CMD_WREN };
    ee_transfer(cmd, NULL_PTR, 1);
}

void EE_writeRaw(uint16 addr, const uint8 *data, uint32 len)
{
    if (len > EE_PAGE_SIZE)
    {
        len = EE_PAGE_SIZE;
    }

    EE_writeEnable();

    uint8 tx[3 + EE_PAGE_SIZE];
    tx[0] = EE_CMD_WRITE;
    tx[1] = (uint8)(addr >> 8);
    tx[2] = (uint8)(addr & 0xFF);
    if (data != NULL_PTR)
    {
        memcpy(&tx[3], data, len);
    }
    else
    {
        memset(&tx[3], 0xFF, len);
    }
    ee_transfer(tx, NULL_PTR, 3 + len);
}

void EE_pageWrite(uint16 addr, const uint8 *data, uint32 len)
{
    EE_writeRaw(addr, data, len);
    EE_waitReady();
}

void EE_read(uint16 addr, uint8 *data, uint32 len)
{
    /* Read in chunks of up to EE_RD_CHUNK bytes per exchange */
    uint8 tx[3 + EE_RD_CHUNK];
    uint8 rx[3 + EE_RD_CHUNK];

    while (len > 0)
    {
        uint32 chunk = (len > EE_RD_CHUNK) ? EE_RD_CHUNK : len;

        tx[0] = EE_CMD_READ;
        tx[1] = (uint8)(addr >> 8);
        tx[2] = (uint8)(addr & 0xFF);
        memset(&tx[3], 0xFF, chunk);

        ee_transfer(tx, rx, 3 + chunk);
        memcpy(data, &rx[3], chunk);

        addr += (uint16)chunk;
        data += chunk;
        len  -= chunk;
    }
}
