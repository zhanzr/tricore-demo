################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
"../Libraries/Service/CpuGeneric/StdIf/IfxStdIf_DPipe.c" \
"../Libraries/Service/CpuGeneric/StdIf/IfxStdIf_Pos.c" \
"../Libraries/Service/CpuGeneric/StdIf/IfxStdIf_PwmHl.c" \
"../Libraries/Service/CpuGeneric/StdIf/IfxStdIf_Timer.c" 

COMPILED_SRCS += \
"Libraries/Service/CpuGeneric/StdIf/IfxStdIf_DPipe.src" \
"Libraries/Service/CpuGeneric/StdIf/IfxStdIf_Pos.src" \
"Libraries/Service/CpuGeneric/StdIf/IfxStdIf_PwmHl.src" \
"Libraries/Service/CpuGeneric/StdIf/IfxStdIf_Timer.src" 

C_DEPS += \
"./Libraries/Service/CpuGeneric/StdIf/IfxStdIf_DPipe.d" \
"./Libraries/Service/CpuGeneric/StdIf/IfxStdIf_Pos.d" \
"./Libraries/Service/CpuGeneric/StdIf/IfxStdIf_PwmHl.d" \
"./Libraries/Service/CpuGeneric/StdIf/IfxStdIf_Timer.d" 

OBJS += \
"Libraries/Service/CpuGeneric/StdIf/IfxStdIf_DPipe.o" \
"Libraries/Service/CpuGeneric/StdIf/IfxStdIf_Pos.o" \
"Libraries/Service/CpuGeneric/StdIf/IfxStdIf_PwmHl.o" \
"Libraries/Service/CpuGeneric/StdIf/IfxStdIf_Timer.o" 


# Each subdirectory must supply rules for building sources it contributes
"Libraries/Service/CpuGeneric/StdIf/IfxStdIf_DPipe.src":"../Libraries/Service/CpuGeneric/StdIf/IfxStdIf_DPipe.c" "Libraries/Service/CpuGeneric/StdIf/subdir.mk"
	cctc -cs --dep-file="$*.d" --misrac-version=2012 -D__CPU__=tc27xd "-fD:/aurix_1036_wks/UART_VCOM_1_KIT_TC275_LK/Debug/TASKING_C_C___Compiler-Include_paths__-I_.opt" --iso=99 --c++14 --language=+volatile --exceptions --anachronisms --fp-model=3 -O0 --tradeoff=4 --compact-max-size=200 -g -Wc-w544 -Wc-w557 -Ctc27xd -Y0 -N0 -Z0 -o "$@" "$<"
"Libraries/Service/CpuGeneric/StdIf/IfxStdIf_DPipe.o":"Libraries/Service/CpuGeneric/StdIf/IfxStdIf_DPipe.src" "Libraries/Service/CpuGeneric/StdIf/subdir.mk"
	astc -Og -Os --no-warnings= --error-limit=42 -o  "$@" "$<"
"Libraries/Service/CpuGeneric/StdIf/IfxStdIf_Pos.src":"../Libraries/Service/CpuGeneric/StdIf/IfxStdIf_Pos.c" "Libraries/Service/CpuGeneric/StdIf/subdir.mk"
	cctc -cs --dep-file="$*.d" --misrac-version=2012 -D__CPU__=tc27xd "-fD:/aurix_1036_wks/UART_VCOM_1_KIT_TC275_LK/Debug/TASKING_C_C___Compiler-Include_paths__-I_.opt" --iso=99 --c++14 --language=+volatile --exceptions --anachronisms --fp-model=3 -O0 --tradeoff=4 --compact-max-size=200 -g -Wc-w544 -Wc-w557 -Ctc27xd -Y0 -N0 -Z0 -o "$@" "$<"
"Libraries/Service/CpuGeneric/StdIf/IfxStdIf_Pos.o":"Libraries/Service/CpuGeneric/StdIf/IfxStdIf_Pos.src" "Libraries/Service/CpuGeneric/StdIf/subdir.mk"
	astc -Og -Os --no-warnings= --error-limit=42 -o  "$@" "$<"
"Libraries/Service/CpuGeneric/StdIf/IfxStdIf_PwmHl.src":"../Libraries/Service/CpuGeneric/StdIf/IfxStdIf_PwmHl.c" "Libraries/Service/CpuGeneric/StdIf/subdir.mk"
	cctc -cs --dep-file="$*.d" --misrac-version=2012 -D__CPU__=tc27xd "-fD:/aurix_1036_wks/UART_VCOM_1_KIT_TC275_LK/Debug/TASKING_C_C___Compiler-Include_paths__-I_.opt" --iso=99 --c++14 --language=+volatile --exceptions --anachronisms --fp-model=3 -O0 --tradeoff=4 --compact-max-size=200 -g -Wc-w544 -Wc-w557 -Ctc27xd -Y0 -N0 -Z0 -o "$@" "$<"
"Libraries/Service/CpuGeneric/StdIf/IfxStdIf_PwmHl.o":"Libraries/Service/CpuGeneric/StdIf/IfxStdIf_PwmHl.src" "Libraries/Service/CpuGeneric/StdIf/subdir.mk"
	astc -Og -Os --no-warnings= --error-limit=42 -o  "$@" "$<"
"Libraries/Service/CpuGeneric/StdIf/IfxStdIf_Timer.src":"../Libraries/Service/CpuGeneric/StdIf/IfxStdIf_Timer.c" "Libraries/Service/CpuGeneric/StdIf/subdir.mk"
	cctc -cs --dep-file="$*.d" --misrac-version=2012 -D__CPU__=tc27xd "-fD:/aurix_1036_wks/UART_VCOM_1_KIT_TC275_LK/Debug/TASKING_C_C___Compiler-Include_paths__-I_.opt" --iso=99 --c++14 --language=+volatile --exceptions --anachronisms --fp-model=3 -O0 --tradeoff=4 --compact-max-size=200 -g -Wc-w544 -Wc-w557 -Ctc27xd -Y0 -N0 -Z0 -o "$@" "$<"
"Libraries/Service/CpuGeneric/StdIf/IfxStdIf_Timer.o":"Libraries/Service/CpuGeneric/StdIf/IfxStdIf_Timer.src" "Libraries/Service/CpuGeneric/StdIf/subdir.mk"
	astc -Og -Os --no-warnings= --error-limit=42 -o  "$@" "$<"

clean: clean-Libraries-2f-Service-2f-CpuGeneric-2f-StdIf

clean-Libraries-2f-Service-2f-CpuGeneric-2f-StdIf:
	-$(RM) ./Libraries/Service/CpuGeneric/StdIf/IfxStdIf_DPipe.d ./Libraries/Service/CpuGeneric/StdIf/IfxStdIf_DPipe.o ./Libraries/Service/CpuGeneric/StdIf/IfxStdIf_DPipe.src ./Libraries/Service/CpuGeneric/StdIf/IfxStdIf_Pos.d ./Libraries/Service/CpuGeneric/StdIf/IfxStdIf_Pos.o ./Libraries/Service/CpuGeneric/StdIf/IfxStdIf_Pos.src ./Libraries/Service/CpuGeneric/StdIf/IfxStdIf_PwmHl.d ./Libraries/Service/CpuGeneric/StdIf/IfxStdIf_PwmHl.o ./Libraries/Service/CpuGeneric/StdIf/IfxStdIf_PwmHl.src ./Libraries/Service/CpuGeneric/StdIf/IfxStdIf_Timer.d ./Libraries/Service/CpuGeneric/StdIf/IfxStdIf_Timer.o ./Libraries/Service/CpuGeneric/StdIf/IfxStdIf_Timer.src

.PHONY: clean-Libraries-2f-Service-2f-CpuGeneric-2f-StdIf

