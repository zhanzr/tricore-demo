/*
  interface.h - low-level ST7789S bus primitives (appkit-tc275 port,
  TK012F6 240x240 module).

  The module connector has NO D/C pin: the panel is strapped for 3-wire
  serial, where every byte is a 9-bit frame - the D/C bit (0 = command,
  1 = data) is clocked first, then the 8 data bits, MSB first. CS frames
  each command; data bursts hold CS low across bytes.

  Drive method (HW only on this port):

    QSPI2 with **16-bit frames carrying a packed 9-bit-frame bitstream**
    (same trick as the STM32 ports - the panel only sees SCL/SDA and
    counts its own 9-bit boundaries, so the packed stream is transparent
    to it; leftover bits before CS rises are always < 9, so no extra
    frame can complete). SPI mode 3 (idle-high clock, data set while the
    clock is low, latched on the rising edge), 20 MHz default SCK via
    LCD_QSPI_BAUDRATE.

    SCLK = P15.6 (QSPI2 SCLK2B), SDA/MOSI = P15.5 (QSPI2 MTSR2A)
    CS   = P14.2 (GPIO software CS; the module's MISO P15.4 is not wired)

  The module's backlight is hardwired on-board (always on after power).
*/

#ifndef __INTERFACE_H
#define __INTERFACE_H

#include <stdint.h>

void WriteComm(uint16_t data);
void WriteData(uint16_t data);
void SendData(uint32_t color);
void LCD_WriteDataFast(uint8_t data);   /* raw byte, caller manages CS/DC */
void LCD_BeginData(void);                /* CS low, for raster bursts */
void LCD_EndData(void);                  /* CS high after a burst */

/* ---- HW QSPI2 bus ---- */
void    LCD_UseHwBus(void);     /* QSPI2 + pin init (before LCD_Init)     */
unsigned long LCD_HwSpiKHz(void); /* active QSPI2 SCK in kHz (info page)  */
void    SPI_HW_Flush(void);     /* drain the HW TX buffer (blocking)      */

#endif /* __INTERFACE_H */
