/*
  st7789s_md120_240x240_ft6336 main for the appkit-tc275
  (TC27xTP @ 200 MHz). TK012F6 module: ST7789S 240x240 driven over
  its 3-wire 9-bit serial protocol (no D/C pin; vendor-verbatim init
  from TK499_LCD_TK012F6021_soft_spi_soft_iic), on the HARDWARE QSPI2
  bus @ 20 MHz (packed 16-bit frames; see interface.c), plus FT6336
  capacitive touch (I2C0 P02.5/P02.4) with the touch state printed on
  the serial console.

  Behavior follows the nucleo-u575 / nucleo-f722 ports (identical
  module wiring; this port reuses their drawing/driver code verbatim
  except for the bus glue). Pattern set (per pass, live FPS counter
  throughout):
    - big-font banner page (8x16 font), then the full pattern set:
      info page (normal + inverted, with the solid-fill durations),
      TEST_STAND screens, HSV gradient sweep, LED test.
    - touch polls run during the waits; touches are printed on the
      serial port.

  Wiring: SCLK=P15.6, SDA/MOSI=P15.5, CS=P14.2. The module has no reset
  pin (the vendor settles CS instead) and no backlight pin (hardwired
  on-module). Touch: FT6336 I2C0 SCL=P02.5 SDA=P02.4.

  NOTE: power-cycle the MODULE (unplug/replug its power, not just
  NRST) before judging a fix - the panel has no reset pin, so a
  latched bad state survives MCU resets.
*/

#include <stdio.h>
#include <string.h>
#include "ASCLIN_Shell_UART.h"
#include "ticks.h"
#include "led.h"
#include "dts.h"
#include "lcd.h"
#include "interface.h"
#include "touch.h"
#include "lcd/lcd_font_1608.h"

/* Route the (STM32-style) printf calls in this port to the ASCLIN0
 * console. */
#define printf(...) PRINTF(__VA_ARGS__)

/* ---- on-board LEDs (P33.8-P33.11, low active) - via the board led.c ---- */
#define LED_ON()   do { LED1_On();  LED2_On();  LED3_On();  LED4_On();  } while (0)
#define LED_OFF()  do { LED1_Off(); LED2_Off(); LED3_Off(); LED4_Off(); } while (0)

#define SCREEN_W   LCD_Width     /* 240 (row buffer sizing) */
#define FPS_BAND   20            /* bottom rows reserved for the FPS text   */
#define BACK_COLOR LCD_BLACK
#define LED_HALF   1000          /* LED test dwell (ms)                    */
#define INFO_DY    18            /* info page line pitch (8x16 font)       */

/* --------------------------------------------------------------------- */
/* Touch printout: polls the FT6336 and prints state/X/Y on the serial
 * port (on touch-down, and on release). Reading 8 bytes from register 0
 * gives: buf[2] = TD_STATUS (1 = one touch), buf[3] = P1_XH ([3:0] X
 * high), buf[4] = P1_XL, buf[5] = P1_YH ([3:0] Y high), buf[6] = P1_YL. */
static uint8_t s_touch_down;

static void touch_task(void)
{
    uint8_t buf[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
    uint16_t x, y;

    if (Touch_Read(buf, 8) == 0U)
    {
        return;                     /* bus failure: keep the state */
    }

    if (buf[2] == 1U)
    {
        x = (uint16_t)(((buf[3] & 0x0FU) << 8) | buf[4]);
        y = (uint16_t)(((buf[5] & 0x0FU) << 8) | buf[6]);

        if (s_touch_down == 0U)
        {
            printf("[TOUCH] down X=%u Y=%u (240-Y=%u)\r\n",
                   (unsigned)x, (unsigned)y, (unsigned)(240U - y));
            s_touch_down = 1U;
        }
    }
    else if (s_touch_down != 0U)
    {
        printf("[TOUCH] release\r\n");
        s_touch_down = 0U;
    }
}

/* Runtime window geometry (follows LCD_SetWindow). */
static uint16_t anim_h(void)
{
    return (uint16_t)(LCD_H() - FPS_BAND);
}

/* --------------------------------------------------------------------- */
/* FPS counter.                                                          */
static volatile uint32_t g_frames;
static uint32_t         g_last_frames;
static uint32_t         g_fps_last_tick;
static uint32_t         g_fps_color = LCD_WHITE;   /* FPS glyph color     */

static void fps_frame(void)
{
    g_frames++;
}

static void fps_update(void)
{
    uint32_t now = get_tick_count();
    if (now - g_fps_last_tick >= 1000)
    {
        uint32_t fps = g_frames - g_last_frames;
        g_last_frames = g_frames;
        g_fps_last_tick = now;

        char buf[8];
        buf[0] = 'F'; buf[1] = 'P'; buf[2] = 'S'; buf[3] = ':';
        buf[4] = (char)('0' + (fps / 100) % 10);
        buf[5] = (char)('0' + (fps / 10) % 10);
        buf[6] = (char)('0' + fps % 10);
        buf[7] = '\0';
        LCD_SetColor(g_fps_color);
        LCD_ShowTransparent(1);              /* no opaque box */
        LCD_DisplayString(1, (uint16_t)(anim_h() + 4), buf);
        LCD_ShowTransparent(0);
    }
}

static void paint_fps_band(void)
{
    LCD_SetColor(BACK_COLOR);
    LCD_SetBackColor(BACK_COLOR);
    LCD_FillRect(0, anim_h(), LCD_W(), FPS_BAND);
}

static void delay_with_fps(uint32_t ms)
{
    uint32_t start = get_tick_count();
    do
    {
        fps_update();
        touch_task();                        /* touch printout during waits */
        delay_ticks(50);
    } while (get_tick_count() - start < ms);
}

/* --------------------------------------------------------------------- */
/* Animated gradient: hue sweeps the full color wheel over `ms`.         */
static uint32_t hsv_to_rgb(int h, int s, int v)
{
    int region = (h / 600) % 6;
    int fpart  = h % 600;
    int p = v * (255 - s) / 255;
    int q = v * (255 - (s * fpart) / 600) / 255;
    int t = v * (255 - (s * (600 - fpart)) / 600) / 255;
    int r, g, b;
    switch (region)
    {
    case 0: r = v; g = t; b = p; break;
    case 1: r = q; g = v; b = p; break;
    case 2: r = p; g = v; b = t; break;
    case 3: r = p; g = q; b = v; break;
    case 4: r = t; g = p; b = v; break;
    default:r = v; g = p; b = q; break;
    }
    return ((uint32_t)r << 16) | ((uint32_t)g << 8) | (uint32_t)b;
}

static void draw_gradient(int hue_a, int hue_b, uint16_t *row)
{
    for (int y = 0; y < anim_h(); y++)
    {
        int frac = y * 1000 / anim_h();
        int hue  = hue_a + (hue_b - hue_a) * frac / 1000;
        uint32_t c = hsv_to_rgb(hue, 255, 255);
        uint16_t rgb565 = (uint16_t)(((c >> 8) & 0xF800) | ((c >> 5) & 0x07E0) |
                                     ((c >> 3) & 0x001F));
        for (int x = 0; x < LCD_W(); x++)
        {
            row[x] = rgb565;
        }
        LCD_CopyBuffer(0, (uint16_t)y, LCD_W(), 1, row);
    }
}

static void gradient_demo(uint32_t ms)
{
    static uint16_t row[SCREEN_W];
    LCD_SetBackColor(BACK_COLOR);
    paint_fps_band();

    uint32_t start = get_tick_count();
    uint32_t t = 0;
    do
    {
        int hue_a = (int)(t * 3600 / ms);
        int hue_b = hue_a + 1800;
        if (hue_b >= 3600) { hue_b -= 3600; }
        draw_gradient(hue_a, hue_b, row);
        fps_frame();
        fps_update();
        t = get_tick_count() - start;
    } while (t < ms);
}

/* --------------------------------------------------------------------- */
/* LED test (the four on-board kit LEDs).                                */
static void led_test(void)
{
    printf("[LCD] LED ON\r\n");
    LED_ON();
    delay_with_fps(LED_HALF);
    printf("[LCD] LED OFF\r\n");
    LED_OFF();
    delay_with_fps(LED_HALF);
}

/* --------------------------------------------------------------------- */
/* Vendor TEST_STAND screens (240x240). The five solid-color fills are
 * timed (ms) - the durations belong to the current driving method only:
 * run_patterns() resets them each pass, before the fills run.          */
static uint32_t g_solid_ms[5];
static const char *const g_solid_name[5] =
{
    "RED", "GREEN", "BLUE", "WHITE", "BLACK"
};

/* kHz -> "20 MHz" / "19.5 MHz" text (shared by console + info page). */
static const char *mhz_text(unsigned long khz)
{
    static char t[16];
    if (khz % 1000UL == 0UL)
    {
        snprintf(t, sizeof t, "%lu MHz", khz / 1000UL);
    }
    else
    {
        snprintf(t, sizeof t, "%lu.%lu MHz",
                 khz / 1000UL, (khz % 1000UL) / 100UL);
    }
    return t;
}

static void TEST_STAND(void)
{
    const uint32_t solid_color[5] = { RED, GREEN, BLUE, WHITE, BLACK };

    DispFrame();
    StopDelay(Delay_Time);

    DispGrayHor16();
    StopDelay(Delay_Time);

    DispBand();
    StopDelay(Delay_Time);

    for (int i = 0; i < 5; i++)
    {
        uint32_t t0 = get_tick_count();
        DispColor(solid_color[i]);
        g_solid_ms[i] = get_tick_count() - t0;
        StopDelay(Delay_Time);
    }

    printf("[LCD] solid fills (ms): RED=%lu GREEN=%lu BLUE=%lu "
           "WHITE=%lu BLACK=%lu\r\n",
           (unsigned long)g_solid_ms[0], (unsigned long)g_solid_ms[1],
           (unsigned long)g_solid_ms[2], (unsigned long)g_solid_ms[3],
           (unsigned long)g_solid_ms[4]);
}

/* --------------------------------------------------------------------- */
/* Info page: compiler, build date, current frequency, drive method and
 * the IO map. `invert` swaps fg/bg (white background page).             */
static void info_demo(uint32_t ms, uint8_t invert)
{
    char buf[32];
    char comp[24];
    unsigned long mhz = (unsigned long)(SystemCoreClock / 1000000UL);
    uint32_t fg = invert ? LCD_BLACK : LCD_WHITE;
    uint32_t bg = invert ? LCD_WHITE : LCD_BLACK;

#if defined(__TASKING__)
    snprintf(comp, sizeof comp, "TASKING");
#elif defined(__GNUC__)
    snprintf(comp, sizeof comp, "GCC %d.%d.%d",
             __GNUC__, __GNUC_MINOR__, __GNUC_PATCHLEVEL__);
#else
    snprintf(comp, sizeof comp, "unknown");
#endif

    printf("[LCD] info%s: compiler=%s build=%s %s\r\n",
           invert ? " (inverted)" : "", comp, __DATE__, __TIME__);
    printf("[LCD] info: freq=%lu MHz drive=%s\r\n",
           mhz, "HW QSPI2");

    LCD_SetColor(fg);
    LCD_SetBackColor(bg);
    LCD_Clear();

    /* FPS band in the page background color, then restore fg/bg. */
    LCD_SetColor(bg);
    LCD_SetBackColor(bg);
    LCD_FillRect(0, anim_h(), LCD_W(), FPS_BAND);
    LCD_SetColor(fg);
    LCD_SetBackColor(bg);
    g_fps_color = fg;                     /* FPS glyph matches the page */

    /* The info page uses the big 8x16 banner font (lines must stay
     * within 29 glyphs to fit the 240 px panel width). */
    LCD_SetAsciiFont(&ASCII_Font16);

    const int ix = 2;
    int y = 2;
    snprintf(buf, sizeof buf, "%s", comp);
    LCD_DisplayString(ix, (uint16_t)y, buf);  y += INFO_DY;

    snprintf(buf, sizeof buf, "Build %s", __DATE__);
    LCD_DisplayString(ix, (uint16_t)y, buf);  y += INFO_DY;

    snprintf(buf, sizeof buf, "Freq %lu MHz", mhz);
    LCD_DisplayString(ix, (uint16_t)y, buf);  y += INFO_DY;

    snprintf(buf, sizeof buf, "HW QSPI2 %s",
             mhz_text(LCD_HwSpiKHz()));
    LCD_DisplayString(ix, (uint16_t)y, buf);  y += INFO_DY;

    snprintf(buf, sizeof buf, "SCLK=P15.6 SDA=P15.5 CS=P14.2");
    LCD_DisplayString(ix, (uint16_t)y, buf);  y += INFO_DY;

    snprintf(buf, sizeof buf, "TOUCH I2C0 P02.5/P02.4 (soft)");
    LCD_DisplayString(ix, (uint16_t)y, buf);  y += INFO_DY;

    /* Die temperature: fetch the result of the measurement triggered at
     * boot / on the previous info page, display it, then trigger the
     * next measurement for the following page. */
    {
        float die_c = read_dts_celsius();
        int t10 = (int)((die_c * 10.0F) + ((die_c >= 0.0F) ? 0.5F : -0.5F));
        int t10abs = (t10 < 0) ? -t10 : t10;

        snprintf(buf, sizeof buf, "Die %d.%d C", t10 / 10, t10abs % 10);
        LCD_DisplayString(ix, (uint16_t)y, buf);  y += INFO_DY;
        printf("[LCD] info: %s\r\n", buf);
        start_dts_measure();
    }

    /* Solid-color fill durations, measured earlier in this pass (the
     * TEST_STAND solids run before the info pages) - so they always
     * reflect the current driving method. */
    for (int i = 0; i < 5; i++)
    {
        snprintf(buf, sizeof buf, "%-5s : %4lu ms",
                 g_solid_name[i], (unsigned long)g_solid_ms[i]);
        LCD_DisplayString(ix, (uint16_t)y, buf);  y += INFO_DY;
    }

    g_fps_color = LCD_WHITE;              /* restore default FPS glyph color */

    uint32_t start = get_tick_count();
    do
    {
        fps_update();
        touch_task();                     /* touch printout during waits */
        delay_ticks(50);
    } while (get_tick_count() - start < ms);

    LCD_SetAsciiFont(&ASCII_Font12);      /* back to the normal font      */
}

/* --------------------------------------------------------------------- */
/* Big-font banner page (8x16 font), lines centered on the panel width.  */
static void banner_page(const char *l1, const char *l2,
                        uint32_t fg, uint32_t bg, uint32_t ms)
{
    g_fps_color = fg;

    LCD_SetAsciiFont(&ASCII_Font16);      /* bigger than the normal 6x12  */
    LCD_SetColor(fg);
    LCD_SetBackColor(bg);
    LCD_Clear();
    LCD_SetColor(bg);
    LCD_SetBackColor(bg);
    LCD_FillRect(0, anim_h(), LCD_W(), FPS_BAND);
    LCD_SetColor(fg);
    LCD_SetBackColor(bg);

    /* 8 px/glyph: center each line dynamically. */
    LCD_DisplayString((uint16_t)((LCD_W() - (int)strlen(l1) * 8) / 2), 40,
                      (char *)l1);
    LCD_DisplayString((uint16_t)((LCD_W() - (int)strlen(l2) * 8) / 2), 64,
                      (char *)l2);

    delay_ticks(ms);

    LCD_SetAsciiFont(&ASCII_Font12);      /* back to the normal font      */
    g_fps_color = LCD_WHITE;
}

/* --------------------------------------------------------------------- */
/* Full test-pattern set. The solid-color fills run FIRST (they are timed
 * for the current driving method), then the two info pages display the
 * measured durations.                                                   */
static void run_patterns(void)
{
    printf("[LCD] phase: TEST_STAND\r\n");
    memset(g_solid_ms, 0, sizeof g_solid_ms);   /* current method only */
    TEST_STAND();

    printf("[LCD] phase: info\r\n");
    info_demo(5000, 0);

    printf("[LCD] phase: info (inverted colors)\r\n");
    info_demo(5000, 1);

    printf("[LCD] phase: gradient\r\n");
    gradient_demo(4000);

    printf("[LCD] phase: LED test\r\n");
    led_test();
}

/* --------------------------------------------------------------------- */
void lcd_demo_main(void)
{
    printf("\r\n==== appkit-tc275 (TC27xTP) st7789s_md120_240x240_ft6336 @ %lu MHz ====\r\n",
           (unsigned long)(SystemCoreClock / 1000000UL));
    printf("ST7789S 1.2\" 240x240 (3-wire 9-bit serial, MADCTL 0x00):\r\n");
    printf("SCLK=P15.6 SDA=P15.5 CS=P14.2; no DC/RST/BL pin (in-protocol DC, "
           "backlight hardwired)\r\n");
    printf("TOUCH: FT6336 I2C0 SCL=P02.5 SDA=P02.4\r\n");

    printf("[BOOT] Touch_Init...\r\n");
    Touch_Init();
    printf("[BOOT] Touch_Init ok\r\n");
    Touch_Diag();
    printf("[BOOT] Touch_Diag ok\r\n");
    LCD_UseHwBus();       /* QSPI2 init + pins (before LCD_Init) */
    printf("[BOOT] LCD bus ok\r\n");

    /* Start the die-temperature measurement so the info pages have a
     * fresh value to display (read_dts_celsius() then waits for it). */
    start_dts_measure();
    LCD_Init();
    LCD_SetAsciiFont(&ASCII_Font12);
    paint_fps_band();

    while (1)
    {
        /* ---- HARDWARE (QSPI2 @ 20 MHz) pass ---- */
        printf("[LCD] phase: HARDWARE banner\r\n");
        LCD_Reinit();         /* re-frame the panel */
        banner_page("ST7789S", "HW QSPI2 test",
                    LCD_BLACK, LCD_CYAN, 3000);

        printf("[LCD] running patterns on HARDWARE QSPI2 @ %s\r\n",
               mhz_text(LCD_HwSpiKHz()));
        run_patterns();
    }
}
