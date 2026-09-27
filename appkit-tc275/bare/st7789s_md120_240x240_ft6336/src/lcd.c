/*
  lcd.c - ST7789S 1.2" 240x240 LCD driver (appkit-tc275 port, TK012F6
  module). 3-wire serial - the module connector has no D/C pin: every
  byte is a 9-bit frame with the D/C bit clocked first (the same
  protocol the ili9163c/ili9341v ports use); drawing API and the
  runtime-window feature follow the jd9851_md140_240x240_cst816d port.

  Wiring: SCLK=P15.6, SDA/MOSI=P15.5, CS=P14.2 (GPIO). The module has no
  reset pin (the vendor sequence settles CS instead) and no backlight pin
  (hardwired on-module).
  Touch: FT6336 over I2C0 (SCL=P02.5, SDA=P02.4) - see touch.c.
  Vendor example: TK499_LCD_TK012F6021_soft_spi_soft_iic.
  Ported from the nucleo-u575 project (identical module and drawing API;
  delays go through ticks.c instead of the HAL tick).
*/

#include <string.h>
#include "ticks.h"
#include "lcd.h"
#include "interface.h"
#include "blockwrite.h"

/* ---- runtime drawing window ----
 * Every draw is relative to this window (origin + size). It defaults to
 * the full panel and can be shrunk at runtime. */
static uint16_t s_win_x = 0, s_win_y = 0;
static uint16_t s_win_w = COL, s_win_h = ROW;

void LCD_SetWindow(uint16_t x, uint16_t y, uint16_t w, uint16_t h)
{
    if (x >= COL || y >= ROW) { return; }
    if (w == 0U || h == 0U)   { return; }
    if (x + w > COL) { w = (uint16_t)(COL - x); }
    if (y + h > ROW) { h = (uint16_t)(ROW - y); }
    s_win_x = x;
    s_win_y = y;
    s_win_w = w;
    s_win_h = h;
}

void LCD_ResetWindow(void)
{
    s_win_x = 0U;
    s_win_y = 0U;
    s_win_w = COL;
    s_win_h = ROW;
}

uint16_t LCD_W(void)
{
    return s_win_w;
}

uint16_t LCD_H(void)
{
    return s_win_h;
}


/* =====================================================================
   GPIO setup: CS = PA4 as GPIO. SCL/SDA (PA5/PA7) are owned by the bus
   selection (LCD_UseHwBus muxes them to SPI1 AF5) and must NOT be
   touched here, or the HW path loses its pins.
   ===================================================================== */
void LCD_GPIOInit(void)
{
    /* CS = P14.2 as GPIO output (SCLK/SDA, P15.6/P15.5, are owned by
     * the QSPI2 bus selection in interface.c and must NOT be touched
     * here). */
    IfxPort_setPinModeOutput(LCD_GPIO_PortCS, LCD_CS_Pin,
                             IfxPort_OutputMode_pushPull,
                             IfxPort_OutputIdx_general);

    /* Idle level: CS high (deselected). */
    LCD_CS_SET;
}

/* No reset pin on this module: the vendor sequence settles CS (high,
 * then low) and waits 100 ms before the init commands. */
void LCD_RESET(void)
{
    LCD_CS_SET;
    delay_ticks(20);
    LCD_CS_CLR;
    delay_ticks(100);
}

/* =====================================================================
   Init sequence - ST7789S (vendored TK012F6021 example, verbatim
   registers): sleep out, MADCTL 0x00, COLMOD, porch (0xB2: 0x1F/0x1F/
   0x00/0x33/0x33), gate levels (0xB7 = 0x12: VGH 12.54 V / VGL -8.23 V),
   VCOM (0xBB = 0x66), power (0xC0/0xC2/0xC3/0xC4), frame rate (0xC6 =
   0x13), gate/source drivers (0xD0 = 0xA4A1, 0xD6 = 0xA1: sleep-in gate
   to GND), positive/negative gamma (0xE0/0xE1), COLMOD 65k (0x3A =
   0x05), MADCTL 0x00, INVOFF, display on.
   ===================================================================== */
void LCD_IC_Init(void)
{
    WriteComm(0x11);              /* Sleep out */
    delay_ticks(120);

    WriteComm(0x36);              /* MADCTL */
    WriteData(0x00);

    WriteComm(0x3A);              /* COLMOD */
    WriteData(0x06);

    WriteComm(0xB2);              /* Porch control */
    WriteData(0x1F);
    WriteData(0x1F);
    WriteData(0x00);
    WriteData(0x33);
    WriteData(0x33);

    WriteComm(0xB7);              /* Gate levels: VGH 12.54 V, VGL -8.23 V */
    WriteData(0x12);

    WriteComm(0xBB);              /* VCOM */
    WriteData(0x66);

    WriteComm(0xC0);              /* VRH */
    WriteData(0x2C);

    WriteComm(0xC2);              /* VDS */
    WriteData(0x01);

    WriteComm(0xC3);              /* GVDD = 4.6 V */
    WriteData(0x15);

    WriteComm(0xC4);              /* VDV = 0 V */
    WriteData(0x20);

    WriteComm(0xC6);              /* Frame rate */
    WriteData(0x13);

    WriteComm(0xD0);              /* Gate/source drivers */
    WriteData(0xA4);
    WriteData(0xA1);

    WriteComm(0xD6);              /* Sleep-in: gate output to GND */
    WriteData(0xA1);

    WriteComm(0xE0);              /* Positive gamma */
    WriteData(0xF0);
    WriteData(0x06);
    WriteData(0x0D);
    WriteData(0x0B);
    WriteData(0x0A);
    WriteData(0x07);
    WriteData(0x2E);
    WriteData(0x43);
    WriteData(0x45);
    WriteData(0x38);
    WriteData(0x14);
    WriteData(0x13);
    WriteData(0x25);
    WriteData(0x29);

    WriteComm(0xE1);              /* Negative gamma */
    WriteData(0xF0);
    WriteData(0x07);
    WriteData(0x0A);
    WriteData(0x08);
    WriteData(0x07);
    WriteData(0x23);
    WriteData(0x2E);
    WriteData(0x33);
    WriteData(0x44);
    WriteData(0x3A);
    WriteData(0x16);
    WriteData(0x17);
    WriteData(0x26);
    WriteData(0x2C);

    WriteComm(0x3A);              /* COLMOD: 65k mode (16bpp) */
    WriteData(0x05);

    WriteComm(0x36);              /* MADCTL: top-left origin, right then
                                     down */
    WriteData(0x00);

    WriteComm(0x20);              /* INVOFF */
    WriteComm(0x29);              /* Display on */
}

void LCD_Init(void)
{
    LCD_GPIOInit();
    LCD_RESET();
    LCD_IC_Init();

    /* Clear to black. */
    DispColor(BLACK);
}

/* Re-init an already-running panel: display off, then the full vendor
 * sequence again (the sleep out inside provides the power-cycle-like
 * reset). */
void LCD_Reinit(void)
{
    WriteComm(0x28);              /* display off */
    LCD_CS_SET;
    delay_ticks(20);

    LCD_RESET();
    LCD_IC_Init();
}

/* =====================================================================
   BlockWrite - set a pixel window relative to the current drawing
   window, then leave the bus ready for the raster dump.
   ===================================================================== */
void BlockWrite(uint16_t Xstart, uint16_t Xend, uint16_t Ystart, uint16_t Yend)
{
    WriteComm(0x2A);
    WriteData((uint16_t)((Xstart + s_win_x) >> 8));
    WriteData((uint16_t)(Xstart + s_win_x));
    WriteData((uint16_t)((Xend + s_win_x) >> 8));
    WriteData((uint16_t)(Xend + s_win_x));

    WriteComm(0x2B);
    WriteData((uint16_t)((Ystart + s_win_y) >> 8));
    WriteData((uint16_t)(Ystart + s_win_y));
    WriteData((uint16_t)((Yend + s_win_y) >> 8));
    WriteData((uint16_t)(Yend + s_win_y));

    WriteComm(0x2C);
}

/* =====================================================================
   Vendor demo screens - identical geometry/ordering. The pixel loops
   stream bytes inside one BeginData/EndData burst (no per-pixel CS
   toggling; HW bursts batch the packed 9-bit stream into 256-word
   chunks).
   ===================================================================== */
void DispColor(uint32_t color)
{
    uint32_t n = (uint32_t)LCD_W() * LCD_H();

    BlockWrite(0, (uint16_t)(LCD_W() - 1U), 0, (uint16_t)(LCD_H() - 1U));
    LCD_BeginData();
    while (n--)
    {
        LCD_WriteDataFast((uint8_t)(color >> 8));
        LCD_WriteDataFast((uint8_t)color);
    }
    LCD_EndData();
}

void DispFrame(void)
{
    int i, j;
    BlockWrite(0, (uint16_t)(LCD_W() - 1U), 0, (uint16_t)(LCD_H() - 1U));
    LCD_BeginData();
    LCD_WriteDataFast(0xF8); LCD_WriteDataFast(0x00);
    for (i = 0; i < (int)LCD_W() - 2; i++) { LCD_WriteDataFast(0xFF); LCD_WriteDataFast(0xFF); }
    LCD_WriteDataFast(0x00); LCD_WriteDataFast(0x1F);
    for (j = 0; j < (int)LCD_H() - 2; j++)
    {
        LCD_WriteDataFast(0xF8); LCD_WriteDataFast(0x00);
        for (i = 0; i < (int)LCD_W() - 2; i++) { LCD_WriteDataFast(0x00); LCD_WriteDataFast(0x00); }
        LCD_WriteDataFast(0x00); LCD_WriteDataFast(0x1F);
    }
    LCD_WriteDataFast(0xF8); LCD_WriteDataFast(0x00);
    for (i = 0; i < (int)LCD_W() - 2; i++) { LCD_WriteDataFast(0xFF); LCD_WriteDataFast(0xFF); }
    LCD_WriteDataFast(0x00); LCD_WriteDataFast(0x1F);
    LCD_EndData();
}

void DispGrayHor16(void)
{
    int i, j, k;
    BlockWrite(0, (uint16_t)(LCD_W() - 1U), 0, (uint16_t)(LCD_H() - 1U));
    LCD_BeginData();
    for (i = 0; i < (int)LCD_H(); i++)
    {
        for (j = 0; j < (int)LCD_W() % 16; j++) { LCD_WriteDataFast(0); LCD_WriteDataFast(0); }
        for (j = 0; j < 16; j++)
        {
            uint16_t c = (uint16_t)(((((j * 2) << 3) | ((j * 4) >> 3)) << 8) |
                                    (((j * 4) << 5) | (j * 2)));
            for (k = 0; k < (int)LCD_W() / 16; k++)
            {
                LCD_WriteDataFast((uint8_t)(c >> 8));
                LCD_WriteDataFast((uint8_t)c);
            }
        }
    }
    LCD_EndData();
}

void DispBand(void)
{
    static const uint16_t color[8] = { 0xF800, 0xF800, 0x07E0, 0x07E0,
                                       0x001F, 0x001F, 0xFFFF, 0xFFFF };
    int i, j, k;
    BlockWrite(0, (uint16_t)(LCD_W() - 1U), 0, (uint16_t)(LCD_H() - 1U));
    LCD_BeginData();
    for (i = 0; i < 8; i++)
    {
        for (j = 0; j < (int)LCD_H() / 8; j++)
        {
            for (k = 0; k < (int)LCD_W(); k++)
            {
                LCD_WriteDataFast((uint8_t)(color[i] >> 8));
                LCD_WriteDataFast((uint8_t)color[i]);
            }
        }
    }
    for (j = 0; j < (int)LCD_H() % 8; j++)
    {
        for (k = 0; k < (int)LCD_W(); k++)
        {
            LCD_WriteDataFast((uint8_t)(color[7] >> 8));
            LCD_WriteDataFast((uint8_t)color[7]);
        }
    }
    LCD_EndData();
}

void StopDelay(uint16_t ms)
{
    delay_ticks(ms);
}

/* =====================================================================
   Colors, addressing, text and 2D drawing - the standard st7789-style
   drawing API, adapted to the 240x240 ST7789S.
   ===================================================================== */

/* Current foreground/background RGB565 + text-transparent flag. */
static uint16_t s_Color     = BLACK;
static uint16_t s_BackColor = BLACK;
static uint8_t  s_Transparent = 0;
static pFONT  *s_AsciiFont = NULL;



/* Convert a 24-bit RGB888 into a 16-bit RGB565 word. */
static uint16_t rgb888_to_rgb565(uint32_t c)
{
    uint16_t r = (uint16_t)((c & 0x00F80000UL) >> 8);
    uint16_t g = (uint16_t)((c & 0x0000FC00UL) >> 5);
    uint16_t b = (uint16_t)((c & 0x000000F8UL) >> 3);
    return (uint16_t)(r | g | b);
}

void LCD_SetColor(uint32_t rgb888)
{
    s_Color = rgb888_to_rgb565(rgb888);
}

void LCD_SetBackColor(uint32_t rgb888)
{
    s_BackColor = rgb888_to_rgb565(rgb888);
}

/* Set the pixel window using the runtime drawing window (origin + size),
 * then leave the bus ready for data. */
void LCD_SetAddress(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2)
{
    x1 = (uint16_t)(x1 + s_win_x);
    y1 = (uint16_t)(y1 + s_win_y);
    x2 = (uint16_t)(x2 + s_win_x);
    y2 = (uint16_t)(y2 + s_win_y);

    WriteComm(0x2A);
    WriteData((uint16_t)(x1 >> 8));
    WriteData((uint16_t)x1);
    WriteData((uint16_t)(x2 >> 8));
    WriteData((uint16_t)x2);

    WriteComm(0x2B);
    WriteData((uint16_t)(y1 >> 8));
    WriteData((uint16_t)y1);
    WriteData((uint16_t)(y2 >> 8));
    WriteData((uint16_t)y2);

    WriteComm(0x2C);
}

void LCD_Clear(void)
{
    uint32_t n = (uint32_t)s_win_w * s_win_h;
    LCD_SetAddress(0, 0, (uint16_t)(s_win_w - 1), (uint16_t)(s_win_h - 1));
    LCD_BeginData();
    while (n--)
    {
        LCD_WriteDataFast((uint8_t)(s_BackColor >> 8));
        LCD_WriteDataFast((uint8_t)s_BackColor);
    }
    LCD_EndData();
}

void LCD_ClearRect(uint16_t x, uint16_t y, uint16_t width, uint16_t height)
{
    uint32_t n = (uint32_t)width * height;
    if (n == 0U) { return; }
    LCD_SetAddress(x, y, (uint16_t)(x + width - 1), (uint16_t)(y + height - 1));
    LCD_BeginData();
    while (n--)
    {
        LCD_WriteDataFast((uint8_t)(s_BackColor >> 8));
        LCD_WriteDataFast((uint8_t)s_BackColor);
    }
    LCD_EndData();
}

/* ---- ASCII text ---- */
void LCD_SetAsciiFont(pFONT *font)
{
    s_AsciiFont = font;
}

void LCD_ShowTransparent(uint8_t mode)
{
    s_Transparent = mode;
}

void LCD_DisplayChar(uint16_t x, uint16_t y, uint8_t c)
{
    uint16_t index;
    uint8_t  disChar;
    /* Must hold the LARGEST font's pixels (8x16 = 128); the 6x12 font
     * only fills the first 72. */
    uint16_t Buff[8 * 16];

    if ((s_AsciiFont == NULL) || (c < 0x20U) || (c > 0x7EU))
    {
        return;
    }
    c -= 0x20U;   /* table starts at space */

    if (s_Transparent)
    {
        uint16_t bytesPerRow = s_AsciiFont->Sizes / s_AsciiFont->Height;
        for (uint16_t row = 0; row < s_AsciiFont->Height; row++)
        {
            for (uint16_t col = 0; col < s_AsciiFont->Width; col++)
            {
                disChar = s_AsciiFont->pTable[(uint16_t)c * s_AsciiFont->Sizes
                          + (uint16_t)row * bytesPerRow + (col / 8)];
                if (disChar & (uint8_t)(1U << (col % 8)))
                {
                    LCD_DrawPoint(x + col, y + row, s_Color);
                }
            }
        }
        return;
    }

    index = 0;
    /* Row-major fill matching the font layout: each glyph row is
     * bytesPerRow bytes, bit 0 = leftmost pixel (the same convention the
     * transparent path uses). Filling linearly from the raw bit stream
     * instead smears the 6x12 font: its 6-bit rows are byte-packed with 2
     * padding bits that would bleed into the next row. */
    uint16_t bytesPerRow = s_AsciiFont->Sizes / s_AsciiFont->Height;
    for (uint16_t row = 0; row < s_AsciiFont->Height; row++)
    {
        for (uint16_t col = 0; col < s_AsciiFont->Width; col++)
        {
            disChar = s_AsciiFont->pTable[(uint16_t)c * s_AsciiFont->Sizes
                      + (uint16_t)row * bytesPerRow + (col / 8)];
            Buff[index++] = (disChar & (uint8_t)(1U << (col % 8)))
                            ? s_Color : s_BackColor;
        }
    }
    LCD_CopyBuffer(x, y, s_AsciiFont->Width, s_AsciiFont->Height, Buff);
}

void LCD_DisplayString(uint16_t x, uint16_t y, char *p)
{
    while (x < COL && *p != '\0')
    {
        LCD_DisplayChar(x, y, (uint8_t)*p);
        x += (uint16_t)((s_AsciiFont != NULL) ? s_AsciiFont->Width : 6);
        p++;
    }
}

/* ---- 2D drawing ---- */
void LCD_DrawPoint(uint16_t x, uint16_t y, uint32_t color)
{
    LCD_SetAddress(x, y, x, y);
    LCD_BeginData();
    LCD_WriteDataFast((uint8_t)(color >> 8));
    LCD_WriteDataFast((uint8_t)color);
    LCD_EndData();
}

void LCD_DrawLine_V(uint16_t x, uint16_t y, uint16_t height)
{
    LCD_SetAddress(x, y, x, (uint16_t)(y + height - 1));
    LCD_BeginData();
    while (height--)
    {
        LCD_WriteDataFast((uint8_t)(s_Color >> 8));
        LCD_WriteDataFast((uint8_t)s_Color);
    }
    LCD_EndData();
}

void LCD_DrawLine_H(uint16_t x, uint16_t y, uint16_t width)
{
    LCD_SetAddress(x, y, (uint16_t)(x + width - 1), y);
    LCD_BeginData();
    while (width--)
    {
        LCD_WriteDataFast((uint8_t)(s_Color >> 8));
        LCD_WriteDataFast((uint8_t)s_Color);
    }
    LCD_EndData();
}

void LCD_DrawLine(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2)
{
    int16_t deltax = 0, deltay = 0, x = 0, y = 0, xinc1 = 0, xinc2 = 0;
    int16_t yinc1 = 0, yinc2 = 0, den = 0, num = 0, numadd = 0;
    int16_t numpixels = 0, curpixel = 0;

    deltax = ABS((int16_t)x2 - (int16_t)x1);
    deltay = ABS((int16_t)y2 - (int16_t)y1);
    x = x1; y = y1;

    if (x2 >= x1) { xinc1 = 1; xinc2 = 1; } else { xinc1 = -1; xinc2 = -1; }
    if (y2 >= y1) { yinc1 = 1; yinc2 = 1; } else { yinc1 = -1; yinc2 = -1; }

    if (deltax >= deltay)
    {
        xinc1 = 0; yinc2 = 0; den = deltax; num = deltax / 2;
        numadd = deltay; numpixels = deltax;
    }
    else
    {
        xinc2 = 0; yinc1 = 0; den = deltay; num = deltay / 2;
        numadd = deltax; numpixels = deltay;
    }
    for (curpixel = 0; curpixel <= numpixels; curpixel++)
    {
        LCD_DrawPoint((uint16_t)x, (uint16_t)y, s_Color);
        num += numadd;
        if (num >= den)
        {
            num -= den;
            x += xinc1;
            y += yinc1;
        }
        x += xinc2;
        y += yinc2;
    }
}

void LCD_DrawRect(uint16_t x, uint16_t y, uint16_t width, uint16_t height)
{
    LCD_DrawLine_H(x, y, width);
    LCD_DrawLine_H(x, (uint16_t)(y + height - 1), width);
    LCD_DrawLine_V(x, y, height);
    LCD_DrawLine_V((uint16_t)(x + width - 1), y, height);
}

void LCD_DrawCircle(uint16_t x, uint16_t y, uint16_t r)
{
    int16_t Xadd = -(int16_t)r, Yadd = 0, err = 2 - 2 * (int16_t)r, e2;
    do
    {
        LCD_DrawPoint((uint16_t)(x - Xadd), (uint16_t)(y + Yadd), s_Color);
        LCD_DrawPoint((uint16_t)(x + Xadd), (uint16_t)(y + Yadd), s_Color);
        LCD_DrawPoint((uint16_t)(x + Xadd), (uint16_t)(y - Yadd), s_Color);
        LCD_DrawPoint((uint16_t)(x - Xadd), (uint16_t)(y - Yadd), s_Color);
        e2 = err;
        if (e2 <= Yadd)
        {
            Yadd++;
            err += (int16_t)(Yadd * 2 + 1);
            if (-Xadd == Yadd && e2 <= Xadd) { e2 = 0; }
        }
        if (e2 > Xadd)
        {
            Xadd++;
            err += (int16_t)(Xadd * 2 + 1);
        }
    }
    while (Xadd <= 0);
}

void LCD_FillRect(uint16_t x, uint16_t y, uint16_t width, uint16_t height)
{
    uint32_t n = (uint32_t)width * height;
    if (n == 0U) { return; }
    LCD_SetAddress(x, y, (uint16_t)(x + width - 1), (uint16_t)(y + height - 1));
    LCD_BeginData();
    while (n--)
    {
        LCD_WriteDataFast((uint8_t)(s_Color >> 8));
        LCD_WriteDataFast((uint8_t)s_Color);
    }
    LCD_EndData();
}

void LCD_FillCircle(uint16_t x, uint16_t y, uint16_t r)
{
    int32_t  D;
    uint32_t CurX, CurY;

    D = 3 - ((int32_t)r << 1);
    CurX = 0;
    CurY = r;
    while (CurX <= CurY)
    {
        if (CurY > 0)
        {
            LCD_DrawLine_V((uint16_t)(x - CurX), (uint16_t)(y - CurY),
                           (uint16_t)(2 * CurY));
            LCD_DrawLine_V((uint16_t)(x + CurX), (uint16_t)(y - CurY),
                           (uint16_t)(2 * CurY));
        }
        if (CurX > 0)
        {
            LCD_DrawLine_V((uint16_t)(x - CurY), (uint16_t)(y - CurX),
                           (uint16_t)(2 * CurX));
            LCD_DrawLine_V((uint16_t)(x + CurY), (uint16_t)(y - CurX),
                           (uint16_t)(2 * CurX));
        }
        if (D < 0)
        {
            D += (int32_t)(CurX << 2) + 6;
        }
        else
        {
            D += (int32_t)((CurX - CurY) << 2) + 10;
            CurY--;
        }
        CurX++;
    }
    LCD_DrawCircle(x, y, r);
}

void LCD_CopyBuffer(uint16_t x, uint16_t y, uint16_t width, uint16_t height,
                    uint16_t *data)
{
    uint32_t n = (uint32_t)width * height;
    LCD_SetAddress(x, y, (uint16_t)(x + width - 1), (uint16_t)(y + height - 1));
    LCD_BeginData();
    while (n--)
    {
        LCD_WriteDataFast((uint8_t)(*data >> 8));
        LCD_WriteDataFast((uint8_t)*data);
        data++;
    }
    LCD_EndData();
}
