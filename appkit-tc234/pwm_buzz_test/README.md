# pwm_buzz_test

PWM passive-buzzer test for the Application Kit TC234 (TC23xLP_A-Step).

- **Buzzer:** passive piezo on **P33.0**, needs a 2048 Hz drive signal.
- **Peripheral:** GTM **TOM0_CH4** (TOUT22) driving P33.0.
- **Behavior:** sweeps the PWM duty cycle 0 % -> 100 % (10 % steps, 100 ms each),
  holds at 100 % for 1 s, sweeps 100 % -> 0 %, holds silent for 1 s, loops.

## Build

Standalone GCC build (no IDE, no TASKING license required):

```
powershell -ExecutionPolicy Bypass -File build_pwm_buzz_test.ps1
```

Output: `%TEMP%\pwm_buzz_test_build\pwm_buzz_test.hex`

## Flash

```
"D:\Infineon\AURIX-Studio-1.10.36\tools\AurixFlasherSoftwareTool_v3.0.18\AURIXFlasher.exe" ^
    -hex %TEMP%\pwm_buzz_test_build\pwm_buzz_test.hex -prog on -ver on -start on
```

Serial (COM6, 115200 8N1) prints a banner on start (including the die
temperature via DTS, ~36 °C); the buzzer runs immediately.

## Implementation notes

TC23A has **no GTM ATOM** — only TOM (Timer Output Module). The TC275 buzzer
example uses ATOM2; this project uses the TOM equivalent:

- `IfxGtm_Tom_Pwm` on **TOM0_CH4**, output pin `IfxGtm_TOM0_4_TOUT22_P33_0_OUT`.
- Clock: GCLK = GTM module clock (200 MHz), TOM channel uses the fixed clock
  **FXCLK1 (GCLK/16 = 12.5 MHz)**. This is required because TOM CM0/CM1 are
  **16-bit** and 200 MHz / 2048 Hz = 97656 ticks would overflow; 12.5 MHz gives
  6103 ticks.
- Runtime duty updates write **CM1 directly** (`IfxGtm_Tom_Ch_setCompareOne`);
  the PWM is configured with `synchronousUpdateEnabled = FALSE` so the direct
  write takes effect immediately instead of being overridden by the shadow
  register on the next TGC trigger.
- `IfxGtm_Cmu_enableClocks(gtm, IFXGTM_CMU_CLKEN_FXCLK)` is required so the
  fixed clock used by the TOM channel is running.

## Files

| File             | Purpose                                  |
|------------------|------------------------------------------|
| `buzzer.c/.h`    | TOM PWM init + duty-cycle control        |
| `Cpu0_Main.c`    | Duty sweep loop                          |
| `serial.c/.h`    | ASC0 UART (P14.0/1, 115200) for banner   |
| `dts.c/.h`       | Die Temperature Sensor readout (banner)  |
