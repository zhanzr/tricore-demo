# blink_hello (tc212-kit)

**TC212 Application Kit** - first bring-up demo.

- Device: **TC21x/TC22x** (`DEVICE-ID: TC22x` family), TriCore 1.6.1, **133 MHz**
- Verified silicon CHIPID: `CHID=12 CHREV=1 FSIZE=1` (**TC212, 512 KB flash**)
- Serial: **ASC0** on P15.2 (TX) / P15.3 (RX), 115200 8N1

## What it does

- 8 LEDs: **P02.0, P02.1, P02.2, P02.3, P02.4, P02.5, P11.10, P11.11** (low active).
- Rotating LED blink (200 ms per LED).
- Prints every 2 s over ASC0 UART:
  `TC212 blink CPU=133.33 MHz SPB=66.66 MHz Die=39.07 C AN18=2.556 V (raw 3172)`
- DTS (die temperature) read via `IfxDts_Dts`.
- **AN18 ADC input**: board `AN18` -> **VADC G1CH6** on **P41.6**, read each
  report as a 12-bit value scaled against `ADC_VREF_MV` (3.3 V) in `adc.c`.

## UART pins

Board wiring (FT2232HL Channel B):
- **BDBUS1 (TXD) -> P15.2** - MCU TX (ASC0 ATX0)
- **BDBUS0 (RXD) -> P15.3** - MCU RX (ASC0 ARX0B)

The alternate P14.0/P14.1 paths have `0R_DNA` (do-not-assemble) resistors and
are not populated. **Combo 1 (P15.2/P15.3) is the correct wiring.**

The miniWiggler is a **combined device**: one USB exposes both the debugger
(DAP/JTAG) and a **virtual serial port** for the target UART. Open the virtual
COM at **115200 8N1**.

## Build / Flash

From a bash shell (MSYS2 `C:\msys64\usr\bin\bash.exe` preferred, else Git for
Windows bash); `TRICORE_GCC` defaults to the AURIX GCC install. **Run from this
project folder:**

```
make hex      # -> build/blink_hello.hex
make flash    # programs build/blink_hello.hex via AURIXFlasher
make clean
```

Equivalent direct flash command:

```
"D:\Infineon\AURIX-Studio-1.10.36\tools\AurixFlasherSoftwareTool_v3.0.18\AURIXFlasher.exe" ^
    -hex build\blink_hello.hex -prog on -ver on -start on
```

## Linker note

TC212 has **512 KB program flash** (0x80000000-0x80080000) and **48 KB DSPR**,
and **no LMU / EDmem**. The bundled TC2xx linker defaults (1 MB flash,
LCF_INTVEC0_START=0x800F4000, 88 KB DSPR, LMU/EDmem regions) overflowed.
`Lcf_Gnuc_Tricore_Tc.lsl` was corrected for the TC212: intvec at 0x80070000,
48 KB DSPR, LMU/EDmem regions removed, `__A9_MEM` aliased to `_SMALL_DATA3_`
(the CStart SDA4 init references it even though TC212 has no LMU).

See `tc212-kit/README.md` for the shared build/flash notes and all projects.
