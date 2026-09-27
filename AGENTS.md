# AGENTS.md

Guidance for AI agents and developers working in this workspace.

## Overview

AURIX multi-board demo workspace for **Infineon TriCore** microcontrollers,
built with **AURIX Development Studio** (ADS). It contains three boards, each
with a set of small demo projects (LED blink, benchmarks, PWM, SPI EEPROM,
ADC, UART).

| Folder          | Board / MCU        | Compiler                              |
|-----------------|--------------------|---------------------------------------|
| `appkit-tc275/` | TC275 (TriCore 1.6.2) | TASKING (ADS IDE; GCC migration planned) |
| `appkit-tc234/` | TC234 (TC23x)      | GCC (`-mcpu=tc23xx`), per-project Makefile |
| `tc212-kit/`    | TC212 (TC21x)      | GCC (`-mcpu=tc22xx`), per-project Makefile |

See each board's `README.md` and the repo-root `README.md`.

## Build system (GCC boards)

All `appkit-tc234` and `tc212-kit` projects use an identical per-project
`Makefile` (GNU Make). **Run from a bash shell** — MSYS2
(`C:\msys64\usr\bin\bash.exe`) preferred, else Git for Windows bash.

Generic workflow (any project folder):

```
make          # build <proj>.hex (same as `make hex`)
make hex      # build build/<proj>.hex
make flash    # program build/<proj>.hex via AURIXFlasher
make size     # print section sizes
make clean    # remove build/
```

Toolchain / flasher defaults (override via environment or `make VAR=...`):

- `TRICORE_GCC ?= D:/aurixgcc_03_2026/bin/tricore-elf-gcc`
- `AURIX_FLASHER ?= D:/Infineon/AURIX-Studio-1.10.36/tools/AurixFlasherSoftwareTool_v3.0.18/AURIXFlasher.exe`

Compile flags: `-std=c11 -Wall -MMD -MP -g`, plus `-ffunction-sections
-fdata-sections`. Optimization:
- Benchmark projects (`dhry_*`, `coremark_*`) in `appkit-tc234`/`tc212-kit`:
  `-O3 -ffast-math -funroll-loops -finline-functions -fno-math-errno`.
- Benchmark projects in `appkit-tc275/bare/`:
  `-Ofast -ffp-contract=fast` + `-funroll-loops` (`dhry_200m`) /
  `-funroll-all-loops` (`coremark_200m`) — matches the `arm-none-eabi-gcc`
  flags of the `nucleo-u575` benchmark reference projects.
- All other projects: `-O1`.

Header dependencies are tracked with `-MMD -MP` (generated `.d` files are
included), so editing a header rebuilds the right objects — no forced `clean`
needed for normal edits.

Link: `-mcpu=<arch> -T <proj>/Lcf_Gnuc_Tricore_Tc.lsl -nostdlib -Wl,--gc-sections
-lgcc -lc -lnosys -lgcc` (libgcc must come after libc for soft-float doubles).

`appkit-tc275` is TASKING-only (free edition builds only from the IDE). The
`setup_libraries_links.sh` script there links the shared `Libraries/`
(symlinks on POSIX, directory junctions on Windows).

## Project layout (GCC projects)

Each project folder contains:

- `Cpu0_Main.c` — entry: `int core0_main(void)` (disable watchdogs, init
  peripherals, main loop).
- `Lcf_Gnuc_Tricore_Tc.lsl` — GCC linker script (board-corrected).
- `Configurations/Ifx_Cfg.h` — iLLD configuration header.
- `abort_stub.c` — empty `_init`/`_fini` stubs required by `-nostdlib`.
- Shared board drivers: `serial.c/h` (ASC0 UART), `dts.c/h` (die temp),
  `led.c/h`, `ticks.c/h` (STM0 ms ticks), and project-specific drivers
  (`buzzer.c`, `ee.c/h`, `adc.c/h`).
- `Makefile`, `README.md`.

Shared iLLD lives at each board root: `<board>/Libraries/` (iLLD drivers +
`Infra/Sfr/TC22A|TC23A|TC27D/_Reg` register headers + `Service`). The Makefiles
compile **all** `Libraries/**/*.c` plus the project sources.

## Serial / hardware

- UART: **ASC0, 115200 8N1**. TC212 wiring: **P15.2 (TX) / P15.3 (RX)**;
  TC234: P14.0/P14.1.
- The miniWiggler exposes a virtual COM (e.g. `COM6`) — open it at 115200 8N1
  to see prints. The same debug probe is used by AURIXFlasher (DAS/JTAG).
- TC212 quirks (see `tc212-kit/README.md`): 512 KB flash / 48 KB DSPR / **no
  LMU or EDmem** — the linker script was corrected for this (intvec at
  `0x80070000`), and `__A9_MEM` must be defined for the CStart SDA4 init even
  without an LMU.
- Flashing is occasionally flaky (first connect may fail with "no device") —
  simply retry. After flashing/resetting, the virtual COM may stop forwarding
  UART until the USB probe is re-plugged.

## Conventions / gotchas

- iLLD is the **1.20.0** generation; it uses
  `IfxStdIf_DPipe_ascInit()` (not `IfxAsclin_Asc_stdIfDPipeInit`).
- newlib needs `PROVIDE(end = __HEAP_END)` in the linker script for `sbrk`.
- VADC: TC212 `AN18` = VADC G1CH6 (`P41.6`). Poll via the group scan slot and
  wait on the result valid flag (`VF`). Do **not** enable `startupCalibration`
  (it can hang when only one group is configured), and pass the channel bits in
  `IfxVadc_Adc_setScan`'s `mask` argument.
- GTM CMU on TC22A has no `IFXGTM_CMU_CLKEN_FXCLK` macro — set
  `CLK_EN.EN_FXCLK` bits [23:22] directly.
- SPI (AT25128N, QSPI0): use SPI **mode 0** (`shiftTransmitDataOnTrailingEdge`
  -> CPH=0) and **software CS** (`autoCS=FALSE`) — hardware autoCS and the
  default CPH=1 corrupt data. 2 MHz is reliable; 8 MHz is not.
- `IFX_INTERRUPT(isr, 0, srn)` places an ISR at vector slot `srn`; the SRC must
  also be wired with `IfxSrc_init`/`IfxSrc_enable`.

## Git

- Build artifacts are ignored (`.gitignore`: `build/`, `**/build/`, `*.hex`,
  `*.elf`, `*.o`, `*.map`, `*/Debug/`, `*/Release/`). Never commit them.
- `*.sh`/`*.ps1` are forced to LF via `.gitattributes`.
- Commit messages are short and lowercase, e.g.
  `add pwm and dts code.`, `build benchmarks with -O3, track header deps in Makefiles, ...`.
- Only commit/push when the user explicitly asks.
