# nv3030b_md183_240x284_cst816d (tc212-kit)

**TK018F3716 touch LCD module** (NV3030B 1.83" 240x284 panel + CST816D
capacitive touch) on the TC212, ported from the appkit-tc234 project of the
same name. Same demo loop: big-font banner, checkerboard stress pattern,
vendor TEST_STAND screens with timed solid fills, info pages (normal +
inverted), HSV gradient sweep, LED test - live FPS counter throughout, all
phases printed on the ASC0 console.

## Wiring

| Signal | Pin | Header | Notes |
|--------|-----|--------|-------|
| SCLK   | P11.6 | X700-27 | QSPI1 `SCLK1B` (alt3) |
| MOSI   | P11.9 | X700-28 | QSPI1 `MTSR1B` (alt3) |
| CS     | P11.2 | X700-25 | GPIO software CS (no HW SLSO - see tc234 lessons) |
| SDA    | P23.1 | X700-8  | bit-banged I2C, open-drain |
| SCL    | P20.13 | X700-6 | bit-banged I2C, push-pull |

The module has no DC/reset/backlight pin in use (wrapped-command SPI
protocol); MISO (P11.3) is wired but the driver is write-only by design.

## Result (measured)

```
==== tc212-kit (TC212) nv3030b_md183_240x284_cst816d @ 133 MHz ====
[QSPI] target=40000 kHz  real=33333 kHz  moduleClk=133 MHz  TQ=0
[TOUCH] compiled pins: SDA=P23.1 SCL=P20.13
[TOUCH] bus scan: 0x15  (1 device)
[TOUCH] self-test: ACK (chip present)
[TOUCH] down X=105 Y=230 (284-Y=54)
[LCD] solid fills (ms): RED=43 GREEN=43 BLUE=43 WHITE=42 BLACK=43
```

Panel clean at 33.3 MHz SCK across repeated full demo loops (solid fills
~43 ms). Touch ACKs at the CST816D address 0x15 and reports coordinates.

## Porting notes (TC234 -> TC212)

- **QSPI2 -> QSPI1** with the `IfxQspi1_SCLK_P11_6_OUT` /
  `IfxQspi1_MTSR_P11_9_OUT` pin symbols; channel config types differ:
  TC22A iLLD uses `IfxQspi_chMode` / `IfxQspi_chConfig` (fields hold the
  `IfxQspi_ClockPolarity_*` / `IfxQspi_ShiftClock_*` enum values), not the
  TC23A's `SpiIf_ChMode` / `SpiIf_ChConfig`.
- **Default SCK 40 MHz request quantizes to 33.33 MHz** on the 133 MHz
  fMAX (TQ=0, ECON /4) - confirmed by the trace, so no change needed; the
  panel is happy there (the tc234 module is not stable above ~40 MHz
  either).
- **`RED`/`WHITE`/`BLACK`/... renamed to `C565_*`** (src/lcd.h): on TC22A
  the raw RGB565 color macro names collide with ASCLIN register bitfields
  (e.g. `FLAGS.B.RED` = "rising edge detected"), which breaks any TU that
  includes both lcd.h and the ASCLIN headers.
- **LEDs**: the tc234 P13.0-P13.3 LEDs are replaced by the 8 kit LEDs
  (P02.0-P02.5, P11.10, P11.11, low active), driven directly by
  `leds_all()` in main.c.
- **Touch pins auto-detect**: defaults SDA=P23.1 / SCL=P20.13 (verified);
  if the default pair ever fails, `Touch_HuntPins()` retries candidate
  pairs at runtime (with a clean-idle-bus guard - see below). Compile-time
  override: `make EXTRA_DEFS="-DT_SCL_PORT=&MODULE_P20 -DT_SCL_PIN=13
  -DT_SDA_PORT=&MODULE_P23 -DT_SDA_PIN=1"`.

### Pin-selection lessons (for the next port)

- **P21.6/P21.7 are the TC2x DAP2/JTAG pins (TDI/TDO)**, wired to the
  board's miniWiggler connector - unusable for I2C (no device ever
  answers; see tc212-kit/README.md "Pin usage and spare buses").
- An LED net on a candidate SDA pin makes **every address look like an
  ACK** (a scan over P02.0/P02.1 reported 119 "devices") - the pin hunt
  therefore requires both lines idle-high before trusting any ACK.
- The working pair is **cross-port** (P23.1 + P20.13); a same-port-only
  hunt could never have found it.

## Build / Flash

From a bash shell (MSYS2 `C:\msys64\usr\bin\bash.exe` preferred):

```
cd tc212-kit/bare/nv3030b_md183_240x284_cst816d
make hex      # -> build/nv3030b_md183_240x284_cst816d.hex
make flash    # programs it via AURIXFlasher (TC21x auto-detected)
```

## Linker

Uses the TC212-corrected `Lcf_Gnuc_Tricore_Tc.lsl` (512 KB flash, 48 KB
DSPR, no LMU/EDmem). See `tc212-kit/README.md`.
