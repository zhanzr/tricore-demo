/*
  lcd.h - ST7789S 1.2" 240x240 LCD driver (appkit-tc275 port, TK012F6
  module, MD120-form-factor connector).
  Same drawing API as the other LCD projects (24-bit colors, lines,
  rectangles, circles, fills, buffer copy, ASCII text).
  Pins: SCLK=P15.6, SDA/MOSI=P15.5, CS=P14.2 (GPIO). This module is
  3-wire serial: no D/C pin - the D/C bit is clocked as the first bit of
  every 9-bit frame (same protocol as the ili9163c/ili9341v projects).
  No reset pin (the vendor sequence settles CS instead) and no backlight
  pin (hardwired on-module).
  Touch: FT6336 over I2C0 (SCL=P02.5, SDA=P02.4).
  Panel geometry: 240x240, windows at COL_Pre = 0, ROW_Pre = 0 (full
  range addressed directly), MADCTL 0x00, 16bpp.
  Vendor example: TK499_LCD_TK012F6021_soft_spi_soft_iic (module-tk012F6).
  Ported from the nucleo-u575 / nucleo-f722 projects; the module wiring
  is identical, only the bus glue (interface.c/touch.c) is board-specific.
*/

#ifndef __LCD_H
#define __LCD_H

#include <stdint.h>
#include "IfxPort.h"
#include "lcd/lcd_fonts.h"

/* ---- Panel geometry / demo timing ---- */
#define LCD_Width    240
#define LCD_Height   240
#define COL       240
#define ROW       240
#define COL_Pre   0
#define ROW_Pre   0
#define Delay_Time 500

/* ---- runtime drawing window ----
 * All drawing is relative to this window; it defaults to the full panel
 * and can be shrunk at runtime. */
void LCD_SetWindow(uint16_t x, uint16_t y, uint16_t w, uint16_t h);
void LCD_ResetWindow(void);   /* back to the full panel */
uint16_t LCD_W(void);         /* current window width  */
uint16_t LCD_H(void);         /* current window height */

/* ---- SPI control pins (TK012F6 wiring on the TC275 kit) ----
 * SCLK = P15.6, SDA/MOSI = P15.5 (owned by QSPI2 in interface.c),
 * CS = P14.2 (GPIO software CS). */

/* ---- pin accessors ---- */
#define LCD_GPIO_PortCS   (&MODULE_P14)
#define LCD_CS_Pin        2
#define LCD_CS_SET       IfxPort_setPinHigh(LCD_GPIO_PortCS, LCD_CS_Pin)
#define LCD_CS_CLR       IfxPort_setPinLow(LCD_GPIO_PortCS, LCD_CS_Pin)

/* ---- 24-bit colors (RGB888, converted to RGB565 by LCD_SetColor) ---- */
#define LCD_WHITE       0xFFFFFF
#define LCD_BLACK       0x000000
#define LCD_BLUE        0x0000FF
#define LCD_GREEN       0x00FF00
#define LCD_RED         0xFF0000
#define LCD_CYAN        0x00FFFF
#define LCD_MAGENTA     0xFF00FF
#define LCD_YELLOW      0xFFFF00

/* ---- raw RGB565 colors (vendor screens use these directly) ---- */
#define WHITE   0xFFFF
#define BLACK   0x0000
#define BLUE    0x001F
#define RED     0xF800
#define MAGENTA 0xF81F
#define GREEN   0x07E0
#define CYAN    0x7FFF
#define YELLOW  0xFFE0

#define ABS(X)  ((X) > 0 ? (X) : -(X))

/* ---- API: init / window / colors ---- */
void LCD_GPIOInit(void);
void LCD_RESET(void);
void LCD_IC_Init(void);
void LCD_Init(void);
void LCD_Reinit(void);   /* reset + re-init after a bus switch */
void LCD_SetAddress(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2);
void LCD_SetColor(uint32_t rbg888);
void LCD_SetBackColor(uint32_t rbg888);
void LCD_Clear(void);
void LCD_ClearRect(uint16_t x, uint16_t y, uint16_t width, uint16_t height);

/* ---- API: vendor screen-window helper (raw, absolute coords) ---- */
void BlockWrite(uint16_t Xstart, uint16_t Xend, uint16_t Ystart, uint16_t Yend);

/* ---- API: vendor demo screens ---- */
void DispColor(uint32_t color);
void DispFrame(void);
void DispGrayHor16(void);
void DispBand(void);
void StopDelay(uint16_t ms);

/* ---- API: ASCII text ---- */
void LCD_SetAsciiFont(pFONT *font);
void LCD_ShowTransparent(uint8_t mode);
void LCD_DisplayChar(uint16_t x, uint16_t y, uint8_t c);
void LCD_DisplayString(uint16_t x, uint16_t y, char *p);

/* ---- API: 2D drawing ---- */
void LCD_DrawPoint(uint16_t x, uint16_t y, uint32_t color);
void LCD_DrawLine_V(uint16_t x, uint16_t y, uint16_t height);
void LCD_DrawLine_H(uint16_t x, uint16_t y, uint16_t width);
void LCD_DrawLine(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2);
void LCD_DrawRect(uint16_t x, uint16_t y, uint16_t width, uint16_t height);
void LCD_DrawCircle(uint16_t x, uint16_t y, uint16_t r);
void LCD_FillRect(uint16_t x, uint16_t y, uint16_t width, uint16_t height);
void LCD_FillCircle(uint16_t x, uint16_t y, uint16_t r);

/* ---- API: raw buffer blit (RGB565 words) ---- */
void LCD_CopyBuffer(uint16_t x, uint16_t y, uint16_t width, uint16_t height,
                    uint16_t *data);

#endif /* __LCD_H */
