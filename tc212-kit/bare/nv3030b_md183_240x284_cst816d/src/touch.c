/*
  touch.c - CST816D capacitive touch over bit-banged I2C for the
tc212-kit TK018F3716 module port.

  Ported from the vendored TK018F3716 example's touch_CTP.c and the
  nano-f411 port: the touch data block is read from register 0x00;
  byte 3 (register 0x03) = 0x80 marks an active touch.

  7-bit slave address 0x15; the byte on the wire is 0x2A for write /
  0x2B for read. SDA is open-drain (release = drive 1 with the module
  pull-up), SCL is push-pull (master-only clock).
*/
#include "touch.h"
#include "IfxPort.h"
#include "IfxStm.h"
#include "serial.h"

/* Touch bus GPIOs: SCL = P20.13 (X700-6), SDA = P23.1 (X700-8).
 * Overridable at build time to test wiring orientations:
 *   make EXTRA_DEFS="-DT_SCL_PIN=1 -DT_SDA_PIN=13"   (swapped)
 *
 * NOTE: the X700 pair P21.6/P21.7 was tried first and is NOT usable:
 * those pads are the TC2x DAP2 pins (TDI/TDO), wired to the board's
 * miniWiggler debug connector. They come out of reset in debug function
 * and the attached probe loads/drives them, so no I2C device ever
 * answers on them. P20.13/P23.1 idle High-Z with no debug function. */
/* Touch bus GPIOs. The correct pair is whatever the pin hunt reports as a
 * real device (both addresses ACKed on a clean idle bus) - it is NOT known
 * in advance for this board.
 *
 * Choices ruled out:
 *   P02.0-P02.5, P11.10, P11.11 : on-board LEDs - the LED nets hold the
 *                                 line low, which produces a bogus ACK on
 *                                 every address. Not usable for I2C.
 *   P21.6/P21.7                 : JTAG TDI/TDO (DAP) pads.
 *
 * Overridable at build time, e.g.
 *   make EXTRA_DEFS="-DT_SCL_PIN=4 -DT_SDA_PIN=2"
 */
#ifndef T_SCL_PORT
#define T_SCL_PORT      &MODULE_P20
#endif
#ifndef T_SCL_PIN
#define T_SCL_PIN       13
#endif
#ifndef T_SDA_PORT
#define T_SDA_PORT      &MODULE_P23
#endif
#ifndef T_SDA_PIN
#define T_SDA_PIN       1
#endif

/* Live pins, held in VARIABLES rather than used directly as macros so the
 * bring-up pin hunt below can retry other candidate pairs at runtime - one
 * flash then covers every plausible wiring instead of one flash per pair.
 * (P21.6/P21.7 are deliberately absent: they are the JTAG/TDI/TDO pads.) */
static Ifx_P *s_sdaPort = T_SDA_PORT;
static uint8  s_sdaPin  = (uint8)T_SDA_PIN;
static Ifx_P *s_sclPort = T_SCL_PORT;
static uint8  s_sclPin  = (uint8)T_SCL_PIN;

/* 7-bit slave address 0x15 -> wire byte 0x2A (write) / 0x2B (read). */
#define CTP_ADDR_W      0x2AU
#define CTP_ADDR_R      0x2BU

/* Stringify the compiled-in pin numbers, so the banner reports what is
 * ACTUALLY built rather than a hardcoded string. Without this an
 * EXTRA_DEFS override is invisible and a swap can be mistaken for "no
 * effect". */
#define T_STR(x)        #x
#define T_XSTR(x)       T_STR(x)

#define T_SCL_HI()      IfxPort_setPinHigh(s_sclPort, s_sclPin)
#define T_SCL_LO()      IfxPort_setPinLow(s_sclPort, s_sclPin)
#define T_SDA_HI()      IfxPort_setPinHigh(s_sdaPort, s_sdaPin)
#define T_SDA_LO()      IfxPort_setPinLow(s_sdaPort, s_sdaPin)
#define T_SDA_VAL()     IfxPort_getPinState(s_sdaPort, s_sdaPin)

/* SCL frequency target and delay calibration.
 *
 * The bus is bit-banged, so the SCL period is set purely by how long
 * t_delay() spins. The delay is calibrated against the system timer at
 * init instead of using a guessed loop count, so the real SCL rate is
 * known and stays put if the CPU clock changes.
 *
 * HALF_BIT_US is the time SCL rests in each state. A full SCL period is
 * 2 half-bits, so SCL = 1 / (2 * half_bit).
 */
#ifndef T_I2C_HZ
#define T_I2C_HZ        400000UL    /* CST816D Fast-mode maximum */
#endif

/* Live SCL target. */
#define T_I2C_HZ_MIN    10000UL     /* slowest supported rate             */
#define T_I2C_HZ_MAX    400000UL    /* CST816D supports Fast-mode 400 kHz */

static uint32_t s_ticks_per_us = 1U;   /* STM ticks per microsecond        */
static uint32_t s_half_bit     = 1U;   /* STM ticks per half SCL period    */
static uint32_t s_target_hz    = T_I2C_HZ;  /* requested SCL rate          */

/* Wait one half SCL period. The STM read also gives the loop enough
 * work that it does not need a fixed iteration count. */
static void t_delay(void)
{
    uint32_t start  = IfxStm_get(&MODULE_STM0);
    uint32_t target = s_half_bit;

    while ((IfxStm_get(&MODULE_STM0) - start) < target)
    {
        ;
    }
}

static void t_delay_init(void)
{
    float32 stmHz = IfxStm_getFrequency(&MODULE_STM0);

    if (stmHz < 1.0F)
    {
        stmHz = 100000000.0F;          /* sane fallback if the CCU is unreadable */
    }

    s_ticks_per_us = (uint32_t)((stmHz / 1000000.0F) + 0.5F);
    if (s_ticks_per_us == 0U)
    {
        s_ticks_per_us = 1U;
    }
}

/* Set the SCL rate. Half of the period is spent in each SCL state. */
static void t_set_hz(uint32_t hz)
{
    uint32_t half_us;

    if (hz < T_I2C_HZ_MIN) { hz = T_I2C_HZ_MIN; }
    if (hz > T_I2C_HZ_MAX) { hz = T_I2C_HZ_MAX; }

    half_us    = 1000000UL / (2UL * hz);
    /* Keep at least one STM tick per half bit, otherwise the loop body
     * costs more than the requested delay and the rate is not honoured. */
    s_half_bit = s_ticks_per_us * (half_us != 0UL ? half_us : 1UL);
    if (s_half_bit < s_ticks_per_us)
    {
        s_half_bit = s_ticks_per_us;
    }
    s_target_hz = hz;
}

/* Configuration figure: the rate that was requested. The real SCL rate is
 * quantised by t_delay()'s one-tick granularity, so deriving it back from
 * the tick count misreports (400 kHz -> "500 kHz"). */
uint32_t Touch_GetHz(void)
{
    return s_target_hz;
}

static void t_sda_out_od(void)
{
    IfxPort_setPinModeOutput(s_sdaPort, s_sdaPin,
                             IfxPort_OutputMode_openDrain,
                             IfxPort_OutputIdx_general);
}

static void t_sda_release(void)
{
    IfxPort_setPinMode(s_sdaPort, s_sdaPin, IfxPort_Mode_inputPullUp);
}

static void soft_start(void)
{
    T_SDA_HI();
    T_SCL_HI();
    t_delay();
    T_SDA_LO();     /* START: SDA falls while SCL is high */
    t_delay();
    T_SCL_LO();
    t_delay();
}

static void soft_stop(void)
{
    T_SDA_LO();
    t_delay();
    T_SCL_HI();
    t_delay();
    T_SDA_HI();     /* STOP: SDA rises while SCL is high */
    t_delay();
}

/* Write one byte MSB first. Returns 1 if the slave ACKed. */
static uint8_t soft_write_byte(uint8_t dat)
{
    uint8_t ack;

    for (uint8_t i = 0; i < 8u; i++)
    {
        if ((dat & 0x80u) != 0u) { T_SDA_HI(); } else { T_SDA_LO(); }
        t_delay();
        T_SCL_HI();
        t_delay();
        T_SCL_LO();
        t_delay();
        dat = (uint8_t)(dat << 1);
    }

    /* 9th clock: release SDA, sample the slave ACK */
    t_sda_release();
    T_SCL_HI();
    t_delay();
    ack = (T_SDA_VAL() == 0u) ? 1u : 0u;
    T_SCL_LO();
    t_delay();
    t_sda_out_od();
    return ack;
}

/* Read one byte MSB first; ACK it unless ack == 0 (last byte).
 *
 * IMPORTANT: the ACK bit is driven with SDA configured as an OUTPUT.
 * t_sda_release() puts the pin into input mode, so writing SDA before
 * restoring the output mode does nothing (the pin has no driver) and the
 * slave never sees the ACK. It then releases SDA, the pull-up wins, and
 * every following byte reads back as 0xFF - the classic
 * "byte0 = <id>, byte1..n = FF" symptom. Drive SDA low FIRST, then
 * clock the 9th bit. */
static uint8_t soft_read_byte(uint8_t ack)
{
    uint8_t dat = 0u;

    t_sda_release();
    for (uint8_t i = 0; i < 8u; i++)
    {
        T_SCL_LO();
        t_delay();
        T_SCL_HI();
        t_delay();
        dat = (uint8_t)((dat << 1) | (T_SDA_VAL() != 0u ? 1u : 0u));
    }
    T_SCL_LO();
    t_delay();

    /* Master ACK/NACK: needs SDA driven, so restore the output mode
     * before setting the level. */
    t_sda_out_od();
    if (ack != 0u) { T_SDA_LO(); } else { T_SDA_HI(); }
    t_delay();
    T_SCL_HI();
    t_delay();
    T_SCL_LO();
    t_delay();
    return dat;
}

/* Bring-up check: probe only the address byte and report the ACK, so a
 * wiring/address problem is distinguishable from "no finger down". */
uint8_t Touch_SelfTest(void)
{
    uint8_t acked;

    soft_start();
    acked = soft_write_byte(CTP_ADDR_W);
    soft_stop();

    return acked;
}

/* ---------------------------------------------------------------------
   Bring-up pin hunt.

   The module's I2C pins are not documented for this board, so rather than
   guessing one pair per flash, try every plausible free X700 pair at
   runtime. Each candidate is set up as a normal open-drain I2C bus and
   probed; the first pair where the CST816D answers both ways (read AND
   write address) is reported over the serial console.

   Excluded on purpose:
     - P21.6/P21.7           : JTAG TDI/TDO (DAP) pads
     - P02.0-P02.5           : the 8 on-board LEDs
     - P02.4-P02.7, P33.9/10 : QSPI3 / spare-SPI alternates
     - P11.2/3/6/9           : the panel SPI in use
   ------------------------------------------------------------------- */
typedef struct
{
    Ifx_P *sdaPort;
    uint8  sda;
    Ifx_P *sclPort;
    uint8  scl;
} T_PinPair;

/* SDA and SCL may be on DIFFERENT ports - the connector interleaves them
 * (X700 has P20.9/P20.13 and P23.1 on separate port blocks), and the first
 * version of this table only ever tried same-port pairs, which is the most
 * likely reason it found nothing.
 *
 * Excluded, with reasons:
 *   P02.0-P02.5, P11.10, P11.11 : the 8 on-board LEDs. Their nets hold the
 *                                 line low / add load, so an "ACK" read
 *                                 there is bogus (a stuck-low SDA looks
 *                                 like an ACK from every address - the
 *                                 scan reported 119 devices).
 *   P21.6/P21.7                 : JTAG TDI/TDO (DAP) pads.
 *   P11.2/11.3/11.6/11.9        : the panel SPI in use.
 *   P33.5                       : EEPROM CS (spi_ee_test).
 *   P15.2/P15.3                 : console ASC0.
 */
static const T_PinPair t_pairs[] = {
    /* The user's actual wiring first, then its inverse. */
    { &MODULE_P23, 1,  &MODULE_P20, 13 },
    { &MODULE_P20, 13, &MODULE_P23, 1  },
    { &MODULE_P20, 9,  &MODULE_P23, 1  },
    { &MODULE_P23, 1,  &MODULE_P20, 9  },
    { &MODULE_P20, 9,  &MODULE_P20, 13 },
    { &MODULE_P20, 13, &MODULE_P20, 9  },
    { &MODULE_P33, 9,  &MODULE_P33, 10 },
    { &MODULE_P33, 10, &MODULE_P33, 9  },
    { &MODULE_P14, 4,  &MODULE_P14, 3  },
    { &MODULE_P14, 3,  &MODULE_P14, 4  },
    /* Older same-port candidates, kept so one flash covers everything. */
    { &MODULE_P21, 4,  &MODULE_P21, 2  },
    { &MODULE_P21, 2,  &MODULE_P21, 4  },
    { &MODULE_P33, 6,  &MODULE_P33, 7  },
    { &MODULE_P33, 7,  &MODULE_P33, 6  },
    { &MODULE_P11, 12, &MODULE_P11, 3  },
    { &MODULE_P11, 3,  &MODULE_P11, 12 },
    { &MODULE_P02, 6,  &MODULE_P02, 7  },
    { &MODULE_P02, 7,  &MODULE_P02, 6  },
};
#define T_PAIR_COUNT (sizeof(t_pairs) / sizeof(t_pairs[0]))

/* A pair is only worth probing if BOTH lines actually idle high.
 *
 * This is the check that was missing: with SDA stuck low (e.g. an LED net
 * holding it), every address "ACKs" and the hunt reports a false device.
 * Release both lines (input + pull-up) and require 1/1 before trusting any
 * ACK. */
static uint8 t_bus_idle_ok(void)
{
    uint8 sda, scl;

    IfxPort_setPinMode(s_sdaPort, s_sdaPin, IfxPort_Mode_inputPullUp);
    IfxPort_setPinMode(s_sclPort, s_sclPin, IfxPort_Mode_inputPullUp);

    sda = (uint8)IfxPort_getPinState(s_sdaPort, s_sdaPin);
    scl = (uint8)IfxPort_getPinState(s_sclPort, s_sclPin);

    return (uint8)((sda != 0u) && (scl != 0u));
}

/* Human-readable port number for the log. Covers every port the scan and
 * the pair hunt can touch. */
static uint8 t_port_num(Ifx_P *port)
{
    if (port == &MODULE_P00) { return 0U; }
    if (port == &MODULE_P02) { return 2U; }
    if (port == &MODULE_P10) { return 10U; }
    if (port == &MODULE_P11) { return 11U; }
    if (port == &MODULE_P14) { return 14U; }
    if (port == &MODULE_P20) { return 20U; }
    if (port == &MODULE_P21) { return 21U; }
    if (port == &MODULE_P23) { return 23U; }
    if (port == &MODULE_P33) { return 33U; }
    return 99U;   /* unmapped: shows as P99 and is obvious in the log */
}

/* Scan candidate pins for an EXTERNAL pull-up, which is what identifies a
 * pin that is really wired to an I2C bus.
 *
 * Method: configure the pin as an input with the INTERNAL PULL-DOWN
 * enabled, then read it. A pin with nothing attached (or with only a
 * push-pull driver) reads 0. A pin sitting on a bus with an external
 * pull-up to 3V3 still reads 1 despite the internal pull-down, because the
 * external resistor wins. That is a wiring fact, independent of any I2C
 * protocol timing, so it cannot be masked by a marginal clock or a bad
 * ACK phase.
 *
 * Ports/pins chosen are those reachable on X700/X701 and not otherwise
 * claimed. P21.6/P21.7 are excluded (JTAG), as requested. */
typedef struct { Ifx_P *port; uint8 pin; } T_PinRef;

static const T_PinRef t_scan_pins[] = {
    { &MODULE_P21,  2 }, { &MODULE_P21,  4 },
    { &MODULE_P20,  9 }, { &MODULE_P20, 13 },
    { &MODULE_P23,  1 },
    { &MODULE_P33,  5 }, { &MODULE_P33,  6 }, { &MODULE_P33,  7 },
    { &MODULE_P33,  8 }, { &MODULE_P33,  9 }, { &MODULE_P33, 10 },
    { &MODULE_P11,  2 }, { &MODULE_P11,  3 }, { &MODULE_P11,  6 },
    { &MODULE_P11,  9 }, { &MODULE_P11, 10 }, { &MODULE_P11, 11 },
    { &MODULE_P11, 12 },
    { &MODULE_P14,  3 }, { &MODULE_P14,  4 }, { &MODULE_P14,  6 },
    { &MODULE_P10,  6 }, { &MODULE_P02,  8 }, { &MODULE_P00,  0 },
};
#define T_SCAN_COUNT (sizeof(t_scan_pins) / sizeof(t_scan_pins[0]))

void Touch_HuntPullups(void)
{
    uint32_t i;

    PRINTF("[TOUCH] external pull-up scan (%u pins, internal pull-DOWN on;"
           " reads 1 => external pull-up => real bus line):\r\n",
           (unsigned)T_SCAN_COUNT);

    for (i = 0; i < T_SCAN_COUNT; i++)
    {
        uint8 v;

        IfxPort_setPinMode(t_scan_pins[i].port, t_scan_pins[i].pin,
                           IfxPort_Mode_inputPullDown);
        v = (uint8)IfxPort_getPinState(t_scan_pins[i].port, t_scan_pins[i].pin);

        if (v != 0u)
        {
            PRINTF("[TOUCH]   P%d.%u = 1  <== EXTERNAL PULL-UP\r\n",
                   (unsigned)t_port_num(t_scan_pins[i].port),
                   (unsigned)t_scan_pins[i].pin);
        }
    }
    PRINTF("[TOUCH] pull-up scan done (pins not listed read 0)\r\n");
}

/* Point the bus at a candidate pair and release the lines. SDA and SCL may
 * be on different ports. */
static void t_select_pins(const T_PinPair *p)
{
    s_sdaPort = p->sdaPort;
    s_sdaPin  = p->sda;
    s_sclPort = p->sclPort;
    s_sclPin  = p->scl;

    IfxPort_setPinModeOutput(s_sclPort, s_sclPin,
                             IfxPort_OutputMode_pushPull,
                             IfxPort_OutputIdx_general);
    t_sda_out_od();
    T_SCL_HI();
    T_SDA_HI();
}

/* Walk the candidate list and report every pair where the chip answers.
 * Returns 1 if a working pair was found (and leaves the bus pointed at it). */
uint8_t Touch_HuntPins(void)
{
    uint32_t i;

    PRINTF("[TOUCH] pin hunt over %u candidate pairs:\r\n",
           (unsigned)T_PAIR_COUNT);

    for (i = 0; i < T_PAIR_COUNT; i++)
    {
        uint8_t rw;

        t_select_pins(&t_pairs[i]);

        /* Reject the pair unless the bus actually idles high. Without this
         * a pin held low reports a bogus ACK on every address. */
        if (t_bus_idle_ok() == 0u)
        {
            PRINTF("[TOUCH]   SDA=P%d.%u SCL=P%d.%u -> SKIP (line stuck low)\r\n",
                   t_port_num(t_pairs[i].sdaPort), (unsigned)t_pairs[i].sda,
                   t_port_num(t_pairs[i].sclPort), (unsigned)t_pairs[i].scl);
            continue;
        }

        t_select_pins(&t_pairs[i]);      /* back to open-drain output */

        /* Probe the write address and the read address: a real ACK on both
         * is much stronger evidence than a single lucky low reading. */
        soft_start();
        rw  = soft_write_byte(CTP_ADDR_W);
        soft_stop();
        soft_start();
        rw += soft_write_byte(CTP_ADDR_R);
        soft_stop();

        PRINTF("[TOUCH]   SDA=P%d.%u SCL=P%d.%u -> ACK %u/2%s\r\n",
               t_port_num(t_pairs[i].sdaPort), (unsigned)t_pairs[i].sda,
               t_port_num(t_pairs[i].sclPort), (unsigned)t_pairs[i].scl,
               (unsigned)rw, (rw >= 2u) ? "   *** DEVICE ***" : "");

        if (rw >= 2u)                    /* both addresses must ACK      */
        {
            return 1u;
        }
    }

    PRINTF("[TOUCH] hunt: no device on any candidate pair\r\n");
    return 0u;
}

/* Probe every 7-bit address (0x01..0x7F) and list the ones that ACK.
 * Bring-up tool for a new wiring: distinguishes "nothing on the bus"
 * (wiring/power problem) from "device at another address" (list shows it). */
void Touch_Scan(void)
{
    uint8_t addr;
    uint8_t found = 0u;

    PRINTF("[TOUCH] bus scan:");
    for (addr = 1u; addr < 120u; addr++)
    {
        uint8_t ack;

        soft_start();
        ack    = soft_write_byte((uint8_t)(addr << 1));
        ack   |= soft_write_byte(0x00u);   /* second byte: many chips only ACK pairs */
        soft_stop();

        if (ack != 0u)
        {
            PRINTF(" 0x%02X", addr);
            found++;
        }
    }
    PRINTF("  (%u device%s)\r\n", found, (found == 1u) ? "" : "s");
}

void Touch_Init(void)
{
    t_delay_init();
    t_set_hz(T_I2C_HZ);

    /* SCL: push-pull output; SDA: open-drain output (released). */
    IfxPort_setPinModeOutput(s_sclPort, s_sclPin,
                             IfxPort_OutputMode_pushPull,
                             IfxPort_OutputIdx_general);
    t_sda_out_od();
    T_SCL_HI();
    T_SDA_HI();
    t_delay();
    soft_stop();        /* release any stuck state from a prior session */

    PRINTF("[TOUCH] CST816D soft I2C target=%lu kHz (halfbit=%lu ticks @ %lu ticks/us)\r\n",
           (unsigned long)(Touch_GetHz() / 1000UL),
           (unsigned long)s_half_bit,
           (unsigned long)s_ticks_per_us);
    PRINTF("[TOUCH] compiled pins: SDA=P%d.%s SCL=P%d.%s\r\n",
           (unsigned)t_port_num(s_sdaPort), T_XSTR(T_SDA_PIN),
           (unsigned)t_port_num(s_sclPort), T_XSTR(T_SCL_PIN));}

/* Read len bytes starting at register 0x00 (the CST816D touch data
 * block): one write transaction sets the register pointer, then a
 * read transaction fetches the data (the CST816D auto-increments).
 * On any NAK the buffer is zeroed (caller sees "no touch"). */
void Touch_Read(uint8_t *buf, uint8_t len)
{
    uint8_t ok = 1u;
    uint8_t i;

    for (i = 0; i < len; i++) { buf[i] = 0u; }

    soft_start();
    if (soft_write_byte(CTP_ADDR_W) == 0u) { ok = 0u; }
    if (ok != 0u)
    {
        if (soft_write_byte(0x00u) == 0u) { ok = 0u; }
    }

    if (ok != 0u)
    {
        soft_start();
        if (soft_write_byte(CTP_ADDR_R) == 0u) { ok = 0u; }
        if (ok != 0u)
        {
            for (i = 0; i < len; i++)
            {
                buf[i] = soft_read_byte((uint8_t)(i == (len - 1u) ? 0u : 1u));
            }
        }
    }
    soft_stop();

    if (ok == 0u)
    {
        for (i = 0; i < len; i++) { buf[i] = 0u; }
    }
}
