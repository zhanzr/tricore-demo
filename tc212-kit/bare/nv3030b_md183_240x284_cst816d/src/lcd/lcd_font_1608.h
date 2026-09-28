/*
  lcd_font_1608.h - 8x16 ASCII font (asc2_1608 grid, from the on-board LCD
  vendor example) as a second, bigger font for the banner pages.

  Characters 0x20 (' ') .. 0x7E ('~'), 95 glyphs x 16 bytes = 1520 bytes.
  Layout: one byte per row, bit 0 = leftmost pixel (LSB first) -- the same
  convention the 6x12 font and LCD_DisplayChar already use.
*/
#ifndef LCD_FONT_1608_H
#define LCD_FONT_1608_H

#include "lcd/lcd_fonts.h"

extern const uint8_t asc2_1608[1520];  /* 95 glyphs x 16 bytes          */
extern pFONT        ASCII_Font16;      /* 8 (w) x 16 (h), 16 B/glyph    */

#endif /* LCD_FONT_1608_H */