# pwm_buzz_test (tc212-kit)

Passive buzzer test on the TC212 using GTM **TOM0_CH2 PWM on P10.5**.

## What it does

- 2048 Hz PWM on **P10.5** (TOUT107) via GTM TOM0_CH2.
- Sweeps duty cycle **0 -> 100%** (10% steps, 100 ms), holds at 100%, sweeps
  **100 -> 0%**, holds silent, loops.
- Prints once over ASC0 UART (115200 8N1, P15.2/P15.3):
  `pwm_buzz_test: P10.5 TOM0_CH2 2048 Hz, sweep duty 0-100-0, Die = 40.47 C`

## Files

- `buzzer.c`/`buzzer.h` - GTM TOM PWM driver for P10.5
- `Cpu0_Main.c` - duty sweep loop
- `serial.h` - ASC0 UART (`PRINTF`)

## Notes

- P10.5 can only be TOM **TOUT107** (`IfxGtm_TOM0_2_TOUT107_P10_5_OUT`,
  output alt1). GPT12 T6OUT P10.5 is the alternative peripheral on the pin.
- The TC22A GTM CMU has no `IFXGTM_CMU_CLKEN_FXCLK` convenience macro; the
  FX clocks are enabled directly via `CLK_EN.EN_FXCLK` bits [23:22].
- Fxclk1 = GCLK / 16 ~ 8.3 MHz at 133 MHz GCLK -> 4076 ticks for 2048 Hz.

## Build / Flash

From a bash shell (MSYS2 `C:\msys64\usr\bin\bash.exe` preferred, else Git for
Windows bash); `TRICORE_GCC` defaults to the AURIX GCC install. **Run from this
project folder:**

```
make hex      # -> build/pwm_buzz_test.hex
make flash    # programs build/pwm_buzz_test.hex via AURIXFlasher
```

```
"D:\Infineon\AURIX-Studio-1.10.36\tools\AurixFlasherSoftwareTool_v3.0.18\AURIXFlasher.exe" ^
    -hex %TEMP%\tc212_buzz_build\pwm_buzz_test.hex -prog on -ver on -start on
```

## Linker

Uses the TC212-corrected `Lcf_Gnuc_Tricore_Tc.lsl` (512 KB flash, 48 KB DSPR,
no LMU/EDmem). See `tc212-kit/README.md`.
