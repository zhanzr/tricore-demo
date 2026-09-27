# st7789s_md120_240x240_ft6336 (appkit-tc275)

Drives the **TK012F6** 240x240 touch LCD module on the Application Kit
TC2X5: **ST7789S** 1.2" panel + **FT6336** capacitive touch controller.

Ported from the STM32 reference ports (same module, identical drawing
code): `nucleo-f722`, `nucleo-l4r5`, `nucleo-u575`
(`bare/st7789s_md120_240x240_ft6336` in each workspace). Only the bus
glue is board-specific:

- LCD: ST7789S over **QSPI2 @ 28.6 MHz**. The module has no D/C pin - the panel
  is strapped for 3-wire serial, so every byte is a 9-bit frame (D/C bit
  first). QSPI2 sends packed 16-bit words carrying the 9-bit bitstream
  (same trick as the STM32 ports), SPI mode 3. 28.6 MHz = 200 MHz fMAX / 7;
  measured full-screen fill is ~87 ms. 25 MHz is the conservative option,
  higher steps (33.3 marginal, 50 marginal/unstable) via
  `make CFLAGS+=-DLCD_QSPI_BAUDRATE=25000000.0` (or 33333333.0 /
  50000000.0).
- Touch: **FT6336 over bit-banged (soft) I2C** on P02.5/P02.4, ~100 kHz
  (see the known issue below for why the hardware I2C is not used).
  Touch_Init performs a 9-clock GPIO bus recovery first: the FT6336 can
  hold SDA low after an aborted transaction (its state survives MCU
  resets), which otherwise wedges the bus.

## Known issue: hardware I2C0 (P02.4/P02.5) is not usable

The TC27x **I2C0 hardware engine cannot drive the P02.4/P02.5 pads** on
this board: its outputs never appear on the wires, while plain GPIO
bit-banging on the exact same pins works flawlessly. Investigated to the
end (findings are re-printed on the console in the
`TOUCH_HW_DIAG=1` build):

- Clock config verified correct: `fBAUD1 = 100 MHz`, `RMC = 1`,
  `FDIVCFG`/`TIMCFG` written and read back as configured.
- Every transaction NAKs - even after a full iLLD re-init at **10 kHz**
  (so timing is not the cause).
- All 8 `GPCTL.PISEL` input-pair selections: NAK.
- All 8 port alternate-output selections (`general`..`alt7`): a CPU1
  edge-sampler watching the pads during HW transfers counted **zero
  edges** - the engine's SCL/SDA outputs are physically unroutable to
  these pads.
- Wiring, pull-ups and module power proven good (the soft backend ACKs
  the same pins; the module also works on the STM32 reference boards).

Conclusion: iLLD pin-map bug (`IfxI2c0_SCL_P02_5_INOUT` claims
`OutputIdx_alt5`) or an I2C IP limitation on this chip step - not
fixable in software. The soft backend is used unconditionally by
default; build with `TOUCH_HW_DIAG=1` to re-run the whole investigation
at boot (HW probes, sweeps, CPU1 pad sampler).

## Wiring (Application Kit TC2X5 connectors)

| Module signal | TC275 pin | Connector | Function            |
|---------------|-----------|-----------|---------------------|
| LCD_MOSI      | P15.5     | X102-34   | QSPI2 MTSR2A (SDA)  |
| LCD_MISO      | P15.4     | X102-33   | not used (not wired on module) |
| LCD_SCLK      | P15.6     | X102-35   | QSPI2 SCLK2B        |
| LCD_CS        | P14.2     | X102-39   | GPIO software CS    |
| TOUCH_SCL     | P02.5     | X103-18   | I2C0 SCL            |
| TOUCH_SDA     | P02.4     | X103-17   | I2C0 SDA            |
| GND           | GND       | X102-3/4  | common ground       |

The module has no reset pin (the vendor init sequence settles CS
instead) and its backlight is hardwired on-module. Power the module
from a stable 3.3 V supply; common ground with the kit is required.

## Demo

Endless pass: banner page, TEST_STAND vendor screens (with timed solid
fills), info pages (normal + inverted), HSV gradient sweep, LED test -
with a live FPS counter and FT6336 touch state (`[TOUCH] down/release`)
printed on the ASC0 console (**115200 8N1**, virtual COM of the
on-board miniWiggler, e.g. COM6).

NOTE: the panel has no reset pin, so a latched bad state survives MCU
resets - power-cycle the module itself when comparing fixes.

## Build

```
make          # build build/st7789s_md120_240x240_ft6336.elf
make hex      # build build/st7789s_md120_240x240_ft6336.hex
make flash    # program via AURIXFlasher
```
