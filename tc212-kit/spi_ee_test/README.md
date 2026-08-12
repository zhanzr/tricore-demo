# spi_ee_test (tc212-kit)

**AT25128N** (128 Kbit = 16 KiB) SPI EEPROM erase/program/read speed test on the
TC212, driven by the QSPI0 SPI master.

## Wiring (QSPI0)

| Signal | Pin    | QSPI0 mapping                          |
|--------|--------|----------------------------------------|
| CS     | P33.5  | `IfxQspi0_SLSO7_P33_5_OUT` (SLSO7)     |
| SCLK   | P20.11 | `IfxQspi0_SCLK_P20_11_OUT`             |
| MOSI   | P20.14 | `IfxQspi0_MTSR_P20_14_OUT` (MTSR)      |
| MISO   | P20.12 | `IfxQspi0_MRSTA_P20_12_IN` (MRST)      |

**SPI mode 0** (CPOL=0 idle low, **CPH=0** via
`shiftTransmitDataOnTrailingEdge`), **2 MHz**, **software CS** (`autoCS=FALSE`,
the driver toggles P33.5 as a GPIO).

## Gotchas discovered

- The iLLD QSPI SpiMaster default `shiftClock = shiftTransmitDataOnLeadingEdge`
  maps to **CPH=1 (SPI mode 1)** -> every byte came back shifted
  (`0xA0..0xA7` read as `0x41,0x43,0x45,..`). AT25128N needs **mode 0**.
- **Hardware autoCS (SLSO7) does not select the EEPROM reliably** on this
  setup: with `autoCS=TRUE` the device never responds (reads all 0x00,
  WREN never sets WEL). Software CS (`autoCS=FALSE`) works.
- **8 MHz is unreliable** (WRITE never triggers a program cycle, verify fails).
  2 MHz is fully reliable; write speed is EEPROM-bound anyway.

## What it measures

- **erase**: writes 0xFF to all 256 pages (64 B/page), 16 KiB total
- **program**: writes an incrementing byte pattern to all 16 KiB
- **read**: reads back all 16 KiB and verifies it matches the pattern

Timing uses the 64-bit STM0 counter (us resolution), reported over ASC0 UART
(115200 8N1, P15.2/P15.3).

## Result (measured, 2 MHz)

```
TC212 AT25128N SPI EEPROM test, CPU=133.33 MHz, Die=40.00 C
QSPI0 2 MHz: CS=P33.5 SCLK=P20.11 MOSI=P20.14 MISO=P20.12
STATUS: WIP=0 WEL=0
erase (write 0xFF)         16384 B:  570.184 ms,    0.029 MB/s
  avg 2227.28 us/page, 34.8 us/byte
program (pattern)          16384 B:  570.803 ms,    0.029 MB/s
  avg 2229.70 us/page, 34.8 us/byte
read                       16384 B:   70.275 ms,    0.233 MB/s
verify: OK, all 16384 bytes match
```

- Write throughput is dominated by the EEPROM page program time
  (~2.23 ms/page) -> **~29 KB/s**; the SPI clock hardly matters for writes.
- Read throughput is SPI-bound at 2 MHz -> **~0.23 MB/s**.

## Build / Flash

From a bash shell (MSYS2 `C:\msys64\usr\bin\bash.exe` preferred); run in the
project folder:

```
cd tc212-kit/spi_ee_test
make hex      # -> build/spi_ee_test.hex
make flash    # programs build/spi_ee_test.hex via AURIXFlasher
```
