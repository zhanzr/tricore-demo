/*
  interface.h - low-level NV3030B bus primitives (appkit-tc234 port,
  TK018F3716 240x284 module).

  The NV3030B wrapped-command protocol: every command is written as
  CS high (settle), CS low, then the 4-byte prefix 02 00 <cmd> 00;
  parameter and pixel bytes then stream into the same CS frame. The
  module's DC pin is not part of this protocol (driven low by the
  vendor / left floating) and there is no reset pin - power-cycling
  the module is the only recovery from a latched state.

  Transport: QSPI2 hardware, 8-bit frames, SPI mode 3, MSB first,
  ~25 MHz. CS is a GPIO output (P15.2), so sub-frame boundaries
  inside one CS-low transaction are invisible to the panel (SCLK
  simply pauses between FIFO-fed bursts - no edges, no bits).

    SCLK = P15.6 (QSPI2 SCLK2B), MOSI = P15.5 (QSPI2 MTSR2A)
    MISO = P15.4 (not wired on this module)
    CS   = P15.2 (GPIO software CS)
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

unsigned long LCD_HwSpiKHz(void); /* active QSPI2 baud in kHz (info page) */
void    SPI_HW_Flush(void);     /* drain the HW TX buffer (blocking)      */
void    LCD_UseHwBus(void);     /* QSPI2 init (idempotent)                */
void    LCD_BusDump(void);      /* print QSPI + clock tree state          */

#endif /* __INTERFACE_H */
