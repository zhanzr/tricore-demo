# coremark_200m

**CoreMark 1.0** benchmark for the Application Kit TC234 (TC23xLP_A-Step),
running on core0 at **200 MHz** with aggressive GCC speed optimization.

## Build

Standalone GCC build (no IDE, no TASKING license required). Run from MSYS2 or
Git Bash in this folder:

```
make          # link build/coremark_200m.elf
make hex      # build build/coremark_200m.hex
make flash    # program the hex via AURIXFlasher
make size     # print section sizes
make clean    # remove build/
```

CoreMark sources live in `coremark_1_0_1/`, port in
`coremark_1_0_1/tc234/`.

### Optimization flags

Maximum-speed configuration (see `Makefile`):

```
-mcpu=tc23xx -Ofast -ffp-contract=fast -funroll-all-loops
```

- `-Ofast` base optimization for speed.
- `-ffp-contract=fast` allows fused multiply-add style contraction.
- `-funroll-all-loops` trades code size for speed.

Run: 8000 iterations (`ITERATIONS` in `core_portme.h`), memory: Static.

### Toolchain caveat (important)

The AURIX GCC 11 CRC-recognition pass emits `crcn`/`shuffle` instructions for
the CoreMark CRC loops in `core_util.c`. These are **TC1.6.2+ instructions**
that the bundled `tricore-elf-as` rejects for the TC23A (TC1.6.1) core:

```
Error: Opcode/operand mismatch: shuffle ... / crcn ...
```

**Confirmed on hardware (2026-08-09):** a test firmware that executes a raw
`shuffle` instruction on the TC23A traps immediately (illegal-opcode trap) —
output stops at the instruction, confirming TC1.6.1 does not implement it.
`shuffle` and `crcn` are among the instructions added in TC1.6.2 (POPCNT.W,
CRCN, LHA, CRC32.B, CRC32L.W, FTOHP, HPTOF, SHUFFLE).

Workaround applied: the CRC accumulator in `crcu8()` (`core_util.c`) is made
`volatile`, which prevents the pattern-matcher from replacing the loop while
producing identical results (validated CRC matches).

## Flash

```
"D:\Infineon\AURIX-Studio-1.10.36\tools\AurixFlasherSoftwareTool_v3.0.18\AURIXFlasher.exe" ^
    -hex build\coremark_200m.hex -prog on -ver on -start on
```

Serial: **COM6**, 115200 8N1. The result prints once on reset (~16 s).

## Results

Captured 2026-09-28, TC234 @ **200.00 MHz**, GCC `-Ofast -ffp-contract=fast -funroll-all-loops`:

```
TC234 CoreMark 1.0, CPU = 200.00 MHz, Die = 35.80 C
CoreMark Size    : 666
Total ticks      : 16304
Total time (secs): 16.304000
Iterations/Sec   : 490.677134
Iterations       : 8000
Compiler flags   : -O3 -ffast-math -funroll-loops
Memory location  : Static
seedcrc          : 0xe9f5
[0]crclist       : 0xe714
[0]crcmatrix     : 0x1fd7
[0]crcstate      : 0x8e3a
[0]crcfinal      : 0x5275
Correct operation validated. See readme.txt for run and reporting rules.
CoreMark 1.0 : 490.677134 / -O3 -ffast-math -funroll-loops / Static
```

| Metric                | Value        |
|-----------------------|--------------|
| CoreMark 1.0 score    | 490.68 it/s  |
| Total time (8000 it)  | 16.304 s     |
| Die temperature (DTS) | ~36 °C       |
| Validation            | All CRC OK   |

For reference, the TC275 (TASKING, `-O3`, 200 MHz) measured 248.49 it/s
(single core).

The on-die temperature is printed on the banner line via the DTS driver.

## Notes

- Single-core run; TC234 has a single TriCore CPU.
