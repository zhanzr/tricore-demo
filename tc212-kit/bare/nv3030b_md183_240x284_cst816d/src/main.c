/*
  nv3030b_md183_240x284_cst816d main for the tc212-kit
  (TC212 @ 133 MHz). TK018F3716 module: NV3030B 240x284 panel
  over its wrapped-command SPI protocol on QSPI1 (hardware 8-bit
  frames, mode 3, ~33 MHz), plus CST816D capacitive touch over
  bit-banged I2C (pins auto-detected) with the touch state printed
  on the serial console.

  Demo phases per pass (live FPS counter throughout): big-font banner,
  TEST_STAND vendor screens (timed solid fills), info pages (normal +
  inverted), HSV gradient sweep, LED test. Ported from the appkit-tc234
  nv3030b port (same module); the TC212 build is HW QSPI1 only.

  Wiring: SCLK=P11.6, MOSI=P11.9, CS=P11.2 (GPIO). The module has no
  DC/reset/backlight pin in use (DC floating is fine for the wrapped
  protocol; backlight is powered from the module supply).
  Touch: CST816D I2C bus, pins auto-detected.
*/

#include <string.h>
#include <stdio.h>
#include "serial.h"
#include "Bsp.h"
#include "IfxPort.h"
#include "IfxScu_reg.h"
#include "IfxScuCcu.h"
#include "lcd.h"
#include "interface.h"
#include "touch.h"
#include "dts.h"
#include "lcd/lcd_font_1608.h"
#include "lcd/asset_test1.h"

#define SCREEN_W   LCD_Width     /* 240 (row buffer sizing) */
#define FPS_BAND   20            /* bottom rows reserved for the FPS text   */
#define BACK_COLOR LCD_BLACK
#define LED_HALF   1000          /* LED test dwell (ms)                    */

/* Info page layout. The big 8x16 font needs a 24 px line pitch (16 px
 * glyph + spacing), which limits the page to 11 lines + the FPS band on
 * a 284 px panel.
 *
 * The panel has rounded corners, so text flush against row 0 or the left
 * edge gets cropped. INFO_TOP starts the block one glyph line down, and
 * INFO_X indents far enough to clear the corner radius. */
#define INFO_DY    24            /* info page line pitch (8x16 font)       */
#define INFO_X     22            /* left indent: clears the corner radius  */
#define INFO_TOP   18            /* first text row (skip row 0)            */
#define INFO_LINES 11            /* usable lines before the FPS band       */

/* Cold-boot panel bring-up.
 *
 * How long a cold-booted panel needs before it accepts commands varies run to
 * run and cannot be measured (the module is write-only by design). Rather than
 * sleeping or counting retries, this fills the screen with a tiled asset:
 * the drawing is the wait, and it doubles as the "the panel came up" test -
 * the moment the artwork appears the panel is live. See panel_bringup_draw().
 *
 * Each cycle is one whole-screen fill plus one LCD_Reinit, so the loop
 * interleaves drawing with init attempts - an init that lands shows up as the
 * next fill actually appearing. */
#define ASSET_TILE_GAP      4    /* gap between tiles, px (keeps a border)  */

/* Draw/re-init cycles. Trades boot time for init attempts (~0.5 s per cycle,
 * mostly the datasheet delays inside LCD_Reinit). A final fill is always done
 * after the last Reinit. */
#define LCD_BRINGUP_PASSES  2u

/* ---- on-board LEDs (P02.0..P02.5, P11.10, P11.11, low active) ---- */
#define LED_COUNT       8u

static void leds_all(uint8_t on)
{
    static Ifx_P *const ports[LED_COUNT] = {
        &MODULE_P02, &MODULE_P02, &MODULE_P02, &MODULE_P02,
        &MODULE_P02, &MODULE_P02, &MODULE_P11, &MODULE_P11
    };
    static const uint8 pins[LED_COUNT] = { 0, 1, 2, 3, 4, 5, 10, 11 };
    uint8 i;

    for (i = 0; i < LED_COUNT; i++)
    {
        IfxPort_setPinModeOutput(ports[i], pins[i], IfxPort_OutputMode_pushPull,
                                 IfxPort_OutputIdx_general);
        if (on != 0u) { IfxPort_setPinLow(ports[i], pins[i]); }
        else          { IfxPort_setPinHigh(ports[i], pins[i]); }
    }
}

/* Milliseconds from the system timer.
 *
 * Bsp.h's now() returns raw STM ticks, NOT milliseconds (the nano-f411
 * original uses a HAL_GetTick() that already counts ms). Dividing by the
 * live STM frequency keeps every dwell, the FPS window and the
 * TEST_STAND fill timing in real milliseconds, and is independent of the
 * CPU/SPB clock configuration. */
static uint32_t ms_now(void)
{
    static uint32_t s_ticks_ms;
    if (s_ticks_ms == 0U)
    {
        s_ticks_ms = (uint32_t)(IfxStm_getFrequency(BSP_DEFAULT_TIMER) / 1000U);
    }
    return (uint32_t)(now() / s_ticks_ms);
}

static void delay_ms(uint32_t ms)
{
    waitTime(IfxStm_getTicksFromMilliseconds(BSP_DEFAULT_TIMER, ms));
}

/* Milliseconds since the STM was first read - i.e. roughly since boot.
 * Prefixes every phase line so a cold-boot capture shows exactly when the
 * panel starts responding (the settle fix is tuned against this). */
static uint32_t uptime_ms(void)
{
    static uint32_t s_t0;
    uint32_t t = ms_now();

    if (s_t0 == 0U)
    {
        s_t0 = t;
    }
    return t - s_t0;
}

/* --------------------------------------------------------------------- */
/* Touch printout: polls the CST816D and prints state/X/Y on the serial
 * port (on touch-down, and on release).                                 */
static uint8_t s_touch_down;

static void touch_task(void)
{
    uint8_t buf[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
    uint16_t x, y;

    Touch_Read(buf, 8);

    if (buf[3] == 0x80U && buf[4] > 1U)
    {
        x = buf[4];
        y = (uint16_t)(((buf[5] & 0x0FU) << 8) | buf[6]);

        if (s_touch_down == 0U)
        {
            PRINTF("[TOUCH] down X=%u Y=%u (284-Y=%u)\r\n",
                   (unsigned)x, (unsigned)y, (unsigned)(284U - y));
            s_touch_down = 1U;
        }
    }
    else if (s_touch_down != 0U)
    {
        PRINTF("[TOUCH] release\r\n");
        s_touch_down = 0U;
    }
}

/* Runtime window geometry (follows LCD_SetWindow). */
static uint16_t anim_h(void)
{
    return (uint16_t)(LCD_H() - FPS_BAND);
}

/* Die temperature in hundredths of a degree C (DTS), sampled on demand.
 * Matches the blink_hello project's reporting. */
static sint32 dts_celsius_x100(void)
{
    float32 t = read_dts_celsius();
    return (sint32)(t * 100.0F);
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
    uint32_t now = ms_now();
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
    uint32_t start = ms_now();
    do
    {
        fps_update();
        touch_task();                        /* touch printout during waits */
        delay_ms(50);
    } while (ms_now() - start < ms);
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

    uint32_t start = ms_now();
    uint32_t t = 0;
    do
    {
        int hue_a = (int)(t * 3600 / ms);
        int hue_b = hue_a + 1800;
        if (hue_b >= 3600) { hue_b -= 3600; }
        draw_gradient(hue_a, hue_b, row);
        fps_frame();
        fps_update();
        t = ms_now() - start;
    } while (t < ms);
}

/* --------------------------------------------------------------------- */
/* LED test (the four on-board user LEDs).                               */
static void led_test(void)
{
    PRINTF("[LCD] LED ON\r\n");
    leds_all(1u);
    delay_with_fps(LED_HALF);
    PRINTF("[LCD] LED OFF\r\n");
    leds_all(0u);
    delay_with_fps(LED_HALF);
}

/* --------------------------------------------------------------------- */
/* Vendor TEST_STAND screens. The five solid-color fills are timed (ms). */
static uint32_t g_solid_ms[5];
const char *const g_solid_name[5] =
{
    "RED", "GREEN", "BLUE", "WHITE", "BLACK"
};

/* kHz -> "25 MHz" text (shared by console + info page). */
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
    const uint32_t solid_color[5] = { C565_RED, C565_GREEN, C565_BLUE, C565_WHITE, C565_BLACK };

    DispFrame();
    StopDelay(Delay_Time);

    DispGrayHor16();
    StopDelay(Delay_Time);

    DispBand();
    StopDelay(Delay_Time);

    for (int i = 0; i < 5; i++)
    {
        uint32_t t0 = ms_now();
        DispColor(solid_color[i]);
        g_solid_ms[i] = ms_now() - t0;
        StopDelay(Delay_Time);
    }

    PRINTF("[LCD] solid fills (ms): RED=%lu GREEN=%lu BLUE=%lu "
           "WHITE=%lu BLACK=%lu\r\n",
           (unsigned long)g_solid_ms[0], (unsigned long)g_solid_ms[1],
           (unsigned long)g_solid_ms[2], (unsigned long)g_solid_ms[3],
           (unsigned long)g_solid_ms[4]);
}

/* --------------------------------------------------------------------- */
/* Info page: compiler, build date, clock rates, DTS die temperature and
 * the IO map, in the big 8x16 font (same as the banner page). `invert`
 * swaps fg/bg (white background page).
 *
 * Layout notes:
 *  - The panel has rounded corners, so the block is indented by INFO_X and
 *    starts at INFO_TOP instead of row 0; a line touching row 0 or the
 *    left edge would have its glyphs clipped.
 *  - A 24 px pitch is used instead of the 16 px glyph height so the bigger
 *    font does not run into itself. That caps the page at 11 lines plus
 *    the FPS band, so the content is trimmed to fit; the full detail stays
 *    on the serial console. */
static void info_demo(uint32_t ms, uint8_t invert)
{
    char buf[40];
    char comp[24];
    unsigned long mhz = (unsigned long)(IfxScuCcu_getCpuFrequency(IfxCpu_ResourceCpu_0) / 1000000.0F);
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

    PRINTF("[LCD] info%s: compiler=%s build=%s %s\r\n",
           invert ? " (inverted)" : "", comp, __DATE__, __TIME__);
    PRINTF("[LCD] info: freq=%lu MHz  qspi=%s  touch=%lu kHz\r\n",
           mhz, mhz_text(LCD_HwSpiKHz()), Touch_GetHz() / 1000UL);
    PRINTF("[LCD] info: die=%d.%02d C  solids(R,G,B,W,K)=%lu,%lu,%lu,%lu,%lu ms\r\n",
           (int)(dts_celsius_x100() / 100), (int)(dts_celsius_x100() % 100),
           (unsigned long)g_solid_ms[0], (unsigned long)g_solid_ms[1],
           (unsigned long)g_solid_ms[2], (unsigned long)g_solid_ms[3],
           (unsigned long)g_solid_ms[4]);

    /* Big font for the whole page (matches the banner page). */
    LCD_SetAsciiFont(&ASCII_Font16);
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

    /* 8 px per glyph in the 8x16 font. */
    const int ix = INFO_X;
    int       y  = INFO_TOP;

    snprintf(buf, sizeof buf, "%s", comp);
    LCD_DisplayString((uint16_t)ix, (uint16_t)y, buf);  y += INFO_DY;

    snprintf(buf, sizeof buf, "CPU %lu MHz", mhz);
    LCD_DisplayString((uint16_t)ix, (uint16_t)y, buf);  y += INFO_DY;

    snprintf(buf, sizeof buf, "QSPI");
    LCD_DisplayString((uint16_t)ix, (uint16_t)y, buf);
    snprintf(buf, sizeof buf, "%s", mhz_text(LCD_HwSpiKHz()));
    LCD_DisplayString((uint16_t)(ix + 5 * 8), (uint16_t)y, buf);  y += INFO_DY;

    snprintf(buf, sizeof buf, "I2C");
    LCD_DisplayString((uint16_t)ix, (uint16_t)y, buf);
    snprintf(buf, sizeof buf, "%lu kHz", Touch_GetHz() / 1000UL);
    LCD_DisplayString((uint16_t)(ix + 5 * 8), (uint16_t)y, buf);  y += INFO_DY;

    /* Die temperature, as reported by the blink_hello project. */
    {
        sint32 t100 = dts_celsius_x100();
        snprintf(buf, sizeof buf, "DIE");
        LCD_DisplayString((uint16_t)ix, (uint16_t)y, buf);
        snprintf(buf, sizeof buf, "%d.%02d C", (int)(t100 / 100), (int)(t100 % 100));
        LCD_DisplayString((uint16_t)(ix + 5 * 8), (uint16_t)y, buf);  y += INFO_DY;
    }

    /* Fill durations, measured earlier this pass so they always reflect the
     * current SPI rate. The names come from g_solid_name, so adding a colour
     * to the list cannot leave this page mislabelled. The 8x16 font allows
     * 11 chars per line at the 240 px width, hence two per line. */
    for (int i = 0; i < 5; i += 2)
    {
        if (i + 1 < 5)
        {
            snprintf(buf, sizeof buf, "%c%lu %c%lu",
                     g_solid_name[i][0], (unsigned long)g_solid_ms[i],
                     g_solid_name[i + 1][0], (unsigned long)g_solid_ms[i + 1]);
        }
        else
        {
            snprintf(buf, sizeof buf, "%c%lu",
                     g_solid_name[i][0], (unsigned long)g_solid_ms[i]);
        }
        LCD_DisplayString((uint16_t)ix, (uint16_t)y, buf);  y += INFO_DY;
    }

    g_fps_color = LCD_WHITE;              /* restore default FPS glyph color */

    uint32_t start = ms_now();
    do
    {
        fps_update();
        touch_task();                     /* touch printout during waits */
        delay_ms(50);
    } while (ms_now() - start < ms);

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

    delay_ms(ms);

    LCD_SetAsciiFont(&ASCII_Font12);      /* back to the normal font      */
    g_fps_color = LCD_WHITE;
}

/* --------------------------------------------------------------------- */
/* Full test-pattern set.                                                */
/* --------------------------------------------------------------------- */
/* Checkerboard stress pattern - the sensitive test the earlier clock sweep
 * was missing.
 *
 * The sweep scored each rate with DispBand(), whose 8 wide solid bars are
 * almost immune to bit errors (a wrong bit inside a solid bar is invisible).
 * That is why 50 MHz "passed" there yet corrupts real content: solid fills
 * never change their data, so any error is hidden, while a checkerboard
 * alternates every single pixel and exposes one wrong bit as a broken cell.
 *
 * Single pixels are drawn with LCD_CopyBuffer so the pattern also exercises
 * the address-window setup (a lost WriteComm shows up as a shifted row).
 * Any tearing, bit error or framing slip is immediately obvious. */
static void stress_pattern(void)
{
    static uint16_t row[SCREEN_W];

    LCD_SetColor(LCD_WHITE);
    LCD_SetBackColor(BACK_COLOR);
    paint_fps_band();

    for (int y = 0; y < anim_h(); y++)
    {
        for (int x = 0; x < LCD_W(); x++)
        {
            /* 1-pixel checkerboard: adjacent pixels are opposite, so every
             * data bit toggles on every pixel. */
            row[x] = ((x ^ y) & 1) ? C565_WHITE : C565_BLACK;
        }
        LCD_CopyBuffer(0, (uint16_t)y, LCD_W(), 1, row);
    }
}

/* =====================================================================
   Asset fill - the cold-boot wait, made productive.
   ===================================================================== */

/* Tile asset_test1 over the row range [y0, y1).
 *
 * As many complete tiles as fit are drawn, centred, with a gap between them
 * and a margin all round - nothing is drawn hard against an edge, so the
 * rounded corners never clip a tile and no partial tile is emitted. This is
 * the real image (RGB565, alpha composited over black when it was converted),
 * not a synthesised shape.
 *
 * 64x64 tiles at 240 px wide give 3 columns; the drawing is therefore short
 * (tens of ms). The wait is dominated by the LCD_Reinit() calls in
 * panel_bringup_draw(), which each carry the datasheet's own delays. */
static void asset_fill_range(uint16_t y0, uint16_t y1)
{
    const int tw  = ASSET_TEST1_W;
    const int th  = ASSET_TEST1_H;
    const int gap = ASSET_TILE_GAP;
    int span_h = (int)y1 - (int)y0;
    int cols, rows, used_w, used_h, x_off, y_off, r, c;

    if ((span_h < th) || ((int)LCD_W() < tw))
    {
        return;                          /* no room for even one tile */
    }

    cols = (LCD_W() - gap) / (tw + gap);
    rows = (span_h   - gap) / (th + gap);
    if ((cols <= 0) || (rows <= 0))
    {
        return;
    }

    used_w = cols * tw + (cols - 1) * gap;
    used_h = rows * th + (rows - 1) * gap;
    x_off  = ((int)LCD_W() - used_w) / 2;
    y_off  = (int)y0 + (span_h - used_h) / 2;

    for (r = 0; r < rows; r++)
    {
        for (c = 0; c < cols; c++)
        {
            LCD_CopyBuffer((uint16_t)(x_off + c * (tw + gap)),
                           (uint16_t)(y_off + r * (th + gap)),
                           (uint16_t)tw, (uint16_t)th, asset_test1);
        }
    }
}

/* Bring the panel up by drawing.
 *
 * The drawing replaces the old sleep-and-retry wait: the time is spent
 * sending real pixels, and the artwork appearing is itself the "the panel is
 * up" signal. The LCD_Reinit() calls are what actually spend most of the
 * wall-clock time, and each one is another chance for a slow-starting panel
 * to catch an init - which is why the structure interleaves draws and
 * reinits: draw -> Reinit -> draw -> Reinit -> ... -> final draw.
 *
 * The final draw is deliberate: it happens after the last Reinit, so the
 * image that stays on screen was sent with the most recent init in effect.
 *
 * A whole-screen fill is used rather than the upper/lower split sketched
 * originally, because 64x64 tiles only fit ONE row in a 114 px half - the
 * split would leave most of the panel empty. Every tile is still whole and
 * inset, so nothing is clipped by the rounded corners. */
static void panel_bringup_draw(void)
{
    uint32_t pass;

    LCD_Clear();

    for (pass = 0; pass < LCD_BRINGUP_PASSES; pass++)
    {
        PRINTF("[%lums] bring-up %lu/%lu: draw\r\n",
               (unsigned long)uptime_ms(),
               (unsigned long)(pass + 1), (unsigned long)LCD_BRINGUP_PASSES);
        asset_fill_range(INFO_TOP, anim_h());

        PRINTF("[%lums] bring-up: Reinit\r\n", (unsigned long)uptime_ms());
        LCD_Reinit();
    }

    /* Last draw with the most recent init in effect. */
    asset_fill_range(INFO_TOP, anim_h());
    PRINTF("[%lums] bring-up: done\r\n", (unsigned long)uptime_ms());
}

static void run_patterns(void)
{
    /* Sensitive pattern FIRST, right after LCD_Init(): this is what
     * actually proves the panel initialised and the SPI rate is usable.
     * Clean checkerboard = the init landed; malformed = it did not (or the
     * link is marginal). Running it first also shortens the wait before
     * the first meaningful thing appears on a cold boot. */
    PRINTF("[%lums] phase: STRESS (checkerboard @ %s)\r\n",
           (unsigned long)uptime_ms(), mhz_text(LCD_HwSpiKHz()));
    stress_pattern();
    delay_with_fps(3000);

    PRINTF("[%lums] phase: TEST_STAND\r\n", (unsigned long)uptime_ms());
    memset(g_solid_ms, 0, sizeof g_solid_ms);   /* current method only */
    TEST_STAND();

    PRINTF("[%lums] phase: info\r\n", (unsigned long)uptime_ms());
    info_demo(5000, 0);

    PRINTF("[%lums] phase: info (inverted colors)\r\n", (unsigned long)uptime_ms());
    info_demo(5000, 1);

    PRINTF("[%lums] phase: gradient\r\n", (unsigned long)uptime_ms());
    gradient_demo(4000);

    PRINTF("[%lums] phase: LED test\r\n", (unsigned long)uptime_ms());
    led_test();
}

/* --------------------------------------------------------------------- */
/* Boot diagnostic: report which reset source we came from and the clock
 * tree state, then dump the QSPI module.
 *
 * This exists because the display is known to work after a debugger reset
 * (flashing) but fail after a power cycle. SCU_RSTSTAT tells the two apart
 * (PORST = power-on, CB1 = debug reset) so the cold-boot case can be
 * captured with the same firmware - without it, every capture is a warm
 * boot and the failing case is never observed. */
static void boot_report(void)
{
    uint32 rst = SCU_RSTSTAT.U;

    PRINTF("[BOOT] RSTSTAT=0x%08lX  PORST=%u ESR0=%u ESR1=%u SW=%u SMU=%u "
           "CB0=%u CB1=%u CB3=%u EVR13=%u\r\n",
           (unsigned long)rst,
           (unsigned)((rst >> 16) & 1U),      /* PORST: power-on reset      */
           (unsigned)((rst >> 0) & 1U),       /* ESR0                       */
           (unsigned)((rst >> 1) & 1U),       /* ESR1                       */
           (unsigned)((rst >> 4) & 1U),       /* SW                         */
           (unsigned)((rst >> 3) & 1U),       /* SMU                        */
           (unsigned)((rst >> 18) & 1U),      /* CB0                        */
           (unsigned)((rst >> 19) & 1U),      /* CB1                        */
           (unsigned)((rst >> 20) & 1U),      /* CB3                        */
           (unsigned)((rst >> 23) & 1U));     /* EVR13                      */

    PRINTF("[BOOT] %s\r\n",
           (((rst >> 16) & 1U) != 0U) ? "COLD BOOT (power-on reset)"
                                      : "warm boot (debug/app reset)");
    PRINTF("[BOOT] HWCFG=0x%02X MODE=%u    cpu=%lu spb=%lu sri=%lu fmax=%lu MHz\r\n",
           (unsigned)SCU_STSTAT.B.HWCFG,
           (unsigned)SCU_STSTAT.B.MODE,
           (unsigned long)(IfxScuCcu_getCpuFrequency(IfxCpu_ResourceCpu_0) / 1000000.0F),
           (unsigned long)(IfxScuCcu_getSpbFrequency() / 1000000.0F),
           (unsigned long)(IfxScuCcu_getSriFrequency() / 1000000.0F),
           (unsigned long)(IfxScuCcu_getMaxFrequency() / 1000000.0F));
}

/* --------------------------------------------------------------------- */
void lcd_demo_main(void)
{
    PRINTF("\r\n==== tc212-kit (TC212) nv3030b_md183_240x284_cst816d @ %lu MHz ====\r\n",
           (unsigned long)(IfxScuCcu_getCpuFrequency(IfxCpu_ResourceCpu_0) / 1000000.0F));
    PRINTF("NV3030B 1.83\" 240x284 (wrapped-command SPI, MADCTL 0x08):\r\n");
    PRINTF("SCLK=P11.6 MOSI=P11.9 CS=P11.2; no DC/MISO/RST/BL pin\r\n");
    PRINTF("TOUCH: CST816D I2C on SDA=P23.1 SCL=P20.13 (X700-8 / X700-6)\r\n");

    boot_report();

    /* Bring the bus up, then probe; only hunt if the default pair fails.
     *
     * The working pair is P23.1 (SDA) / P20.13 (SCL), verified on hardware.
     * An earlier revision hunted same-port pairs only and so could never
     * have found it: the two pins are on DIFFERENT ports (P23, P20).
     *
     * If the default pair does fail, the hunt requires a CLEAN IDLE BUS
     * (both lines high) before it trusts any ACK. That guard is essential:
     * the first version trusted a plain ACK and reported a device on
     * P02.0/P02.1 - but those are LED pins, and an LED net holding SDA low
     * makes every address look like an ACK (the scan reported 119
     * "devices"). */
    Touch_Init();
    Touch_Scan();

    if (Touch_SelfTest() == 0u)
    {
        Touch_HuntPins();
    }

    /* Bring-up diagnostic: does the CST816D answer on I2C at all? */
    {
        static uint8_t st_buf[8];
        uint8_t ack = Touch_SelfTest();
        uint8_t i;

        PRINTF("[TOUCH] self-test: %s\r\n",
               (ack != 0U) ? "ACK (chip present)" : "NO ACK (check wiring/addr)");

        Touch_Read(st_buf, 8);
        PRINTF("[TOUCH] id regs:");
        for (i = 0; i < 8U; i++)
        {
            PRINTF(" %02X", st_buf[i]);
        }
        PRINTF("\r\n");
    }
    LCD_UseHwBus();       /* QSPI1 init + pins (before LCD_Init) */
    LCD_BusDump();        /* QSPI state, for cold-vs-warm comparison */
    LCD_Init();
    LCD_SetAsciiFont(&ASCII_Font12);
    paint_fps_band();

    /* Panel bring-up by drawing.
     *
     * The old approach repeated the init on a fixed schedule and slept in
     * between, which wasted the wait and blinked. This instead spends the
     * time drawing a tiled icon, so:
     *   - the wait is the drawing itself (no idle delay);
     *   - progress is visible, and the icon appearing IS the "panel is up"
     *     signal - it is also the test, because the icon is drawn with the
     *     same path as everything else;
     *   - the icon is simple and blocky, so a marginal clock shows as a
     *     slightly noisy face rather than a broken image;
     *   - the second half is drawn after LCD_Reinit(), so both init paths
     *     get a chance, matching how the demo itself re-inits each loop.
     *
     * A border is left undrawn around the icon field, and the range stays
     * inside INFO_TOP..anim_h(), to keep clear of the rounded corners and the
     * FPS band. */
    panel_bringup_draw();

    while (1)
    {
        PRINTF("[%lums] phase: banner (Reinit)\r\n", (unsigned long)uptime_ms());
        LCD_Reinit();         /* re-frame the panel */
        banner_page("NV3030B", "HW QSPI1 test",
                    LCD_BLACK, LCD_CYAN, 3000);

        PRINTF("[%lums] running patterns on HARDWARE QSPI1 @ %s\r\n",
               (unsigned long)uptime_ms(), mhz_text(LCD_HwSpiKHz()));
        run_patterns();
    }
}
