/*
  interface.h - low-level NV3030B bus primitives (tc212-kit port,
  TK018F3716 240x284 module).

  The NV3030B wrapped-command protocol: every command is written as
  CS high (settle), CS low, then the 4-byte prefix 02 00 <cmd> 00;
  parameter and pixel bytes then stream into the same CS frame. The
  module's DC pin is not part of this protocol (driven low by the
  vendor / left floating) and there is no reset pin - power-cycling
  the module is the only recovery from a latched state.

  Transport: QSPI1 hardware, 8-bit frames, SPI mode 3, MSB first,
  ~33 MHz. CS is a GPIO output (P11.2), so sub-frame boundaries
  inside one CS-low transaction are invisible to the panel (SCLK
  simply pauses between FIFO-fed bursts - no edges, no bits).

    SCLK = P11.6 (QSPI1 SCLK1B), MOSI = P11.9 (QSPI1 MTSR1B)`r`n    MISO = P11.3 (not used: write-only driver)`r`n    CS   = P11.2 (GPIO software CS)
*/

#ifndef __INTERFACE_H
#define __INTERFACE_H

#include <stdint.h>

void CS_SET(void);
void CS_CLR(void);
void WriteComm(uint16_t data);
void WriteData(uint16_t data);
void SendData(uint32_t color);
void LCD_WriteDataFast(uint8_t data);   /* raw byte, caller manages framing */
void LCD_BeginData(void);                /* CS low, ready for raster bytes */
void LCD_EndData(void);                  /* CS high, closes the frame */
void LCD_FillBulk(uint32_t color, uint32_t pixels); /* solid burst, open frame */

unsigned long LCD_HwSpiKHz(void); /* active QSPI1 baud in kHz (info page) */
void    SPI_HW_Flush(void);     /* drain the HW TX buffer (blocking)      */
void    LCD_UseHwBus(void);     /* QSPI1 init (idempotent)                */
void    LCD_BusDump(void);      /* print QSPI + clock tree state          */

#endif /* __INTERFACE_H */
