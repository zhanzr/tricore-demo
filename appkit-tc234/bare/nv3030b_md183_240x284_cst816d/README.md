# nv3030b_md183_240x284_cst816d

**NV3030B 240x284 IPS panel + CST816D capacitive touch** for the Application
Kit TC234 (TC23xLP A-Step), module **TK018F3716** (MD183 form factor).

- Panel driver: **NV3030B**, driven over QSPI2 with its *wrapped-command*
  8-bit SPI protocol.
- Touch controller: **CST816D**, bit-banged I2C.
- Console: ASC0 (COM6) at 115200 8N1.

## Wiring

Display SPI (QSPI2):

| Signal        | Port  | X102 pin | Notes                          |
|---------------|-------|----------|--------------------------------|
| CS            | P15.2 | 31       | plain GPIO (software CS)       |
| MISO (MRST2A) | P15.4 | 33       | wired, but **unused by design**|
| MOSI (MTSR2A) | P15.5 | 34       | QSPI2 MTSR2A                   |
| SCLK (SCLK2B) | P15.6 | 35       | QSPI2 SCLK2B                   |

Touch I2C:

| Signal | Port  | X103 pin |
|--------|-------|----------|
| SDA    | P02.0 | 13       |
| SCL    | P02.1 | 14       |

### Write-only by design (MISO)

This module wires MISO to P15.4 (`QSPI2_MRSTA`), but **the driver does not use
it — and must not**. Most modules from this vendor do not bring MISO out to
the connector at all, so depending on it would tie this driver to the few
boards that do. The NV3030B wrapped-command protocol needs no readback (no
status polling, no GRAM read), so nothing in `interface.c` reads the bus even
though the pin is available here.

The practical consequence is that **the panel's readiness cannot be probed** —
hence the boot retry loop below rather than a status poll.

The module has **no DC, reset or backlight pin** in use: the wrapped-command
framing carries command/data, there is no reset (power-cycle is the only
recovery from a latched state), and the backlight is powered from the module
supply.

## Build / flash

Run from MSYS2 or Git Bash in this folder:

```
make          # link build/nv3030b_md183_240x284_cst816d.elf
make hex      # build the .hex
make flash    # program via AURIXFlasher
make size     # section sizes
make clean    # remove build/
```

Serial: **COM6**, 115200 8N1. The demo loops (banner, vendor test screens,
info pages, HSV gradient, LED test) with a live FPS counter.

Defaults: **40 MHz** SPI (`LCD_QSPI_BAUDRATE` in `interface.c`) — validated;
20 MHz also proven, 50 MHz not usable. I2C is **400 kHz** (`T_I2C_HZ` in
`touch.c`, the CST816D Fast-mode maximum). See "SPI clock" below.

Override, e.g. `make CFLAGS+=-DLCD_QSPI_BAUDRATE=20000000.0`.

## Finding the usable SPI rate

`LCD_HwSetBaudrate(khz)` retunes the QSPI live (no reset) and
`LCD_HwSpiKHz()` reports the rate actually programmed, so a rate can be
tested without rebuilding.

**Score every candidate rate with the checkerboard (`stress_pattern()`),
never with solid fills or the colour bars.** Those have no information
content: a bit error inside a solid region is invisible, and the fill timing
stays perfect even when the data is wrong. This is exactly how 50 MHz passed
the original sweep and then corrupted real content — see "Why the default is
20 MHz" below.

History, for reference (measured 2026-09-28, TC234, fMAX = 200 MHz, scored
with the insensitive colour-bar test — treat these timings as throughput
figures only, **not** as proof the rate is usable):

```
[SWEEP] SPI  1000 kHz -> band fill 1110 ms
[SWEEP] SPI  5000 kHz -> band fill  237 ms
[SWEEP] SPI 10000 kHz -> band fill  128 ms
[SWEEP] SPI 20000 kHz -> band fill   74 ms
[SWEEP] SPI 25000 kHz -> band fill   63 ms
[SWEEP] SPI 50000 kHz -> band fill   42 ms
[SWEEP] I2C  50000 Hz -> ACK  ...  400000 Hz -> ACK
```

The I2C ladder is trustworthy — each step is scored on the CST816D ACK
(`Touch_SelfTest()`), a real bus verdict, and every step through the 400 kHz
Fast-mode maximum ACKs.

`LCD_HwSpiKHz()` is rendered through `mhz_text()`, so whole-megahertz rates
print as `50 MHz` rather than `50000 kHz`. `Touch_GetHz()` returns the
*requested* rate: the real SCL period is quantised by `t_delay()`'s one-tick
granularity (5 → 9 ticks at 400 kHz..100 kHz), so deriving it back from the
tick count misreports (400 kHz → "500 kHz").

### Time-quantum sizing (important)

`GLOBALCON.TQ` must be sized from the **fastest** rate in the ladder:

```c
g_qspi->GLOBALCON.B.TQ =
    IfxQspi_calculateTimeQuantumLength(g_qspi, LCD_QSPI_BAUDRATE_MAX);  /* 50 MHz */
```

`TQ` sets the QSPI time quanta (`TQspi = fMAX / (TQ + 1)`) and the channel
dividers are integer multiples of it, so a coarse quantum cannot express a
high baud rate at all. Sizing `TQ` from the *slowest* rate (1 MHz → `TQspi`
= 40 MHz) clamped **every** request above 10 MHz back to 10 MHz — the first
sweep run printed `set 50000 kHz -> real 10000 kHz` for nine consecutive
steps. Sizing from 50 MHz gives `TQ = 0`, i.e. `TQspi = 200 MHz` and ~5 ns
resolution.

### UART software-FIFO buffers

Same 4-byte alignment requirement as the other boards — see the board
`README.md`. Any `uint8 x[.. + sizeof(Ifx_Fifo) + 8]` handed to
`Ifx_Fifo_init()` needs `__attribute__((aligned(4)))`.

## SPI clock: 40 MHz validated, 20 MHz proven, 50 MHz not usable

**50 MHz is not usable on this module.** It completes transfers — so the
QSPI reports `real=50000 kHz` and the fill timing looks perfect — but the
panel then shows **intermittent, pattern-dependent corruption**. That is a
marginal SPI clock (signal integrity), not a driver bug.

| Rate | Result |
|------|--------|
| 20 MHz | reliable (many consecutive clean loops) |
| **40 MHz** | **reliable — validated, current default** |
| 50 MHz | **not usable** — intermittent corruption of detailed content |

`LCD_QSPI_PAD_DRIVER` defaults to `IfxPort_PadDriver_cmosAutomotiveSpeed1`
(the *slowest* edge), which is part of why 40 MHz holds up here.

### Why the earlier sweep "passed" 50 MHz

The sweep scored each rate with `DispBand()`, whose 8 wide **solid** bars are
nearly immune to bit errors: a wrong bit *inside* a solid bar is invisible,
and a solid fill never changes its data, so any error is hidden. Only
detailed content exposes the problem — hence "solid fills correct, grey ramp
and colour/black bars malformed".

`stress_pattern()` in `main.c` is the sensitive test the sweep was missing:
a **1-pixel checkerboard**, where adjacent pixels are opposite so *every*
data bit toggles on *every* pixel, and each row is drawn through
`LCD_CopyBuffer` so the address-window setup is exercised too. One wrong bit
shows up as a broken cell; a lost `WriteComm` shows up as a shifted row.

**Rule: validate a new SPI rate with the checkerboard, never with solid
fills.**

### What was changed

- `LCD_QSPI_BAUDRATE` default lowered to **20 MHz**. Raise it only after
  re-validating with the checkerboard.
- `LCD_QSPI_PAD_DRIVER` added, defaulting to
  **`IfxPort_PadDriver_cmosAutomotiveSpeed1`** — the *slowest* edge. The
  earlier code used `Speed3`; a fast edge on the module's flex cable and
  connector makes overshoot/ringing worse. Slower slew is the standard first
  fix for a marginal high-speed bus. Override with
  `make CFLAGS+=-DLCD_QSPI_PAD_DRIVER=IfxPort_PadDriver_cmosAutomotiveSpeed2`.
- The checkerboard runs every pass and prints the active rate, so corruption
  can be spotted without guessing which phase is which.

### Reading the remaining symptoms

The corruption drifting over time (bad → worse → recovers → different
corruption) is characteristic of a marginal link whose bit errors move with
temperature and refresh content. It is **not** a software race: the DMA is
unused, `spi_send_stream()` is the only writer and completes fully before any
refresh, and the panel periodically refreshes its own GRAM from the values it
received — so stale/marginal cells appear, drift and can clear again with no
MCU involvement. That explains why the display can "recover" on its own.

If 20 MHz still shows any checkerboard corruption, step down to 10 MHz and
re-test; if it is clean at 20 MHz but the demo still shows occasional tearing,
the next lever is verifying the MCU/P15 ground return and shortening the
display wiring.

## Power-on sequencing (important)

**Symptom:** the display works right after flashing but comes up blank after
a power cycle — and then fails at *any* SPI rate, even rates that were proven
good. That makes it look like a clock/signal-integrity problem when it is a
**sequencing** one.

The module has **no reset pin**, so the panel's own power-on reset and its
charge pumps ride on board power — nothing the MCU can drive.

### The panel needs time, and how much varies

A first attempt used a single 250 ms settle wait (`LCD_POWER_SETTLE_MS`)
before the init. That was too short. Measured on a cold boot: after
re-powering the screen showed "hardly any visible change" until it came back
to normal **after about half a minute** — which is *exactly one demo-loop
period* (26.3 s). That timing is the tell: it was the **second** init
(`LCD_Reinit()` at the top of loop 2) that finally took, not the first.

Note the delay is **variable**, not a fixed number to look up: with the settle
raised to 500 ms the panel is typically already up at the first init, so
250 ms was simply below the threshold on that power cycle rather than the
panel needing tens of seconds. The variability is why the code draws through
the wait instead of relying on one constant.

### Fix: draw through the wait instead of sleeping

This module is **write-only** (MISO is deliberately unused — see "Write-only
by design"), so there is no status register to poll and software cannot ask
whether the panel is ready. The panel's wake time also varies run to run, so
no single settle constant is right.

Rather than sleeping, `panel_bringup_draw()` **spends the wait drawing a tiled
image**, interleaved with re-inits:

```c
LCD_Clear();
for (pass = 0; pass < LCD_BRINGUP_PASSES; pass++) {
    asset_fill_range(INFO_TOP, anim_h());   /* whole-screen tile fill */
    LCD_Reinit();                           /* one more init attempt  */
}
asset_fill_range(INFO_TOP, anim_h());       /* final draw, latest init */
```

Measured on hardware:

```
[0ms] bring-up 1/2: draw
[26ms] bring-up: Reinit
[296ms] bring-up 2/2: draw
[321ms] bring-up: Reinit
[616ms] bring-up: done
```

Why this shape:

- **The drawing is the wait.** Every pixel sent is another chance for the
  panel to have come up, so no time is idle.
- **The image appearing IS the "panel is up" signal.** It is drawn through the
  same path as everything else, so it cannot report success while the panel is
  still deaf.
- **Each `LCD_Reinit()` is a fresh init attempt**, and it is what dominates the
  ~0.6 s wall-clock time (the datasheet delays inside it). The loop therefore
  alternates "draw" and "try an init", and a fill that lands tells you the
  preceding init worked.
- **A final draw after the last `LCD_Reinit()`** leaves on screen an image sent
  with the most recent init in effect.
- **Tiles stay whole and inset** — a gap between tiles and a margin all round,
  inside `INFO_TOP`..`anim_h()`, so nothing is clipped by the rounded corners
  and no partial tile is emitted.

`LCD_BRINGUP_PASSES` (default **2**) trades boot time for init attempts, at
roughly 0.5 s per pass.

An earlier version retried the init on a fixed schedule and slept in between.
That worked but wasted the wait and blinked (each retry did a sleep-in /
display-off), which also hid which attempt had landed.

`LCD_Init()` additionally waits `LCD_POWER_SETTLE_MS` (default **500 ms**, in
`lcd.h`) before its first clock edge, and drives **CS high as its very first
action** (in `LCD_GPIOInit()`) so no spurious clock edges are seen as
commands.

## Image assets

The bring-up fill uses `assets/test1.png` (64x64), converted to an RGB565 C
array by a checked-in script:

```
python tools/png_to_rgb565.py ../../assets/test1.png src/lcd/asset_test1 asset_test1
```

It writes `src/lcd/asset_test1.c` (the array) and `src/lcd/asset_test1.h`
(dimensions + declaration). Regenerate rather than editing the `.c` by hand.

Notes on the conversion:

- **Alpha is composited over black**, so transparent and anti-aliased edge
  pixels come out black — matching the panel background — instead of garbage.
- **Standard RGB565 packing** (R 15..11, G 10..5, B 4..0), which is what
  `LCD_CopyBuffer` expects (it sends the high byte first).
- The script is **pure standard library** (`zlib` only), so it runs without
  Pillow/numpy.
- Colour is preserved as far as RGB565 allows; the conversion was verified by
  decoding the generated array back to a PNG and comparing.
- The header guard is `<SYMBOL>_H_INCLUDED`, deliberately **not**
  `<SYMBOL>_H`: the latter would collide with the `<SYMBOL>_H` height macro
  and silently break the height (`#define ASSET_TEST1_H 64` would be
  redefined to nothing by the guard).

The asset is a plain RGB565 array with no third-party artwork, and the script
keeps its provenance auditable.

### Reading the timestamps

Every phase line is prefixed with milliseconds since boot:

```
[0ms] bring-up retry 1/8
[439ms] bring-up retry 2/8
...
[3066ms] bring-up retry 8/8
[3505ms] phase: banner (Reinit)
[6827ms] running patterns on HARDWARE QSPI2 @ 40 MHz
```

That makes a cold boot directly comparable to a warm one, and shows exactly
which retry the panel started responding to.

## Boot diagnostic

`boot_report()` prints the reset source, so a cold boot can be captured with
the same firmware (without it every capture is a warm boot and the failing
case is never observed):

```
[BOOT] RSTSTAT=0x12810000  PORST=1 ESR0=0 ESR1=0 SW=0 SMU=0 CB0=0 CB1=0 CB3=0 EVR13=1
[BOOT] COLD BOOT (power-on reset)
[BOOT] HWCFG=0x74 MODE=1    cpu=200 spb=100 sri=200 fmax=200 MHz
```

`PORST = 1` means power-on reset; `CB1` means a debug reset. Note that the
AURIX Flasher's `-start on` **also** reports `PORST = 1`, so the reset source
alone does not separate the two cases — what actually differs is how long the
panel has been powered.

`LCD_BusDump()` prints the QSPI module and clock tree so a cold boot can be
diffed against a working run:

```
[QSPI] CLC=0x00000000 GLB=0x01003C00 GLB1=0x02800000 ECON0=0x00003240 STATUS=0x00000000
[QSPI] DISS=0 EN=1 TQ=0 fmax=200 MHz spb=100 MHz sri=200 MHz
[QSPI] real=50000 kHz  (requested 50000 kHz)
```

`spi_hw_init()` also forces an explicit `CLC.DISR` disable → enable cycle and
waits for `DISS` at each step, because `DISR` is only a *request* and writes
to `GLOBALCON`/`ECON` are silently dropped while the module is still gated.

## Transport notes

### Wrapped-command framing

Every command is `CS high (settle) -> CS low -> 02 00 <cmd> 00`, and the frame
**stays open** (CS low) so parameter and pixel bytes append contiguously. CS
rises only at `CS_SET()` / `LCD_EndData()`. Because CS is a plain GPIO, the
QSPI sub-frame boundaries inside one CS-low transaction are invisible to the
panel (SCLK just pauses between FIFO-fed bursts, with no edges).

### QSPI2 FIFO protocol

Modelled on iLLD `IfxQspi_SpiMaster`: a burst is fed as
`BACON(LAST=0) + data words`, with a `BACON(LAST=1)` entry before the final
data word. The hardware TX FIFO is **4 entries deep including BACON entries**
(`IFXQSPI_HWFIFO_DEPTH`). Completion is polled via the **PT2 (end of frame)**
event flag. `spi_send_stream()` is non-reentrant and blocks until the frame
finishes.

### Clock configuration

`ms_now()` divides the raw STM ticks returned by `Bsp.h`'s `now()` by the live
`IfxStm_getFrequency()`, so all dwells, the FPS window and the TEST_STAND fill
timings are true milliseconds and stay correct if the CPU clock changes.

## Bugs found and fixed (2026-09-28)

### 1. Freeze on the first QSPI2 transfer in `LCD_Init()` (fixed)

**Symptom:** the serial banner printed, then output stopped dead. LEDs reached
the `LCD_Init` boot probe and never advanced.

**Root cause:** `WriteComm()` never loaded the 4-byte wrapped-command prefix
into the transmit buffer. The nano-f411 reference sends it with a direct
"send now" call, but this port buffers everything and the buffer copy was
dropped, so `s_tx_len` stayed **0**. `spi_send_stream()` then transmitted
nothing and blocked forever on:

```c
while (!g_qspi->STATUS.B.PT2F) { }   /* no frame is running -> never sets */
```

Instrumentation confirmed it exactly:

```
[QSPI] pre len=0 GLB=0x01003C01 GLB1=0x02800000 ECON0=0x00003240 BACON=0x03A00000
[QSPI] PT2F TIMEOUT STATUS=0x00000000 GLB=0x01003C01 GLB1=0x02800000 FIFO=0
```

`len=0` on the first transfer, `STATUS=0` (so `PT2F` clear), FIFO empty. The
QSPI configuration itself (master mode, `EN=1`, ECON, BACON) was correct.

**Fix:** `WriteComm()` now copies the prefix into `s_tx_buf` before flushing.
`spi_send_stream()` additionally returns early when nothing is buffered, and
the `PT2F` wait is bounded so any future misconfiguration reports a diagnostic
instead of freezing silently.

### 2. Millisecond timings were raw STM ticks (fixed)

`ms_now()` returned `now()` directly, but `Bsp.h`'s `now()` returns **STM
ticks**, not milliseconds (the f411 original's `HAL_GetTick()` already counted
ms). At the 100 MHz STM every dwell and the FPS window were ~100000x too long,
and TEST_STAND reported `RED=5031131 ms` instead of ~50 ms. Now divided by the
live STM frequency; fills correctly report 50-51 ms.

### 3. `CS_SET` / `CS_CLR` were never called - no display output (fixed)

`CS_SET` and `CS_CLR` are **functions** (`void CS_SET(void)`), but the code
used them like the macros the f411 port had:

```c
SPI_HW_Flush();
CS_SET;              /* statement with no effect - not a call!  */
CS_CLR;              /* statement with no effect                */
```

`-Wall` flagged these as *"statement with no effect"*. The consequence was
severe: **CS never changed state**, so it sat at whatever level
`LCD_GPIOInit()` left it (high, deselected) and the panel ignored every byte
while the code happily "sent" the whole init sequence and all pixel data. The
SPI waveform, timing and framing were all fine - the panel simply was never
selected. Fixed by calling the functions: `CS_SET();` / `CS_CLR();`.

### 4. I2C master ACK was driven with SDA as an input (fixed)

`soft_read_byte()` released SDA (`t_sda_release()` sets the pin to *input*
mode) for the data bits, then tried to drive the master ACK **without**
restoring the output mode:

```c
T_SCL_LO();
if (ack != 0u) { T_SDA_LO(); } ...   /* pin is an input - write does nothing */
```

The pin had no driver, so no ACK ever reached the slave. The CST816D then
released SDA, the pull-up won and every subsequent byte read back as `0xFF`:

```
[TOUCH] self-test: ACK (chip present)
[TOUCH] id regs: 00 FF FF FF FF FF FF FF     <- byte 0 ok, then bus fault
```

Because `buf[3]` is a *data* byte it could therefore never equal `0x80`, and
touch was never detected. Fixed by switching SDA back to output **before**
driving the ACK bit. The register read is now clean:

```
[TOUCH] self-test: ACK (chip present)
[TOUCH] id regs: 00 00 00 00 00 00 00 00     <- valid idle read
```

## Bring-up diagnostics

Both buses print their configured rate and the touch driver runs a presence
check, so a silent panel/touch can be narrowed down quickly:

```
[TOUCH] CST816D soft I2C target=50 kHz (halfbit=1000 ticks @ 100 ticks/us)
[TOUCH] self-test: ACK (chip present)
[TOUCH] id regs: 00 00 00 00 00 00 00 00
[QSPI] target=10000 kHz  real=10000 kHz  moduleClk=200 MHz  TQ=4
```

- `self-test: NO ACK` means a wiring/address problem, not "no finger".
- `real` far from `target` means the QSPI divider was computed from a wrong
  module-clock assumption (`IfxScuCcu_getMaxFrequency()`).
- Set `LCD_QSPI_TRACE=0` to silence the QSPI line.

## Verification

Captured 2026-09-28, TC234 @ 200 MHz, QSPI2 @ 40 MHz, I2C @ 400 kHz:

```
[TOUCH] self-test: ACK (chip present)
[TOUCH] id regs: 2B 00 00 00 00 00 00 00
[QSPI] target=40000 kHz  real=40000 kHz  moduleClk=200 MHz  TQ=0
[0ms] bring-up retry 1/8
...
[3066ms] bring-up retry 8/8
[3505ms] phase: banner (Reinit)
[6827ms] running patterns on HARDWARE QSPI2 @ 40 MHz
```

No `PT2F` timeouts; the demo loops continuously; the CST816D answers on I2C at
its Fast-mode maximum.

Visually confirmed by the operator:

- 40 MHz shows a **clean checkerboard** across many consecutive loops.
- On a cold power-on the panel comes up by **retry 1** (the on-screen `R1`),
  and the init retry loop recovers it reliably.

The panel itself remains the final authority — the serial log reports what the
MCU sent, not what the module accepted.
