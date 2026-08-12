/**********************************************************************************************************************
 * \file ee.h
 * \brief AT25128N (128 Kbit / 16 KiB SPI EEPROM) driver over QSPI0.
 * \description Pins: CS=P33.5 (QSPI0 SLSO7), SCLK=P20.11, MOSI=P20.14 (MTSR),
 * MISO=P20.12 (MRST). 256 pages x 64 bytes, SPI mode 0.
 *********************************************************************************************************************/

#ifndef EE_H_
#define EE_H_

#include "Ifx_Types.h"

#define EE_SIZE_BYTES       16384u          /* 128 Kbit = 16 KiB */
#define EE_PAGE_SIZE        64u             /* page size in bytes */
#define EE_NUM_PAGES        (EE_SIZE_BYTES / EE_PAGE_SIZE)

/* AT25128N instructions */
#define EE_CMD_WREN         0x06u
#define EE_CMD_WRDI         0x04u
#define EE_CMD_RDSR         0x05u
#define EE_CMD_WRSR         0x01u
#define EE_CMD_READ         0x03u
#define EE_CMD_WRITE        0x02u
#define EE_SR_WIP           0x01u
#define EE_SR_WEL           0x02u

void EE_init(void);
void EE_waitReady(void);
uint8 EE_readStatus(void);
void EE_writeEnable(void);
void EE_read(uint16 addr, uint8 *data, uint32 len);
void EE_pageWrite(uint16 addr, const uint8 *data, uint32 len);
void EE_writeRaw(uint16 addr, const uint8 *data, uint32 len);   /* WREN + WRITE, no WIP wait */

#endif /* EE_H_ */
