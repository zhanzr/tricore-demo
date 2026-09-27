# dhry_200m

**Dhrystone 2.1** benchmark for the Application Kit TC234 (TC23xLP_A-Step),
running on core0 at **200 MHz** with aggressive GCC speed optimization.

## Build

Standalone GCC build (no IDE, no TASKING license required). Run from MSYS2 or
Git Bash in this folder:

```
make          # link build/dhry_200m.elf
make hex      # build build/dhry_200m.hex
make flash    # program the hex via AURIXFlasher
make size     # print section sizes
make clean    # remove build/
```

### Optimization flags

Maximum-speed configuration (see `Makefile`):

```
-mcpu=tc23xx -Ofast -ffp-contract=fast -funroll-loops
```

- `-Ofast` base optimization for speed.
- `-ffp-contract=fast` allows fused multiply-add style contraction.
- `-funroll-loops` trades code size for speed.

Run: 2,000,000 runs (`RUN_NUMBER` in `dhry.h`).

## Flash

```
"D:\Infineon\AURIX-Studio-1.10.36\tools\AurixFlasherSoftwareTool_v3.0.18\AURIXFlasher.exe" ^
    -hex build\dhry_200m.hex -prog on -ver on -start on
```

Serial: **COM6**, 115200 8N1. The result prints once on reset.

## Results

Captured 2026-09-28, TC234 @ **200.00 MHz**, GCC `-Ofast -ffp-contract=fast -funroll-loops`:

```
TC234 Dhrystone 2.1, CPU = 200.00 MHz, Die = 35.33 C
...
MicroSecond for one run through Dhrystone[1-5071]:  2.535
Dhrystones per Second:  394477.312
DMIPS/MHz:  1.123
```

| Metric                        | Value          |
|-------------------------------|----------------|
| Microseconds / run            | 2.535 µs       |
| Dhrystones / second           | 394,477.31     |
| DMIPS / MHz                   | 1.123          |
| Die temperature (DTS)         | ~35 °C         |
| Validation                    | All checks OK  |

For reference, the TC275 (TASKING, `-O3`, single core 200 MHz) measured
183,318 Dhrystones/s / 0.522 DMIPS/MHz.

The on-die temperature is printed on the banner line via the DTS driver.

## Notes

- LEDs blink once after the run finishes.
- The benchmark is single-core; TC234 has a single TriCore CPU.
