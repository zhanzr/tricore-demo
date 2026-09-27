# TC275 Application Kit - CoreMark 1.0.1

Tricore 1.6(E, P, P) 200 MHz, 3 cores (0,1,2), 8000 iterations, 2K run, static.

## Results: TASKING vs GCC

| Core | TASKING (IDE, -O3) | GCC (CLI, `-Ofast -ffp-contract=fast -funroll-all-loops`) |
|------|--------------------|----------------|
| CPU0 | 248.5  (32.19 s)  | **300.7** (26.61 s) |
| CPU1 | 417.1  (19.18 s)  | **511.5** (15.64 s) |
| CPU2 | 262.5  (30.48 s)  | **348.3** (22.97 s) |

All runs validated (`crcfinal 0x5275`, "Correct operation validated"). The
current GCC flags match the `nucleo-u575` `coremark_160m` reference
(`arm-none-eabi-gcc`); vs the previous GCC -O3 build, CPU0/CPU1 improved
marginally and CPU2 gained ~5.8%.

## GCC (-Ofast) raw output

```
2K performance run parameters for coremark.
CoreMark Size    : 666
Total ticks      : 26607
Total time (secs): 26.607000
Iterations/Sec   : 300.672755
Iterations       : 8000
Compiler flags   : -Ofast -ffp-contract=fast -funroll-all-loops
Memory location  : Static
seedcrc          : 0xe9f5
[0]crclist       : 0xe714
[0]crcmatrix     : 0x1fd7
[0]crcstate      : 0x8e3a
[0]crcfinal      : 0x5275
Correct operation validated. See readme.txt for run and reporting rules.
CoreMark 1.0 : 300.672755 / -Ofast -ffp-contract=fast -funroll-all-loops / Static

2K performance run parameters for coremark.
CoreMark Size    : 666
Total ticks      : 15640
Total time (secs): 15.640000
Iterations/Sec   : 511.508951
Iterations       : 8000
Compiler flags   : -Ofast -ffp-contract=fast -funroll-all-loops
Memory location  : Static
seedcrc          : 0xe9f5
[0]crclist       : 0xe714
[0]crcmatrix     : 0x1fd7
[0]crcstate      : 0x8e3a
[0]crcfinal      : 0x5275
Correct operation validated. See readme.txt for run and reporting rules.
CoreMark 1.0 : 511.508951 / -Ofast -ffp-contract=fast -funroll-all-loops / Static

2K performance run parameters for coremark.
CoreMark Size    : 666
Total ticks      : 22969
Total time (secs): 22.969000
Iterations/Sec   : 348.295529
Iterations       : 8000
Compiler flags   : -Ofast -ffp-contract=fast -funroll-all-loops
Memory location  : Static
seedcrc          : 0xe9f5
[0]crclist       : 0xe714
[0]crcmatrix     : 0x1fd7
[0]crcstate      : 0x8e3a
[0]crcfinal      : 0x5275
Correct operation validated. See readme.txt for run and reporting rules.
CoreMark 1.0 : 348.295529 / -Ofast -ffp-contract=fast -funroll-all-loops / Static
```

## TASKING raw output

```
TASKING VX-toolset for AURIX Development Studio (non-commercial): control program   v1.1r8 Build 22011964

2K performance run parameters for coremark.
CoreMark Size    : 666
Total ticks      : 32194
Total time (secs): 32.194000
Iterations/Sec   : 248.493500
Iterations       : 8000
Compiler flags   : -O3
Memory location  : Static
seedcrc          : 0xe9f5
[0]crclist       : 0xe714
[0]crcmatrix     : 0x1fd7
[0]crcstate      : 0x8e3a
[0]crcfinal      : 0x5275
Correct operation validated. See readme.txt for run and reporting rules.
CoreMark 1.0 : 248.493500 / -O3 / Static

2K performance run parameters for coremark.
CoreMark Size    : 666
Total ticks      : 19180
Total time (secs): 19.180000
Iterations/Sec   : 417.101100
Iterations       : 8000
Compiler flags   : -O3
Memory location  : Static
seedcrc          : 0xe9f5
[0]crclist       : 0xe714
[0]crcmatrix     : 0x1fd7
[0]crcstate      : 0x8e3a
[0]crcfinal      : 0x5275
Correct operation validated. See readme.txt for run and reporting rules.
CoreMark 1.0 : 417.101100 / -O3 / Static

2K performance run parameters for coremark.
CoreMark Size    : 666
Total ticks      : 30476
Total time (secs): 30.476000
Iterations/Sec   : 262.501600
Iterations       : 8000
Compiler flags   : -O3
Memory location  : Static
seedcrc          : 0xe9f5
[0]crclist       : 0xe714
[0]crcmatrix     : 0x1fd7
[0]crcstate      : 0x8e3a
[0]crcfinal      : 0x5275
Correct operation validated. See readme.txt for run and reporting rules.
CoreMark 1.0 : 262.501600 / -O3 / Static
```
