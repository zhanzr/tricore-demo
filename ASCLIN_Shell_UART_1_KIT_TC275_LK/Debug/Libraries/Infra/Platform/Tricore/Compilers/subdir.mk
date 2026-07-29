################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
"../Libraries/Infra/Platform/Tricore/Compilers/CompilerDcc.c" \
"../Libraries/Infra/Platform/Tricore/Compilers/CompilerGhs.c" \
"../Libraries/Infra/Platform/Tricore/Compilers/CompilerGnuc.c" \
"../Libraries/Infra/Platform/Tricore/Compilers/CompilerTasking.c" 

COMPILED_SRCS += \
"Libraries/Infra/Platform/Tricore/Compilers/CompilerDcc.src" \
"Libraries/Infra/Platform/Tricore/Compilers/CompilerGhs.src" \
"Libraries/Infra/Platform/Tricore/Compilers/CompilerGnuc.src" \
"Libraries/Infra/Platform/Tricore/Compilers/CompilerTasking.src" 

C_DEPS += \
"./Libraries/Infra/Platform/Tricore/Compilers/CompilerDcc.d" \
"./Libraries/Infra/Platform/Tricore/Compilers/CompilerGhs.d" \
"./Libraries/Infra/Platform/Tricore/Compilers/CompilerGnuc.d" \
"./Libraries/Infra/Platform/Tricore/Compilers/CompilerTasking.d" 

OBJS += \
"Libraries/Infra/Platform/Tricore/Compilers/CompilerDcc.o" \
"Libraries/Infra/Platform/Tricore/Compilers/CompilerGhs.o" \
"Libraries/Infra/Platform/Tricore/Compilers/CompilerGnuc.o" \
"Libraries/Infra/Platform/Tricore/Compilers/CompilerTasking.o" 


# Each subdirectory must supply rules for building sources it contributes
"Libraries/Infra/Platform/Tricore/Compilers/CompilerDcc.src":"../Libraries/Infra/Platform/Tricore/Compilers/CompilerDcc.c" "Libraries/Infra/Platform/Tricore/Compilers/subdir.mk"
	cctc -cs --dep-file="$*.d" --misrac-version=2012 -D__CPU__=tc27xd "-fD:/aurix_1036_wks/ASCLIN_Shell_UART_1_KIT_TC275_LK/Debug/TASKING_C_C___Compiler-Include_paths__-I_.opt" --iso=99 --c++14 --language=+volatile --exceptions --anachronisms --fp-model=3 -O0 --tradeoff=2 --compact-max-size=200 -Wc-w544 -Wc-w557 -Ctc27xd -Y0 -N0 -Z0 -o "$@" "$<"
"Libraries/Infra/Platform/Tricore/Compilers/CompilerDcc.o":"Libraries/Infra/Platform/Tricore/Compilers/CompilerDcc.src" "Libraries/Infra/Platform/Tricore/Compilers/subdir.mk"
	astc -Og -Os --no-warnings= --error-limit=42 -o  "$@" "$<"
"Libraries/Infra/Platform/Tricore/Compilers/CompilerGhs.src":"../Libraries/Infra/Platform/Tricore/Compilers/CompilerGhs.c" "Libraries/Infra/Platform/Tricore/Compilers/subdir.mk"
	cctc -cs --dep-file="$*.d" --misrac-version=2012 -D__CPU__=tc27xd "-fD:/aurix_1036_wks/ASCLIN_Shell_UART_1_KIT_TC275_LK/Debug/TASKING_C_C___Compiler-Include_paths__-I_.opt" --iso=99 --c++14 --language=+volatile --exceptions --anachronisms --fp-model=3 -O0 --tradeoff=2 --compact-max-size=200 -Wc-w544 -Wc-w557 -Ctc27xd -Y0 -N0 -Z0 -o "$@" "$<"
"Libraries/Infra/Platform/Tricore/Compilers/CompilerGhs.o":"Libraries/Infra/Platform/Tricore/Compilers/CompilerGhs.src" "Libraries/Infra/Platform/Tricore/Compilers/subdir.mk"
	astc -Og -Os --no-warnings= --error-limit=42 -o  "$@" "$<"
"Libraries/Infra/Platform/Tricore/Compilers/CompilerGnuc.src":"../Libraries/Infra/Platform/Tricore/Compilers/CompilerGnuc.c" "Libraries/Infra/Platform/Tricore/Compilers/subdir.mk"
	cctc -cs --dep-file="$*.d" --misrac-version=2012 -D__CPU__=tc27xd "-fD:/aurix_1036_wks/ASCLIN_Shell_UART_1_KIT_TC275_LK/Debug/TASKING_C_C___Compiler-Include_paths__-I_.opt" --iso=99 --c++14 --language=+volatile --exceptions --anachronisms --fp-model=3 -O0 --tradeoff=2 --compact-max-size=200 -Wc-w544 -Wc-w557 -Ctc27xd -Y0 -N0 -Z0 -o "$@" "$<"
"Libraries/Infra/Platform/Tricore/Compilers/CompilerGnuc.o":"Libraries/Infra/Platform/Tricore/Compilers/CompilerGnuc.src" "Libraries/Infra/Platform/Tricore/Compilers/subdir.mk"
	astc -Og -Os --no-warnings= --error-limit=42 -o  "$@" "$<"
"Libraries/Infra/Platform/Tricore/Compilers/CompilerTasking.src":"../Libraries/Infra/Platform/Tricore/Compilers/CompilerTasking.c" "Libraries/Infra/Platform/Tricore/Compilers/subdir.mk"
	cctc -cs --dep-file="$*.d" --misrac-version=2012 -D__CPU__=tc27xd "-fD:/aurix_1036_wks/ASCLIN_Shell_UART_1_KIT_TC275_LK/Debug/TASKING_C_C___Compiler-Include_paths__-I_.opt" --iso=99 --c++14 --language=+volatile --exceptions --anachronisms --fp-model=3 -O0 --tradeoff=2 --compact-max-size=200 -Wc-w544 -Wc-w557 -Ctc27xd -Y0 -N0 -Z0 -o "$@" "$<"
"Libraries/Infra/Platform/Tricore/Compilers/CompilerTasking.o":"Libraries/Infra/Platform/Tricore/Compilers/CompilerTasking.src" "Libraries/Infra/Platform/Tricore/Compilers/subdir.mk"
	astc -Og -Os --no-warnings= --error-limit=42 -o  "$@" "$<"

clean: clean-Libraries-2f-Infra-2f-Platform-2f-Tricore-2f-Compilers

clean-Libraries-2f-Infra-2f-Platform-2f-Tricore-2f-Compilers:
	-$(RM) ./Libraries/Infra/Platform/Tricore/Compilers/CompilerDcc.d ./Libraries/Infra/Platform/Tricore/Compilers/CompilerDcc.o ./Libraries/Infra/Platform/Tricore/Compilers/CompilerDcc.src ./Libraries/Infra/Platform/Tricore/Compilers/CompilerGhs.d ./Libraries/Infra/Platform/Tricore/Compilers/CompilerGhs.o ./Libraries/Infra/Platform/Tricore/Compilers/CompilerGhs.src ./Libraries/Infra/Platform/Tricore/Compilers/CompilerGnuc.d ./Libraries/Infra/Platform/Tricore/Compilers/CompilerGnuc.o ./Libraries/Infra/Platform/Tricore/Compilers/CompilerGnuc.src ./Libraries/Infra/Platform/Tricore/Compilers/CompilerTasking.d ./Libraries/Infra/Platform/Tricore/Compilers/CompilerTasking.o ./Libraries/Infra/Platform/Tricore/Compilers/CompilerTasking.src

.PHONY: clean-Libraries-2f-Infra-2f-Platform-2f-Tricore-2f-Compilers

