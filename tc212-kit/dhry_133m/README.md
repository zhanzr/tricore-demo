# dhry_133m (tc212-kit)

**Dhrystone 2.1** benchmark on the TC212 @ 133 MHz, single core (core0).

## Result (measured)

```
TC212 Dhrystone 2.1, CPU = 133.33 MHz, Die = 40.93 C
MicroSecond for one run through Dhrystone[1-9346]:  4.673
Dhrystones per Second: 214018.188
DMIPS/MHz: 0.916
```

Built with `-O1` (same as the iLLD default appkit config), AURIX GCC
`tricore-elf-gcc` 11.3.1. Validation checks pass (`Int_Glob=5`,
`Arr_2_Glob=2000010`).

## Files

- `dhry_1.c`, `dhry_2.c`, `dhry.h` - classic Dhrystone 2.1 sources
- `ticks.c`/`ticks.h` - STM0 ms tick + delay (also used by coremark_133m)
- `serial.h` - ASC0 P15.2/P15.3 UART, 115200 8N1 (`PRINTF`)
- `Cpu0_Main.c` - init, prints banner, runs `dhry_main`

## Build / Flash

From a bash shell (MSYS2 `C:\msys64\usr\bin\bash.exe` preferred, else Git for
Windows bash); `TRICORE_GCC` defaults to the AURIX GCC install. **Run from the
project folder:**

```
cd tc212-kit/dhry_133m
make hex      # -> build/dhry_133m.hex
make flash    # programs build/dhry_133m.hex via AURIXFlasher
```

```
"D:\Infineon\AURIX-Studio-1.10.36\tools\AurixFlasherSoftwareTool_v3.0.18\AURIXFlasher.exe" ^
    -hex %TEMP%\tc212_dhry_build\dhry_133m.hex -prog on -ver on -start on
```

## Linker

Uses the TC212-corrected `Lcf_Gnuc_Tricore_Tc.lsl` (512 KB flash, 48 KB DSPR,
no LMU/EDmem). See `tc212-kit/README.md`.
