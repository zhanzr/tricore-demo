################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
"../Libraries/iLLD/TC27D/Tricore/Cpu/Trap/IfxCpu_Trap.c" 

COMPILED_SRCS += \
"Libraries/iLLD/TC27D/Tricore/Cpu/Trap/IfxCpu_Trap.src" 

C_DEPS += \
"./Libraries/iLLD/TC27D/Tricore/Cpu/Trap/IfxCpu_Trap.d" 

OBJS += \
"Libraries/iLLD/TC27D/Tricore/Cpu/Trap/IfxCpu_Trap.o" 


# Each subdirectory must supply rules for building sources it contributes
"Libraries/iLLD/TC27D/Tricore/Cpu/Trap/IfxCpu_Trap.src":"../Libraries/iLLD/TC27D/Tricore/Cpu/Trap/IfxCpu_Trap.c" "Libraries/iLLD/TC27D/Tricore/Cpu/Trap/subdir.mk"
	cctc -cs --dep-file="$*.d" --misrac-version=2012 -D__CPU__=tc27xd "-fD:/aurix_1036_wks/ASCLIN_Shell_UART_1_KIT_TC275_LK/Debug/TASKING_C_C___Compiler-Include_paths__-I_.opt" --iso=99 --c++14 --language=+volatile --exceptions --anachronisms --fp-model=3 -O0 --tradeoff=2 --compact-max-size=200 -Wc-w544 -Wc-w557 -Ctc27xd -Y0 -N0 -Z0 -o "$@" "$<"
"Libraries/iLLD/TC27D/Tricore/Cpu/Trap/IfxCpu_Trap.o":"Libraries/iLLD/TC27D/Tricore/Cpu/Trap/IfxCpu_Trap.src" "Libraries/iLLD/TC27D/Tricore/Cpu/Trap/subdir.mk"
	astc -Og -Os --no-warnings= --error-limit=42 -o  "$@" "$<"

clean: clean-Libraries-2f-iLLD-2f-TC27D-2f-Tricore-2f-Cpu-2f-Trap

clean-Libraries-2f-iLLD-2f-TC27D-2f-Tricore-2f-Cpu-2f-Trap:
	-$(RM) ./Libraries/iLLD/TC27D/Tricore/Cpu/Trap/IfxCpu_Trap.d ./Libraries/iLLD/TC27D/Tricore/Cpu/Trap/IfxCpu_Trap.o ./Libraries/iLLD/TC27D/Tricore/Cpu/Trap/IfxCpu_Trap.src

.PHONY: clean-Libraries-2f-iLLD-2f-TC27D-2f-Tricore-2f-Cpu-2f-Trap

