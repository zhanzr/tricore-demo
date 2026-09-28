/*
  touch.c - CST816D capacitive touch over bit-banged I2C (SDA = P02.0,
  SCL = P02.1) for the appkit-tc234 TK018F3716 module port.

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

/* Touch bus GPIOs: SCL = P02.1, SDA = P02.0. */
#define T_SCL_PORT      &MODULE_P02
#define T_SCL_PIN       1U
#define T_SDA_PORT      &MODULE_P02
#define T_SDA_PIN       0U

/* 7-bit slave address 0x15 -> wire byte 0x2A (write) / 0x2B (read). */
#define CTP_ADDR_W      0x2AU
#define CTP_ADDR_R      0x2BU

#define T_SCL_HI()      IfxPort_setPinHigh(T_SCL_PORT, T_SCL_PIN)
#define T_SCL_LO()      IfxPort_setPinLow(T_SCL_PORT, T_SCL_PIN)
#define T_SDA_HI()      IfxPort_setPinHigh(T_SDA_PORT, T_SDA_PIN)
#define T_SDA_LO()      IfxPort_setPinLow(T_SDA_PORT, T_SDA_PIN)
#define T_SDA_VAL()     IfxPort_getPinState(T_SDA_PORT, T_SDA_PIN)

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
    IfxPort_setPinModeOutput(T_SDA_PORT, T_SDA_PIN,
                             IfxPort_OutputMode_openDrain,
                             IfxPort_OutputIdx_general);
}

static void t_sda_release(void)
{
    IfxPort_setPinMode(T_SDA_PORT, T_SDA_PIN, IfxPort_Mode_inputPullUp);
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

void Touch_Init(void)
{
    t_delay_init();
    t_set_hz(T_I2C_HZ);

    /* SCL: push-pull output; SDA: open-drain output (released). */
    IfxPort_setPinModeOutput(T_SCL_PORT, T_SCL_PIN,
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
}

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
