/*
  lcd.h - NV3030B 1.83" 240x284 LCD driver (tc212-kit port, TK018F3716
  module, MD183 form factor).
  Same drawing API as the other LCD projects (24-bit colors, lines,
  rectangles, circles, fills, buffer copy, ASCII text).
  Pins: SCLK=P11.6, MOSI=P11.9, CS=P11.2 (GPIO software CS, QSPI1 SLSO
  unused). The NV3030B wrapped-command protocol carries command/data
  framing in the transaction itself - the module's DC pin is not wired
  and not needed. No MISO, no reset pin, no backlight pin on this
  module's connector (backlight is powered from the module supply).
  Touch: CST816D over bit-banged I2C (SDA=P21.4, SCL=P21.2).
  Panel geometry: 240x284, windows at COL_Pre = 0, ROW_Pre = 0 (full
  range addressed directly), MADCTL 0x08, 16bpp, IPS inversion on.
*/

#ifndef __LCD_H
#define __LCD_H

#include <stdint.h>
#include "IfxPort.h"
#include "lcd/lcd_fonts.h"

/* ---- Panel geometry / demo timing ---- */
#define LCD_Width    240
#define LCD_Height   284
#define COL       240
#define ROW       284
#define COL_Pre   0
#define ROW_Pre   0
#define Delay_Time 500

/* Power-on settle before the first panel command, in ms. The module has
 * no reset pin, so its POR/charge-pump settling rides on board power; a
 * cold power-on needs this delay or the init sequence is lost (see
 * LCD_Init in lcd.c).
 *
 * 250 ms proved MUCH too short: on a cold boot the panel only came alive
 * after ~26 s, which is exactly one demo-loop period - i.e. it was the
 * SECOND init (LCD_Reinit at the top of loop 2) that finally took. Raise
 * this until a cold boot shows the first init working; 500 ms is a
 * starting point for a slow-starting module. */
#define LCD_POWER_SETTLE_MS 500

/* ---- backlight ----
 * The module's backlight is powered from the module supply (no
 * dedicated backlight pin is wired on this connector set). */

/* ---- runtime drawing window ----
 * All drawing is relative to this window; it defaults to the full panel
 * and can be shrunk at runtime. */
void LCD_SetWindow(uint16_t x, uint16_t y, uint16_t w, uint16_t h);
void LCD_ResetWindow(void);   /* back to the full panel */
uint16_t LCD_W(void);         /* current window width  */
uint16_t LCD_H(void);         /* current window height */

/* ---- SPI control pins (TK018F3716 wiring) ----
 * SCLK = P11.6, MOSI = P11.9 (owned by QSPI1 in interface.c), CS =
 * P11.2 (GPIO software CS). Touch: SDA = P21.4, SCL = P21.2. */

/* ---- pin accessors ---- */
#define LCD_GPIO_PortCS   (&MODULE_P11)
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
#define C565_WHITE   0xFFFF
#define C565_BLACK   0x0000
#define C565_BLUE    0x001F
#define C565_RED     0xF800
#define C565_MAGENTA 0xF81F
#define C565_GREEN   0x07E0
#define C565_CYAN    0x7FFF
#define C565_YELLOW  0xFFE0

#define ABS(X)  ((X) > 0 ? (X) : -(X))

/* ---- API: init / window / colors ---- */
void LCD_GPIOInit(void);
void LCD_RESET(void);
void LCD_IC_Init(void);
void LCD_Init(void);
void LCD_Reinit(void);   /* sleep-in + re-init (no soft-reset on NV3030B) */
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
                    const uint16_t *data);

#endif /* __LCD_H */
