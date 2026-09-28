# tc212-kit

Projects for the **TC212 Application Kit** (TC22x family, AURIX 1G).

![TC212 Application Kit](board_images/board_1.jpg "TC212 Application Kit")

- Device: **TC21x/TC22x** (`DEVICE-ID: TC22x` family)
- Verified silicon CHIPID: `CHID=12 CHREV=1 FSIZE=1` (**TC212, 512 KB program flash**)
- Core: TriCore 1.6.1 (uses `-mcpu=tc22xx`)
- Max CPU frequency: **133 MHz** (datasheet)
- Program flash: **512 KB** (0x80000000-0x80080000), DSPR: **48 KB** (0x70000000)
- No LMU / EDmem on TC212 (linker script removed those regions)
- Debug: miniWiggler (DAS), **TC2_JTAG**
- Serial: **ASC0** on P15.2 (TX) / P15.3 (RX), 115200 8N1

## Projects

| Project         | Description                                                        | Build tool            | Result |
|-----------------|--------------------------------------------------------------------|-----------------------|--------|
| `bare/blink_hello`   | 8 LEDs, CPU/die temp + **AN18 ADC** via ASC0                      | `make`                | boot OK @133.33 MHz, ~39 C die, AN18=2.556 V |
| `bare/dhry_133m`     | Dhrystone 2.1 benchmark (core0)                                    | `make`                | **254,939 Dhrystones/s**, 1.091 DMIPS/MHz @ -Ofast |
| `bare/coremark_133m` | CoreMark 1.0 benchmark (core0)                                     | `make`                | **325.6 CoreMark** (2.44 CoreMark/MHz) @ -Ofast |
| `bare/pwm_buzz_test` | Passive buzzer on **P10.5** (GTM TOM0_CH2), 2048 Hz PWM duty sweep | `make`                | boot + banner OK (audible sweep) |
| `bare/spi_ee_test`   | **AT25128N** SPI EEPROM (P33.5/P20.11/P20.14/P20.12) erase/program/read speed test | `make`          | verify OK: write ~29 KB/s, read ~0.23 MB/s |
| `bare/nv3030b_md183_240x284_cst816d` | TK018F3716 240x284 NV3030B LCD (QSPI1) + CST816D touch (bit-bang I2C) | `make`     | panel clean @33.3 MHz; touch ACK + coords on SDA=P23.1 / SCL=P20.13 |

All projects live in the `bare/` folder; the board root holds only the shared
`Libraries/`, `bare/` and `board_images/`.

The benchmark projects (`bare/dhry_133m`, `bare/coremark_133m`) build with
`-Ofast -ffp-contract=fast` plus `-funroll-loops` (dhry) / `-funroll-all-loops`
(coremark) and `-ffunction-sections -fdata-sections`, matching the
appkit-tc275/appkit-tc234 benchmark builds. (On the cache-less TC212 this set
scores marginally below the old `-O3 -ffast-math` set for Dhrystone and
marginally above for CoreMark; it is kept for cross-board comparability.)
Non-benchmark projects use `-O1`.

## Shared Libraries

The iLLD `Libraries/` folder is shared at the board root (`tc212-kit/Libraries`),
extracted from the **TC22A iLLD 1.20.0** package bundled in AURIX Studio
(`iLLD_1_20_0__TC22A.zip`). It is the same iLLD generation as `appkit-tc234`
(the register base addresses are identical to the TC212 for all shared modules).
The TC212's own memory map is used via each project's linker script
`Lcf_Gnuc_Tricore_Tc.lsl`.

## Pin usage and spare buses

### Pins currently taken

| Function | Pins | Project |
|----------|------|---------|
| ASC0 UART (console) | P15.2 (TX), P15.3 (RX) | all |
| LEDs (8, low active) | P02.0, P02.1, P02.2, P02.3, P02.4, P02.5, P11.10, P11.11 | `bare/blink_hello` |
| Buzzer (GTM TOM0_CH2) | P10.5 | `bare/pwm_buzz_test` |
| QSPI0 SPI EEPROM | P20.11 (SCLK), P20.14 (MOSI), P20.12 (MISO), P33.5 (CS) | `bare/spi_ee_test` |
| VADC AN18 | P41.6 (not on X700/X701) | `bare/blink_hello` |

### Spare SPI

**QSPI1 and QSPI3 are both completely unused** (QSPI0 is the EEPROM, QSPI2
shares pins with the console ASC0). The best spare set is **QSPI1, all on
X700** — four adjacent pins, all on port P11, so one pad-driver/slew setting
covers them all:

| Signal | Pin | X700 | iLLD symbol |
|--------|-----|------|-------------|
| CS (software) | P11.2 | 25 | GPIO — not a QSPI pin |
| MISO (MRST1B) | P11.3 | 26 | `IfxQspi1_MRSTB_P11_3_IN` |
| SCLK | P11.6 | 27 | `IfxQspi1_SCLK_P11_6_OUT` |
| MOSI (MTSR1B) | P11.9 | 28 | `IfxQspi1_MTSR_P11_9_OUT` |

👍 **Use software CS on P11.2** (plain GPIO), not a hardware SLSO. This matches
the project convention and the lessons already recorded for the other boards:
hardware `autoCS` corrupts data on these panels, and a GPIO CS also lets several
sub-transfers share one CS-low frame (which wrapped-command protocols need).
P11.2 *is* also `QSPI1_SLSO5` (alt4, different selector from the alt3 used by
the other three) if a hardware chip-select is ever wanted, but software CS is
the recommended choice.

Naming note: **MISO must use the `_IN` symbol `IfxQspi1_MRSTB_P11_3_IN`**, not
`IfxQspi1_MRST_P11_3_OUT`. Both exist for P11.3 — the `_OUT` variant is the
same pin's alternate *output* function (a master driving MRST), which is not
what a MISO input needs.

#### Trap: the X701 "QSPI3, four adjacent pins" option

P02.4-P02.7 looks like a clean QSPI3 set (SCLK3A = P02.7, MTSR3A = P02.6,
MRST3A = P02.5, SLSO0 = P02.4) on four adjacent X701 pins. **Don't use it:**

- **P02.0-P02.5 are the 8 on-board LEDs** (`bare/blink_hello/led.c`), so
  QSPI3's MISO input **MRST3A = P02.5 collides with LED6**.
- P02.6/P02.7 sit next to those LED pins on the same port. Their nets were
  never probed for the touch bus, so they are not a safe assumption either.

#### Other spare SPI-capable pins

If P11 is unsuitable, these X700 pins also carry QSPI alternate functions:
P11.10/P11.11 (QSPI0/1 SLSO3/4), P20.9 and P20.13 (QSPI0/1 SLSO/SCLK),
P33.6-P33.10 (QSPI1/3 SLSO), P23.1 (QSPI3 SLSO13).

### Spare I2C

**The TC212 has no hardware I2C controller** — there is no I2C module in the
TC22A register set (same as TC23x). "I2C" therefore means a **bit-banged bus on
two spare GPIOs**, which is exactly what the `nv3030b` touch driver already
does on the TC234, so that code transfers directly.

#### ⚠️ Do NOT use P21.6 / P21.7 — they are the JTAG/DAP pins

`P21.6` and `P21.7` are **`P21.6/TDI`** and **`P21.7/TDO/DAP2`** on the TC22xA.
They are the debug interface, and the pin mapper's own reset state confirms it
(P21.6 resets to **PU**, P21.7 to **PD**) — the pull-up/pull-down keeps the JTAG
lines defined while the debugger is attached. Using them as a normal bus means
the DAP/JTAG hardware fights the bus, and driving them can interfere with
debugging and with attaching the flasher.

Despite the iLLD listing only GTM/GPT12 alternate functions for these pins
(its pin-map tables cover *peripheral* routing and never list debug signals),
the TC22xA pin mapper names the debug function in the pin's own primary name.
**This was caught in review — an earlier draft of this document recommended
P21.6/P21.7, which was wrong.**

For reference, the complete set of debug-default pins on TC22xA is:

| Pin | Primary function |
|-----|------------------|
| P21.6 | `P21.6/TDI` (JTAG data in) |
| P21.7 | `P21.7/TDO/DAP2` (JTAG data out / DAP) |
| — | `TCK/DAP0`, `TMS/DAP1` (dedicated debug pins) |
| P20.0 | primary `P20.0`, but carries `OCDS.TGO0` as an alternate — avoid |

#### Verified working pair (used by the nv3030b touch driver)

| Signal | Pin | X700 | Reset | Notes |
|--------|-----|------|-------|-------|
| SCL | P20.13 | 6 | HighZ | `QSPI0_M.CLK` / `QSPI1_M.SEL2` alternates (unused) |
| SDA | P23.1 | 8 | HighZ | `QSPI3_M.SEL13`; also `SCU.EXTCLK0/1` |

These are the pins the TK018F3716 module's CST816D touch controller actually
answers on, confirmed on hardware:

```
[TOUCH] compiled pins: SDA=P23.1 SCL=P20.13
[TOUCH] bus scan: 0x15  (1 device)
[TOUCH] self-test: ACK (chip present)
[TOUCH] down X=105 Y=230 (284-Y=54)
```

⚠️ The two pins are on **different ports** (P20 and P23). An earlier version of
the touch driver's pin hunt only ever tried *same-port* pairs, so it could not
find them however many pairs it tried — worth remembering when writing a pin
hunt: test cross-port combinations too.

The only caveat is P23.1 also carrying `SCU.EXTCLK0`/`SCU.EXTCLK1` as
alternates; that only matters if an external clock source is fed into the chip.

No external pull-ups are needed: the module provides them, which is confirmed
by the bus idling high (`t_bus_idle_ok()` passes on this pair).

#### Pins that are NOT usable for I2C

An earlier revision of this section recommended P21.4/P21.2. **That pair is
wrong** — it was reasoned from the pinmapper alone, and on hardware the CST816D
never answers on it, nor on P20.9/P20.13, P33.6/P33.10 or P14.3/P14.4. Several of
the candidate pairs also read back "stuck low" at reset, which is what a wrong
pair looks like.

| Pins | Why not |
|------|---------|
| P02.0-P02.5, P11.10, P11.11 | the 8 on-board LEDs — the LED nets load the bus, so a low SDA fakes an ACK on *every* address (a scan reported 119 "devices") |
| P21.6, P21.7 | JTAG `TDI`/`TDO` (DAP) — see the warning above |
| P11.2, P11.3, P11.6, P11.9 | the panel SPI above |
| P33.5 | EEPROM CS (`bare/spi_ee_test`) |
| P15.2, P15.3 | console ASC0 |

#### Other verified-free pins

Every one of these is HighZ at reset, non-debug, and open-drain capable:
**P33.6-P33.10**, **P11.12**, **P20.9**, **P14.3/P14.4**. (P33.5/P33.10 clash
with the EEPROM CS / spare SPI, and P11.x with the recommended SPI set above,
so pick a pair that does not clash.) They all probe cleanly but no device
answered on them in the hunt — so treat them as free, not as known touch pins.

### Notes

- All four SPI pins use **alt3**; the iLLD pin-map symbols encode the
  alternate-function selectors (`RxSel_b` for MRST1B/MTSR1B), so passing the
  symbol to `IfxQspi_initSclkOutPin()` / `IfxQspi_initMtsrOutPin()` /
  `IfxQspi_initMrstInPin()` is enough — no manual P11xx register writes. The
  software CS on P11.2 is configured with `IfxPort_setPinModeOutput()`, exactly
  as `bare/spi_ee_test` does for its SLSO7/P33.5 pin.
- X701 exposes the analogue inputs **AN5-AN18 on pins 25-38**. In particular
  **AN18 = X701 pin 38 = P41.6 = VADC G1CH6**, which is what `bare/blink_hello`
  samples - so that pin is in use by the ADC demo and must not be repurposed.
- The power/supply pins (VIN, +3V3, +5V, GND, VAREF), `NC`, `/ESR1` and
  `/PORST` are not usable as GPIO.

## Serial port (host)

The TC212 miniWiggler is a **combined device**: one USB exposes both the
debugger (DAP/JTAG) and a **virtual serial port** for the target UART (printf
style). Open the virtual COM at **115200 8N1**.

UART wiring (FT2232HL Channel B):
- **BDBUS1 (TXD) -> P15.2** - MCU TX (ASC0 ATX0)
- **BDBUS0 (RXD) -> P15.3** - MCU RX (ASC0 ARX0B)

The alternate P14.0/P14.1 paths have `0R_DNA` (do-not-assemble) resistors and
are not populated. **Combo 1 (P15.2/P15.3) is the correct wiring.**

## Build

Each project has a `Makefile` (GNU Make, AURIX GCC `tricore-elf-gcc`,
`-mcpu=tc22xx`). Run from a bash shell (MSYS2 `C:\msys64\usr\bin\bash.exe`
preferred, else Git for Windows bash). `TRICORE_GCC` defaults to
`D:/aurixgcc_03_2026/bin/tricore-elf-gcc`; override it (or set the toolchain on
PATH) if your install differs.

**Generic workflow** — `cd` into any project folder and use the make targets:

```
cd tc212-kit/bare/<project>
make hex      # build build/<proj>.hex
make flash    # program build/<proj>.hex via AURIXFlasher
```

Targets:

| Target       | Action                                              |
|--------------|-----------------------------------------------------|
| `make`       | build `<proj>.hex` (same as `make hex`)             |
| `make hex`   | build `<proj>.hex`                                  |
| `make flash` | program `<proj>.hex` via AURIXFlasher               |
| `make size`  | print section sizes                                 |
| `make clean` | remove `build/`                                     |

Header dependencies are tracked via `-MMD -MP`, so touching a header
automatically rebuilds the affected objects.

Outputs stay in the project folder: `<project>/build/<proj>.hex` (plus
`.elf`/`.map`/objects). These are ignored via the repo `.gitignore`
(`build/`, `*.hex`, `*.elf`, `*.map`, `*.o`).

## Flash

`make flash` programs the project-local hex (defaults to the AURIX Studio
flasher; set `AURIX_FLASHER` to override). From any project folder:

```
cd tc212-kit/bare/<project> && make flash
```

Equivalent direct command (run from the project folder):

```
"D:\Infineon\AURIX-Studio-1.10.36\tools\AurixFlasherSoftwareTool_v3.0.18\AURIXFlasher.exe" ^
    -hex build\<proj>.hex -prog on -ver on -start on
```

The tool auto-detects the connected TC21x device and reports `Pass` on success.

## Notes / gotchas

- **TC212 has only 512 KB flash and 48 KB DSPR.** The bundled TC2xx linker
  defaulted to 1 MB flash (LCF_INTVEC0_START=0x800F4000) which overflowed.
  The corrected `Lcf_Gnuc_Tricore_Tc.lsl` uses 0x80070000 for the interrupt
  table, 48 KB DSPR, and removes the LMU/EDmem regions (TC212 has none).
- TC212 has no LMU, so the CStart SDA4 init needs `__A9_MEM` defined; it is
  aliased to `_SMALL_DATA3_` (A8 base) in the linker script.
- The older iLLD CStart references `_init()`, which newer GCC does not link
  with `-nostdlib`; `abort_stub.c` provides empty `_init`/`_fini` stubs.
- The `end` symbol needed by newlib's `sbrk` is provided via
  `PROVIDE(end = __HEAP_END)` in the linker script.
- `IfxStdIf_DPipe_ascInit()` is the ASC stdIf init name in this iLLD version
  (not `IfxAsclin_Asc_stdIfDPipeInit`).
- The GTM CMU `CLK_EN` register in TC22A has no `IFXGTM_CMU_CLKEN_FXCLK`
  convenience macro; the buzzer enables the FX clocks directly with
  `EN_FXCLK` bits [23:22].
- VADC: **AN18** on the board maps to **VADC G1CH6** (`P41.6`). The blink demo
  polls it via the group scan request slot and waits on the result valid flag
  (`VF`). `IfxVadc_Adc_initModule` must NOT enable `startupCalibration`
  (it enables all converter groups and can hang when only G1 is configured),
  and `IfxVadc_Adc_setScan`'s `mask` argument must cover the channel bits.
- **UART software-FIFO buffers must be 4-byte aligned.** `serial.c` declares
  `g_uartTxBuffer` / `g_uartRxBuffer` as `uint8[]` and `Ifx_Fifo_init()` casts
  them to `Ifx_Fifo *`; the FIFO code then performs 32-bit accesses at struct
  offsets 0/4/12. A plain `uint8[]` only has alignment 1 and gets **no
  alignment directive at all** in the generated assembly — it merely inherits
  the default alignment of its section, so whether it lands on a 4-byte
  boundary is luck (and changes whenever the layout changes). At higher
  optimisation levels GCC emits wide accesses for the FIFO header, and a
  misaligned buffer then raises an **instruction-error trap (class 2, tin 4 =
  data address alignment)**; the iLLD trap handler ends in `__debug()`, so the
  CPU silently parks and serial output just stops mid-message.

  This was reproduced on the TC234 board, where the buffers happened to land
  2-byte aligned (`0x7000319E`) and both benchmarks died mid-report. All UART
  buffers here carry an explicit alignment so the bug cannot reappear:

  ```c
  static uint8 g_uartTxBuffer[ASC_TX_BUFFER_SIZE + sizeof(Ifx_Fifo) + 8] __attribute__((aligned(4)));
  static uint8 g_uartRxBuffer[ASC_RX_BUFFER_SIZE + sizeof(Ifx_Fifo) + 8] __attribute__((aligned(4)));
  ```

  Verified: the generated assembly now carries `.align 2` (4 bytes) for both
  objects and all projects link with both symbols on 4-byte boundaries. Keep
  the attribute on any new UART buffer. Hardware-verified: both benchmarks
  now run to completion at `-Ofast` on this board (previously they died
  mid-report when the buffers landed 2-byte aligned).
