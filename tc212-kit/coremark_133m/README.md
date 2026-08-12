# coremark_133m (tc212-kit)

**CoreMark 1.0** benchmark on the TC212 @ 133 MHz, single core (core0).

## Result (measured)

```
TC212 CoreMark, CPU = 133.33 MHz, Die = 40.93 C
2K performance run parameters for coremark.
CoreMark Size    : 666
Total time (secs): 35.244000
Iterations/Sec   : 226.988991
Iterations       : 8000
seedcrc          : 0xe9f5
[0]crclist       : 0xe714
[0]crcmatrix     : 0x1fd7
[0]crcstate      : 0x8e3a
[0]crcfinal      : 0x5275
CoreMark 1.0 : 226.988991 / -O1 / Static
```

All CRCs match the known-good 2K performance run values -> valid score.
**227.0 CoreMark = 1.70 CoreMark/MHz @ -O1** (AURIX GCC 11.3.1).

## Files

- `coremark_1_0_1/` - official CoreMark 1.0.1 sources (`core_main.c`, etc.)
- `coremark_1_0_1/tc212/core_portme.c|h` - TC212 port (STM0 ms ticks via
  `ticks.c`, `PRINTF` over ASC0)
- `ticks.c`/`ticks.h` - STM0 ms tick + delay (shared pattern with dhry_133m)
- `Cpu0_Main.c` - init, prints banner, runs `coremark_main`

## Build / Flash

From a bash shell (MSYS2 `C:\msys64\usr\bin\bash.exe` preferred, else Git for
Windows bash); `TRICORE_GCC` defaults to the AURIX GCC install. **Run from the
project folder:**

```
cd tc212-kit/coremark_133m
make hex      # -> build/coremark_133m.hex
make flash    # programs build/coremark_133m.hex via AURIXFlasher
```

```
"D:\Infineon\AURIX-Studio-1.10.36\tools\AurixFlasherSoftwareTool_v3.0.18\AURIXFlasher.exe" ^
    -hex %TEMP%\tc212_coremark_build\coremark_133m.hex -prog on -ver on -start on
```

## Linker

Uses the TC212-corrected `Lcf_Gnuc_Tricore_Tc.lsl` (512 KB flash, 48 KB DSPR,
no LMU/EDmem). See `tc212-kit/README.md`.
