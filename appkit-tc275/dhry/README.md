# TC275 Application Kit Demo
Using Aurix IDE.

Tricore 1.6(E, P, P) 200 MHz
## dhrystone

### Flash Single core
TASKING VX-toolset for AURIX Development Studio (non-commercial): control program   v1.1r8 Build 22011964

Execution starts, 2000000 runs through Dhrystone

single core(core 0)
```
-O3

MicroSecond for one run through Dhrystone[10-10920]:     5.455
Dhrystones per Second:  183318.100
DMIPS/MHz:      0.522
```

3 cores(0,1,2)
```
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
