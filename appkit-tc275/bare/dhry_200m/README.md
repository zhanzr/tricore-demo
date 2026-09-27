# TC275 Application Kit - Dhrystone 2.1

Tricore 1.6(E, P, P) 200 MHz, 3 cores (0,1,2), 2,000,000 runs, `register` vars.

## Results: TASKING vs GCC

| Core | TASKING (IDE, -O3) | GCC (CLI, `-Ofast -ffp-contract=fast -funroll-loops`) |
|------|--------------------|----------------|
| CPU0 | 183318 (0.522 DMIPS/MHz) | **205128** (0.584 DMIPS/MHz) |
| CPU1 | **333890** (0.950 DMIPS/MHz) | 315457 (0.898 DMIPS/MHz) |
| CPU2 | 245700 (0.699 DMIPS/MHz) | **276243** (0.786 DMIPS/MHz) |

All runs produced correct final values. The current GCC flags match the
`nucleo-u575` `dhry_160m` reference (`arm-none-eabi-gcc`); re-measuring with
them reproduced the previous GCC -O3 numbers exactly (integer workload,
unaffected by the FP-related flag delta).

GCC is faster on CPU0/CPU2; CPU1's TASKING run shows a higher number — the
cores share the GTM/bus, so results vary with the token-handoff timing.

## GCC (-Ofast) raw output

```
Dhrystone Benchmark, Version 2.1 (Language: C)

Execution starts, 2000000 runs through Dhrystone
Execution ends

MicroSecond for one run through Dhrystone[76-9826]:	 4.875
Dhrystones per Second:	205128.203
DMIPS/MHz:	0.584
core:0 ends

MicroSecond for one run through Dhrystone[15538-21878]:	 3.170
Dhrystones per Second:	315457.406
DMIPS/MHz:	0.898
core:1 ends

MicroSecond for one run through Dhrystone[27592-34832]:	 3.620
Dhrystones per Second:	276243.094
DMIPS/MHz:	0.786
core:2 ends
```

## TASKING raw output

```
TASKING VX-toolset for AURIX Development Studio (non-commercial): control program   v1.1r8 Build 22011964

Execution starts, 2000000 runs through Dhrystone

single core(core 0)
-O3

MicroSecond for one run through Dhrystone[10-10920]:     5.455
Dhrystones per Second:  183318.100
DMIPS/MHz:      0.522

3 cores(0,1,2)
-O3

MicroSecond for one run through Dhrystone[10-11240]:     5.615
Dhrystones per Second:  178094.400
DMIPS/MHz:      0.507

MicroSecond for one run through Dhrystone[16854-22844]:  2.995
Dhrystones per Second:  333889.800
DMIPS/MHz:      0.950

MicroSecond for one run through Dhrystone[28459-36599]:  4.070
Dhrystones per Second:  245700.300
DMIPS/MHz:      0.699
```
