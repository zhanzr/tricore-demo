# dhry_200m

**Dhrystone 2.1** benchmark for the Application Kit TC234 (TC23xLP_A-Step),
running on core0 at **200 MHz** with aggressive GCC speed optimization.

## Build

Standalone GCC build (no IDE, no TASKING license required):

```
powershell -ExecutionPolicy Bypass -File build_dhry_200m.ps1
```

Output: `%TEMP%\dhry_200m_build\dhry_200m.hex`

### Optimization flags

Maximum-speed configuration:

```
-mcpu=tc23xx -O3 -ffast-math -funroll-loops -finline-functions -fno-math-errno
```

- `-O3` base optimization for speed.
- `-ffast-math` allows faster float code (safe here; no strict-IEEE dependency).
- `-funroll-loops` / `-finline-functions` trade code size for speed.
- `-fno-math-errno` avoids errno checks in math builtins.

Run: 2,000,000 runs (`RUN_NUMBER` in `dhry.h`).

## Flash

```
"D:\Infineon\AURIX-Studio-1.10.36\tools\AurixFlasherSoftwareTool_v3.0.18\AURIXFlasher.exe" ^
    -hex %TEMP%\dhry_200m_build\dhry_200m.hex -prog on -ver on -start on
```

Serial: **COM6**, 115200 8N1. The result prints once on reset.

## Results

Captured 2026-08-09, TC234 @ **200.00 MHz**, GCC `-O3 -ffast-math -funroll-loops`:

```
TC234 Dhrystone 2.1, CPU = 200.00 MHz
...
MicroSecond for one run through Dhrystone[1-5221]:  2.610
Dhrystones per Second:  383141.750
DMIPS/MHz:  1.090
```

| Metric                        | Value          |
|-------------------------------|----------------|
| Microseconds / run            | 2.610 µs       |
| Dhrystones / second           | 383,141.75     |
| DMIPS / MHz                   | 1.090          |
| Validation                    | All checks OK  |

For reference, the TC275 (TASKING, `-O3`, single core 200 MHz) measured
183,318 Dhrystones/s / 0.522 DMIPS/MHz.

## Notes

- LEDs blink once after the run finishes.
- The benchmark is single-core; TC234 has a single TriCore CPU.
