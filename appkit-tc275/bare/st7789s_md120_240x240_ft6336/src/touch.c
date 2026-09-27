/*
  touch.c - FT6336 capacitive touch (SCL = P02.5, SDA = P02.4) for the
  appkit-tc275 TK012F6 module port.

  Two backends, selected automatically at boot by Touch_Diag():

    HW  - iLLD IfxI2c_I2c on the I2C0 peripheral (P02.5/P02.4 are its
          pin-map pins), 100 kHz standard mode.
    SOFT- pure GPIO bit-bang (same 100 kHz-ish timing). The f4-demo
          jd9851 port used the same approach when no hardware I2C was
          available on the touch pins.

  Reads `len` bytes starting at register 0x00: 0x00 Device_Mode,
  0x01 GEST_ID, 0x02 TD_STATUS (number of touch points; 1 = valid),
  0x03 P1_XH ([3:0] = X high nibble), 0x04 P1_XL, 0x05 P1_YH
  ([3:0] = Y high nibble), 0x06 P1_YL. 7-bit slave address 0x38
  (the vendor TK80 demo uses the 8-bit form 0x70, same as us).
*/
#include "touch.h"
#include "ASCLIN_Shell_UART.h"
#include "IfxI2c_I2c.h"
#include "IfxI2c_PinMap.h"
#include "IfxPort.h"
#include "ticks.h"
#include <string.h>

/* 7-bit slave address 0x38; iLLD wants the 8-bit address (0x70). */
#define CTP_ADDR_8BIT   0x70U
#define CTP_ADDR_W      0x70U
#define CTP_ADDR_R      0x71U

/* ---- Backend selection ------------------------------------------------
 * The hardware I2C0 engine on this chip cannot drive the P02.4/P02.5
 * pads: its outputs never appear on the wires, no matter which port
 * alternate-output selection (0-7) or PISEL input pair (0-7) is used,
 * and every transaction NAKs (even at 10 kHz, with the clock config
 * verified correct). The investigation is documented in the README
 * ("Known issue: hardware I2C0") and the soft bit-bang backend below
 * is used instead. Re-run the full investigation at boot by building
 * with TOUCH_HW_DIAG=1 (add to CFLAGS). */
#ifndef TOUCH_HW_DIAG
#define TOUCH_HW_DIAG 0
#endif

/* Touch bus GPIO numbers (SCL = P02.5, SDA = P02.4). */
#define T_SCL_PORT      &MODULE_P02
#define T_SCL_PIN       5U
#define T_SDA_PORT      &MODULE_P02
#define T_SDA_PIN       4U

#define T_SCL_HI()      IfxPort_setPinHigh(T_SCL_PORT, T_SCL_PIN)
#define T_SCL_LO()      IfxPort_setPinLow(T_SCL_PORT, T_SCL_PIN)
#define T_SDA_HI()      IfxPort_setPinHigh(T_SDA_PORT, T_SDA_PIN)
#define T_SDA_LO()      IfxPort_setPinLow(T_SDA_PORT, T_SDA_PIN)
#define T_SCL_VAL()     IfxPort_getPinState(T_SCL_PORT, T_SCL_PIN)
#define T_SDA_VAL()     IfxPort_getPinState(T_SDA_PORT, T_SDA_PIN)

/* ~5 us half-bit at the 200 MHz core (a few hundred busy iterations).
 * Gives roughly 60-100 kHz - well inside the FT6336 spec. */
static void t_delay(void)
{
    for (volatile uint32_t i = 0; i < 200U; i++)
    {
    }
}

static IfxI2c_I2c        g_i2c;
static IfxI2c_I2c_Device g_dev;

/* Selected backend: 0 = none yet, 1 = HW iLLD, 2 = soft bit-bang. */
static uint8_t s_mode;
/* GPCTL.PISEL value under which the HW backend ACKed (set by the diag
 * sweep; 0 = iLLD default). */
static uint8_t s_hw_pisel;
/* Port alternate-output indexes under which the HW engine's SCL/SDA
 * actually reach the pads (found by the diag sweep; -1 = iLLD default
 * OutputIdx_alt5). */
static sint8 s_hw_scl_idx = -1;
static sint8 s_hw_sda_idx = -1;

/* ---------------------------------------------------------------------
 * Pad-sampling diagnostic (runs on CPU1): counts 0->1 edges on the two
 * touch wires while CPU0 performs a hardware-bus transaction. Tells us
 * whether the I2C engine's outputs ever reach the pads, and which pad
 * carries the clock:
 *   both ~0 edges      -> engine outputs are NOT routed to the pads
 *   P02.4 edges >> P02.5 -> SCL/SDA alt-output roles swapped on the pads
 *   P02.5 toggles       -> normal roles (SCL on P02.5); ACK-input issue
 */
volatile uint32_t g_pinlog_cmd;    /* 0 idle, 1 sample, 2 stop */
volatile uint32_t g_pinlog_scl_edges;
volatile uint32_t g_pinlog_sda_edges;

void Touch_PinlogCpu1(void)
{
    uint32_t prev = 0xFFFFFFFFU;

    g_pinlog_scl_edges = 0U;
    g_pinlog_sda_edges = 0U;

    for (;;)
    {
        if (g_pinlog_cmd == 1U)
        {
            uint32_t edges_scl = 0U;
            uint32_t edges_sda = 0U;
            uint32_t v = (uint32_t)IfxPort_getPinState(T_SCL_PORT, T_SCL_PIN);
            v |= ((uint32_t)IfxPort_getPinState(T_SDA_PORT, T_SDA_PIN)) << 1U;
            prev = v;
            while (g_pinlog_cmd == 1U)
            {
                v = (uint32_t)IfxPort_getPinState(T_SCL_PORT, T_SCL_PIN);
                v |= ((uint32_t)IfxPort_getPinState(T_SDA_PORT, T_SDA_PIN)) << 1U;
                edges_scl += ((v ^ prev) & 1U);
                edges_sda += ((v ^ prev) >> 1U) & 1U;
                prev = v;
            }
            g_pinlog_scl_edges = edges_scl;
            g_pinlog_sda_edges = edges_sda;
        }
    }
}

/* ---------------------------------------------------------------------
 * GPIO / soft-I2C primitives
 * --------------------------------------------------------------------- */

static void t_pins_gpio_output(void)
{
    IfxPort_setPinModeOutput(T_SCL_PORT, T_SCL_PIN, IfxPort_OutputMode_pushPull,
                             IfxPort_OutputIdx_general);
    IfxPort_setPinModeOutput(T_SDA_PORT, T_SDA_PIN, IfxPort_OutputMode_pushPull,
                             IfxPort_OutputIdx_general);
    T_SCL_HI();
    T_SDA_HI();
}

/* SDA as input (released - the module pull-ups drive it high). */
static void t_sda_release(void)
{
    IfxPort_setPinModeInput(T_SDA_PORT, T_SDA_PIN, IfxPort_InputMode_pullUp);
    t_delay();
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
static uint8_t soft_write_byte(uint8_t b)
{
    uint8_t ack;

    for (uint8_t i = 0; i < 8U; i++)
    {
        if ((b & 0x80U) != 0U)
        {
            T_SDA_HI();
        }
        else
        {
            T_SDA_LO();
        }
        t_delay();
        T_SCL_HI();
        t_delay();
        T_SCL_LO();
        t_delay();
        b = (uint8_t)(b << 1);
    }

    /* 9th clock: release SDA, sample the slave ACK */
    t_sda_release();
    T_SCL_HI();
    t_delay();
    ack = (T_SDA_VAL() == 0U) ? 1U : 0U;
    T_SCL_LO();
    t_delay();
    IfxPort_setPinModeOutput(T_SDA_PORT, T_SDA_PIN, IfxPort_OutputMode_pushPull,
                             IfxPort_OutputIdx_general);
    return ack;
}

/* Read one byte MSB first, ACKing it unless ack == 0 (last byte). */
static uint8_t soft_read_byte(uint8_t ack)
{
    uint8_t b = 0;

    t_sda_release();
    for (uint8_t i = 0; i < 8U; i++)
    {
        T_SCL_HI();
        t_delay();
        b = (uint8_t)((b << 1) | (T_SDA_VAL() != 0U ? 1U : 0U));
        T_SCL_LO();
        t_delay();
    }

    /* 9th clock: ACK (low) or NAK (high) */
    IfxPort_setPinModeOutput(T_SDA_PORT, T_SDA_PIN, IfxPort_OutputMode_pushPull,
                             IfxPort_OutputIdx_general);
    if (ack != 0U)
    {
        T_SDA_LO();
    }
    else
    {
        T_SDA_HI();
    }
    t_delay();
    T_SCL_HI();
    t_delay();
    T_SCL_LO();
    t_delay();
    T_SDA_HI();
    return b;
}

/* Read `len` bytes starting at `reg` over the soft bus. Returns 1 on
 * success (every address byte ACKed), 0 on any NAK. */
static uint8_t soft_read_regs(uint8_t reg, uint8_t *buf, uint8_t len)
{
    uint8_t ok = 1U;

    t_pins_gpio_output();
    soft_start();
    if (soft_write_byte(CTP_ADDR_W) == 0U) { ok = 0U; }
    if (ok != 0U)
    {
        if (soft_write_byte(reg) == 0U) { ok = 0U; }
    }
    if (ok != 0U)
    {
        soft_start();                       /* repeated start */
        if (soft_write_byte(CTP_ADDR_R) == 0U) { ok = 0U; }
    }
    if (ok != 0U)
    {
        for (uint8_t i = 0; i < len; i++)
        {
            buf[i] = soft_read_byte((i == (uint8_t)(len - 1U)) ? 0U : 1U);
        }
    }
    soft_stop();
    return ok;
}

/* ---------------------------------------------------------------------
 * HW (iLLD I2C0) backend
 * --------------------------------------------------------------------- */

/* Read `len` bytes starting at `reg` over the hardware bus. Returns the
 * iLLD status of the final read (IfxI2c_I2c_Status_ok = 0 on success). */
static IfxI2c_I2c_Status hw_read_regs(uint8_t reg, uint8_t *buf, uint8_t len)
{
    IfxI2c_I2c_Status st;

    st = IfxI2c_I2c_write(&g_dev, &reg, 1);
    if (st != IfxI2c_I2c_Status_ok)
    {
        return st;
    }
    st = IfxI2c_I2c_read(&g_dev, buf, len);
    if (st != IfxI2c_I2c_Status_ok)
    {
        memset(buf, 0, len);
    }
    return st;
}

/* ---------------------------------------------------------------------
 * Public API
 * --------------------------------------------------------------------- */

void Touch_Init(void)
{
#if TOUCH_HW_DIAG
    IfxI2c_I2c_Config cfg;
#endif

    /* ---- GPIO stage: probe the wire levels and recover a wedged bus.
     * The module carries pull-ups, so an idle wired bus reads 1/1. A
     * slave stuck mid-transaction (e.g. after a previous aborted run -
     * FT6336 state survives MCU resets) holds SDA low; the classic cure
     * is 9 SCL pulses + a STOP. This also prevents the iLLD init from
     * spinning forever in IfxI2c_stop's wait-bus-idle loop. */
    IfxPort_setPinModeInput(T_SCL_PORT, T_SCL_PIN, IfxPort_InputMode_pullUp);
    IfxPort_setPinModeInput(T_SDA_PORT, T_SDA_PIN, IfxPort_InputMode_pullUp);
    t_delay();
    PRINTF("[TOUCH] wire levels: SCL=%d SDA=%d (1 = pulled high)\r\n",
           (int)T_SCL_VAL(), (int)T_SDA_VAL());

    t_pins_gpio_output();
    if (T_SDA_VAL() == 0U)
    {
        for (uint32_t i = 0; i < 9U; i++)
        {
            T_SCL_LO();
            t_delay();
            T_SCL_HI();
            t_delay();
            if (T_SDA_VAL() != 0U)
            {
                break;      /* slave released SDA */
            }
        }
        /* STOP: SDA low while SCL high, then SDA high */
        T_SCL_HI();
        T_SDA_LO();
        t_delay();
        T_SDA_HI();
        t_delay();
        PRINTF("[TOUCH] bus recovery done, SDA now %d\r\n", (int)T_SDA_VAL());
    }

    /* release the bus to input-high before the peripheral muxes it */
    IfxPort_setPinModeInput(T_SCL_PORT, T_SCL_PIN, IfxPort_InputMode_pullUp);
    IfxPort_setPinModeInput(T_SDA_PORT, T_SDA_PIN, IfxPort_InputMode_pullUp);

#if TOUCH_HW_DIAG
    /* ---- hardware I2C0 stage (investigation build only - see the
     * TOUCH_HW_DIAG note at the top of this file) ---- */
    IfxI2c_I2c_initConfig(&cfg, &MODULE_I2C0);
    cfg.baudrate = 100000.0F;   /* 100 kHz standard mode */
    {
        static const IfxI2c_Pins pins = {
            &IfxI2c0_SCL_P02_5_INOUT,   /* P02.5 */
            &IfxI2c0_SDA_P02_4_INOUT,   /* P02.4 */
            IfxPort_PadDriver_cmosAutomotiveSpeed1
        };
        cfg.pins = &pins;
    }
    IfxI2c_I2c_initModule(&g_i2c, &cfg);

    IfxI2c_I2c_deviceConfig devCfg;
    IfxI2c_I2c_initDeviceConfig(&devCfg, &g_i2c);
    devCfg.deviceAddress = CTP_ADDR_8BIT;
    IfxI2c_I2c_initDevice(&g_dev, &devCfg);

    PRINTF("[TOUCH] i2c clk: fMAX=%u fBAUD1=%u RMC=%u FDIVCFG=%08X TIMCFG=%08X GPCTL=%08X\r\n",
           (uint32)IfxScuCcu_getMaxFrequency(),
           (uint32)IfxScuCcu_getBaud1Frequency(),
           g_i2c.i2c->CLC1.B.RMC,
           g_i2c.i2c->FDIVCFG.U,
           g_i2c.i2c->TIMCFG.U,
           g_i2c.i2c->GPCTL.U);
    s_mode = 0U;    /* Touch_Diag() probes HW vs SOFT and locks one in */
#else
    /* Production path: soft bit-bang backend only. The hardware I2C0
     * engine's outputs are not routable to P02.4/P02.5 on this chip
     * (documented in the README "Known issue" section). */
    s_mode = 2U;
#endif
}

/* One-shot bring-up diagnostic: probes the FT6336 through BOTH backends
 * (hardware iLLD I2C0 and soft bit-bang), prints what each sees and
 * locks in the working one. Status codes (IfxI2c_I2c_Status):
 * 0 = ok, 1 = bus not free, 2 = arbitration lost, 3 = NAK, 4 = error. */
void Touch_Diag(void)
{
    uint8_t buf[8];
    uint8_t firm_soft = 0U;
    uint8_t soft_ok;
#if TOUCH_HW_DIAG
    uint8_t firm_hw  = 0U;
    uint8_t hw_ok;
    IfxI2c_I2c_Status st;
#endif

#if TOUCH_HW_DIAG
    /* ---- HW probe ---- */
    buf[0] = 0U;
    hw_ok = (hw_read_regs(0xA8U, buf, 1) == IfxI2c_I2c_Status_ok) ? 1U : 0U;
    firm_hw = buf[0];
    PRINTF("[TOUCH] diag: HW   FIRMID(0xA8) st_ok=%d id=0x%02X\r\n",
           hw_ok, firm_hw);

    /* ---- HW probe at a very slow baud (if the fast one NAKed): a full
     * iLLD re-init in config mode, since the baud registers can only be
     * changed while the module is stopped. Separates "SCL too fast for
     * the slave" from "wrong routing". ---- */
    if (hw_ok == 0U)
    {
        IfxI2c_I2c_Config cfgSlow;
        static const IfxI2c_Pins pins = {
            &IfxI2c0_SCL_P02_5_INOUT,
            &IfxI2c0_SDA_P02_4_INOUT,
            IfxPort_PadDriver_cmosAutomotiveSpeed1
        };
        IfxI2c_I2c_initConfig(&cfgSlow, &MODULE_I2C0);
        cfgSlow.baudrate = 10000.0F;    /* 10 kHz */
        cfgSlow.pins     = &pins;
        IfxI2c_I2c_initModule(&g_i2c, &cfgSlow);
        PRINTF("[TOUCH] diag: FDIVCFG after 10k re-init=%08X\r\n",
               g_i2c.i2c->FDIVCFG.U);
        buf[0] = 0U;
        g_pinlog_cmd = 1U;      /* CPU1: start sampling */
        hw_ok = (hw_read_regs(0xA8U, buf, 1) == IfxI2c_I2c_Status_ok) ? 1U : 0U;
        g_pinlog_cmd = 2U;      /* CPU1: stop */
        delay_ticks(10);
        PRINTF("[TOUCH] diag: HW @10kHz FIRMID st_ok=%d id=0x%02X "
               "pad edges: SCL(P02.5)=%u SDA(P02.4)=%u\r\n",
               hw_ok, buf[0],
               (unsigned int)g_pinlog_scl_edges,
               (unsigned int)g_pinlog_sda_edges);

        /* ---- PISEL sweep: if even 10 kHz NAKs, the SDA input pair
         * selection (GPCTL.PISEL) may not match this iLLD's pin map.
         * Probe all 8 input pair selections. ---- */
        if (hw_ok == 0U)
        {
            for (uint8_t p = 0U; p < 8U; p++)
            {
                g_i2c.i2c->GPCTL.B.PISEL = p;
                buf[0] = 0U;
                if (hw_read_regs(0xA8U, buf, 1) == IfxI2c_I2c_Status_ok)
                {
                    hw_ok = 1U;
                    s_hw_pisel = p;
                    PRINTF("[TOUCH] diag: HW ACKed with PISEL=%u "
                           "FIRMID=0x%02X\r\n", p, buf[0]);
                    break;
                }
            }
            if (hw_ok == 0U)
            {
                PRINTF("[TOUCH] diag: HW NAKed with all PISEL 0..7\r\n");
                g_i2c.i2c->GPCTL.B.PISEL = 0U;

                /* ---- Alternate-output-index sweep: the CPU1 edge
                 * sampler showed the engine's outputs never reach the
                 * pads with iLLD's OutputIdx_alt5 - sweep all 8 output
                 * selections on both pins and watch which one puts the
                 * clock/data on the wires. ---- */
                static const IfxPort_OutputIdx idxTab[8] = {
                    IfxPort_OutputIdx_general, IfxPort_OutputIdx_alt1,
                    IfxPort_OutputIdx_alt2,    IfxPort_OutputIdx_alt3,
                    IfxPort_OutputIdx_alt4,    IfxPort_OutputIdx_alt5,
                    IfxPort_OutputIdx_alt6,    IfxPort_OutputIdx_alt7
                };
                sint8 scl_idx = -1;
                for (uint8_t i = 0U; i < 8U; i++)
                {
                    IfxPort_setPinModeOutput(T_SCL_PORT, T_SCL_PIN,
                                             IfxPort_OutputMode_openDrain,
                                             idxTab[i]);
                    IfxPort_setPinModeOutput(T_SDA_PORT, T_SDA_PIN,
                                             IfxPort_OutputMode_openDrain,
                                             idxTab[i]);
                    buf[0] = 0U;
                    g_pinlog_cmd = 1U;
                    st = hw_read_regs(0xA8U, buf, 1);
                    g_pinlog_cmd = 2U;
                    delay_ticks(5);
                    PRINTF("[TOUCH] diag: outidx %u: st=%d edges "
                           "SCL=%lu SDA=%lu\r\n", i, (int)st,
                           (unsigned long)g_pinlog_scl_edges,
                           (unsigned long)g_pinlog_sda_edges);
                    if (st == IfxI2c_I2c_Status_ok)
                    {
                        s_hw_scl_idx = scl_idx = (sint8)i;
                        s_hw_sda_idx = (sint8)i;
                        hw_ok = 1U;
                        break;
                    }
                    if (g_pinlog_scl_edges >= 5U)
                    {
                        scl_idx = (sint8)i;   /* clock found on the pad */
                    }
                }

                if ((hw_ok == 0U) && (scl_idx >= 0))
                {
                    /* SCL routing found - sweep SDA separately */
                    IfxPort_setPinModeOutput(T_SCL_PORT, T_SCL_PIN,
                                             IfxPort_OutputMode_openDrain,
                                             idxTab[scl_idx]);
                    for (uint8_t i = 0U; i < 8U; i++)
                    {
                        IfxPort_setPinModeOutput(T_SDA_PORT, T_SDA_PIN,
                                                 IfxPort_OutputMode_openDrain,
                                                 idxTab[i]);
                        buf[0] = 0U;
                        st = hw_read_regs(0xA8U, buf, 1);
                        PRINTF("[TOUCH] diag: SCL=%d SDA outidx %u: "
                               "st=%d id=0x%02X\r\n", (int)scl_idx, i,
                               (int)st, buf[0]);
                        if (st == IfxI2c_I2c_Status_ok)
                        {
                            s_hw_scl_idx = scl_idx;
                            s_hw_sda_idx = (sint8)i;
                            hw_ok = 1U;
                            break;
                        }
                    }
                }

                if (hw_ok != 0U)
                {
                    PRINTF("[TOUCH] diag: HW I2C WORKS with "
                           "outidx SCL=%d SDA=%d\r\n",
                           (int)s_hw_scl_idx, (int)s_hw_sda_idx);
                }
                else
                {
                    PRINTF("[TOUCH] diag: HW I2C outputs unreachable on "
                           "P02.4/P02.5 in any output selection\r\n");
                    /* back to input so the soft backend owns the bus */
                    IfxPort_setPinModeInput(T_SCL_PORT, T_SCL_PIN,
                                            IfxPort_InputMode_pullUp);
                    IfxPort_setPinModeInput(T_SDA_PORT, T_SDA_PIN,
                                            IfxPort_InputMode_pullUp);
                }
            }
        }

        /* restore the 100 kHz config */
        IfxI2c_I2c_Config cfg100k;
        IfxI2c_I2c_initConfig(&cfg100k, &MODULE_I2C0);
        cfg100k.baudrate = 100000.0F;
        cfg100k.pins     = &pins;
        IfxI2c_I2c_initModule(&g_i2c, &cfg100k);
        if (hw_ok != 0U)
        {
            /* re-apply the routing that made the engine reach the pads
             * (initModule just re-muxed everything to iLLD's alt5) */
            g_i2c.i2c->GPCTL.B.PISEL = s_hw_pisel;
            IfxPort_setPinModeOutput(T_SCL_PORT, T_SCL_PIN,
                                     IfxPort_OutputMode_openDrain,
                                     (s_hw_scl_idx >= 0)
                                     ? (IfxPort_OutputIdx)(0x10U << 3) +
                                       ((uint32)s_hw_scl_idx << 3)
                                     : IfxPort_OutputIdx_alt5);
            IfxPort_setPinModeOutput(T_SDA_PORT, T_SDA_PIN,
                                     IfxPort_OutputMode_openDrain,
                                     (s_hw_sda_idx >= 0)
                                     ? (IfxPort_OutputIdx)(0x10U << 3) +
                                       ((uint32)s_hw_sda_idx << 3)
                                     : IfxPort_OutputIdx_alt5);
        }
    }
#endif /* TOUCH_HW_DIAG */

    /* ---- SOFT probe ---- */
    buf[0] = 0U;
    soft_ok = soft_read_regs(0xA8U, buf, 1);
    firm_soft = buf[0];
    PRINTF("[TOUCH] diag: SOFT FIRMID(0xA8) ack=%d id=0x%02X\r\n",
           soft_ok, firm_soft);

    /* ---- pick the backend ---- */
#if TOUCH_HW_DIAG
    if (hw_ok != 0U)
    {
        s_mode = 1U;
        PRINTF("[TOUCH] backend: HW iLLD I2C0\r\n");
    }
    else
#endif
    if (soft_ok != 0U)
    {
        s_mode = 2U;
#if TOUCH_HW_DIAG
        PRINTF("[TOUCH] backend: SOFT bit-bang (HW NAKed)\r\n");
#else
        PRINTF("[TOUCH] backend: SOFT bit-bang\r\n");
#endif
    }
    else
    {
        s_mode = 2U;    /* try soft during runtime anyway */
        PRINTF("[TOUCH] backend: none ACKed, defaulting to SOFT\r\n");
    }

    /* ---- touch data block through the selected backend ---- */
    memset(buf, 0, sizeof(buf));
    if (Touch_Read(buf, 8) != 0U)
    {
        PRINTF("[TOUCH] diag: block:");
        for (uint32_t i = 0; i < 8U; i++)
        {
            PRINTF(" %02X", buf[i]);
        }
        PRINTF("\r\n");
    }
    else
    {
        PRINTF("[TOUCH] diag: block read failed\r\n");
    }
}

/* Read the touch data block (len bytes from register 0x00) through the
 * selected backend. Returns 1 when the bus transfer succeeded, 0 on any
 * NAK/error (the caller then sees "no touch"). */
uint8_t Touch_Read(uint8_t *buf, uint8_t len)
{
    memset(buf, 0, len);

    if (s_mode == 1U)
    {
        return (hw_read_regs(0x00U, buf, len) == IfxI2c_I2c_Status_ok) ? 1U : 0U;
    }

    /* SOFT (also the default before any selection) */
    if (soft_read_regs(0x00U, buf, len) == 0U)
    {
        memset(buf, 0, len);
        return 0U;
    }
    return 1U;
}
