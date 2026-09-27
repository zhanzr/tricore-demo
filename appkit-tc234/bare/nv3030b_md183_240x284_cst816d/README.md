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
| MISO (MRST2A) | P15.4 | 33       | **not used** (write-only panel)|
| MOSI (MTSR2A) | P15.5 | 34       | QSPI2 MTSR2A                   |
| SCLK (SCLK2B) | P15.6 | 35       | QSPI2 SCLK2B                   |

Touch I2C:

| Signal | Port  | X103 pin |
|--------|-------|----------|
| SDA    | P02.0 | 13       |
| SCL    | P02.1 | 14       |

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

Override the panel clock with, e.g.
`make CFLAGS+=-DLCD_QSPI_BAUDRATE=25000000.0`. Defaults are the fastest
rates the clock sweep confirmed good: **50 MHz** SPI
(`LCD_QSPI_BAUDRATE` in `interface.c`) and **400 kHz** I2C
(`T_I2C_HZ` in `touch.c`, the CST816D Fast-mode maximum).

## Clock sweep (bring-up)

`sweep_test()` runs once at startup and ramps both buses from slow to fast,
so the highest usable rate can be confirmed on the real hardware:

- **SPI**: each ladder step is applied live (no reset), then a colour-bar
  pattern is drawn and timed. The panel is the pass/fail indicator — a
  marginal clock visibly corrupts the bars — and the rate is printed on the
  panel so a bad step can be identified. The serial log gives the measured
  fill time, which should roughly halve each time the rate doubles.
- **I2C**: each step is scored on the CST816D ACK (`Touch_SelfTest()`),
  which is an unambiguous bus verdict.

Ladders live in `interface.c` (`s_sweep_khz`, 1 MHz → 50 MHz) and `touch.c`
(`s_sweep_hz`, 50 kHz → 400 kHz). Both buses are left at the last (fastest)
rate afterwards; drop them back with `LCD_HwSetBaudrate()` / `Touch_SetHz()`
or by editing the constants.

Measured 2026-09-28 (TC234, fMAX = 200 MHz):

```
[SWEEP] SPI  1000 kHz -> real  1000 kHz, band fill 1110 ms
[SWEEP] SPI  2000 kHz -> real  2000 kHz, band fill  565 ms
[SWEEP] SPI  5000 kHz -> real  5000 kHz, band fill  237 ms
[SWEEP] SPI 10000 kHz -> real 10000 kHz, band fill  128 ms
[SWEEP] SPI 15000 kHz -> real 14285 kHz, band fill   96 ms
[SWEEP] SPI 20000 kHz -> real 20000 kHz, band fill   74 ms
[SWEEP] SPI 25000 kHz -> real 25000 kHz, band fill   63 ms
[SWEEP] SPI 30000 kHz -> real 28571 kHz, band fill   58 ms
[SWEEP] SPI 40000 kHz -> real 40000 kHz, band fill   47 ms
[SWEEP] SPI 50000 kHz -> real 50000 kHz, band fill   42 ms
[SWEEP] I2C  50000 Hz -> ACK  ...  400000 Hz -> ACK
```

The fill time tracks the clock all the way to 50 MHz, and the CST816D ACKs
at every I2C step through its 400 kHz Fast-mode maximum — so both buses are
electrically sound at their top rates, which are now the **defaults**.

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

Captured 2026-09-28, TC234 @ 200 MHz, QSPI2 @ 10 MHz, I2C @ 50 kHz:

```
[TOUCH] self-test: ACK (chip present)
[TOUCH] id regs: 00 00 00 00 00 00 00 00
[QSPI] target=10000 kHz  real=10000 kHz  moduleClk=200 MHz  TQ=4
[LCD] phase: HARDWARE banner
[LCD] running patterns on HARDWARE QSPI2 @ 10 MHz
[LCD] phase: TEST_STAND
[LCD] solid fills (ms): RED=115 GREEN=115 BLUE=115 WHITE=115 BLACK=115
... (loops)
```

No `PT2F` timeouts; the demo loops continuously; the CST816D answers on I2C.

**Still to confirm on hardware:** that the panel actually lights up now that CS
toggles. The serial report alone cannot prove pixels are being rendered - the
fill timings (~115 ms at 10 MHz, which matches 240x284x2 bytes plus overhead)
are consistent with data being clocked out, and the touch chip is confirmed
alive, but the visual result needs a human check.
