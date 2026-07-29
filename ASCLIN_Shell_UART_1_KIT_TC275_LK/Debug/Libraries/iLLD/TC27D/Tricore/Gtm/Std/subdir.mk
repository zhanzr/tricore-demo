################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
"../Libraries/iLLD/TC27D/Tricore/Gtm/Std/IfxGtm.c" \
"../Libraries/iLLD/TC27D/Tricore/Gtm/Std/IfxGtm_Atom.c" \
"../Libraries/iLLD/TC27D/Tricore/Gtm/Std/IfxGtm_Cmu.c" \
"../Libraries/iLLD/TC27D/Tricore/Gtm/Std/IfxGtm_Dpll.c" \
"../Libraries/iLLD/TC27D/Tricore/Gtm/Std/IfxGtm_Tbu.c" \
"../Libraries/iLLD/TC27D/Tricore/Gtm/Std/IfxGtm_Tim.c" \
"../Libraries/iLLD/TC27D/Tricore/Gtm/Std/IfxGtm_Tom.c" 

COMPILED_SRCS += \
"Libraries/iLLD/TC27D/Tricore/Gtm/Std/IfxGtm.src" \
"Libraries/iLLD/TC27D/Tricore/Gtm/Std/IfxGtm_Atom.src" \
"Libraries/iLLD/TC27D/Tricore/Gtm/Std/IfxGtm_Cmu.src" \
"Libraries/iLLD/TC27D/Tricore/Gtm/Std/IfxGtm_Dpll.src" \
"Libraries/iLLD/TC27D/Tricore/Gtm/Std/IfxGtm_Tbu.src" \
"Libraries/iLLD/TC27D/Tricore/Gtm/Std/IfxGtm_Tim.src" \
"Libraries/iLLD/TC27D/Tricore/Gtm/Std/IfxGtm_Tom.src" 

C_DEPS += \
"./Libraries/iLLD/TC27D/Tricore/Gtm/Std/IfxGtm.d" \
"./Libraries/iLLD/TC27D/Tricore/Gtm/Std/IfxGtm_Atom.d" \
"./Libraries/iLLD/TC27D/Tricore/Gtm/Std/IfxGtm_Cmu.d" \
"./Libraries/iLLD/TC27D/Tricore/Gtm/Std/IfxGtm_Dpll.d" \
"./Libraries/iLLD/TC27D/Tricore/Gtm/Std/IfxGtm_Tbu.d" \
"./Libraries/iLLD/TC27D/Tricore/Gtm/Std/IfxGtm_Tim.d" \
"./Libraries/iLLD/TC27D/Tricore/Gtm/Std/IfxGtm_Tom.d" 

OBJS += \
"Libraries/iLLD/TC27D/Tricore/Gtm/Std/IfxGtm.o" \
"Libraries/iLLD/TC27D/Tricore/Gtm/Std/IfxGtm_Atom.o" \
"Libraries/iLLD/TC27D/Tricore/Gtm/Std/IfxGtm_Cmu.o" \
"Libraries/iLLD/TC27D/Tricore/Gtm/Std/IfxGtm_Dpll.o" \
"Libraries/iLLD/TC27D/Tricore/Gtm/Std/IfxGtm_Tbu.o" \
"Libraries/iLLD/TC27D/Tricore/Gtm/Std/IfxGtm_Tim.o" \
"Libraries/iLLD/TC27D/Tricore/Gtm/Std/IfxGtm_Tom.o" 


# Each subdirectory must supply rules for building sources it contributes
"Libraries/iLLD/TC27D/Tricore/Gtm/Std/IfxGtm.src":"../Libraries/iLLD/TC27D/Tricore/Gtm/Std/IfxGtm.c" "Libraries/iLLD/TC27D/Tricore/Gtm/Std/subdir.mk"
	cctc -cs --dep-file="$*.d" --misrac-version=2012 -D__CPU__=tc27xd "-fD:/aurix_1036_wks/ASCLIN_Shell_UART_1_KIT_TC275_LK/Debug/TASKING_C_C___Compiler-Include_paths__-I_.opt" --iso=99 --c++14 --language=+volatile --exceptions --anachronisms --fp-model=3 -O0 --tradeoff=2 --compact-max-size=200 -Wc-w544 -Wc-w557 -Ctc27xd -Y0 -N0 -Z0 -o "$@" "$<"
"Libraries/iLLD/TC27D/Tricore/Gtm/Std/IfxGtm.o":"Libraries/iLLD/TC27D/Tricore/Gtm/Std/IfxGtm.src" "Libraries/iLLD/TC27D/Tricore/Gtm/Std/subdir.mk"
	astc -Og -Os --no-warnings= --error-limit=42 -o  "$@" "$<"
"Libraries/iLLD/TC27D/Tricore/Gtm/Std/IfxGtm_Atom.src":"../Libraries/iLLD/TC27D/Tricore/Gtm/Std/IfxGtm_Atom.c" "Libraries/iLLD/TC27D/Tricore/Gtm/Std/subdir.mk"
	cctc -cs --dep-file="$*.d" --misrac-version=2012 -D__CPU__=tc27xd "-fD:/aurix_1036_wks/ASCLIN_Shell_UART_1_KIT_TC275_LK/Debug/TASKING_C_C___Compiler-Include_paths__-I_.opt" --iso=99 --c++14 --language=+volatile --exceptions --anachronisms --fp-model=3 -O0 --tradeoff=2 --compact-max-size=200 -Wc-w544 -Wc-w557 -Ctc27xd -Y0 -N0 -Z0 -o "$@" "$<"
"Libraries/iLLD/TC27D/Tricore/Gtm/Std/IfxGtm_Atom.o":"Libraries/iLLD/TC27D/Tricore/Gtm/Std/IfxGtm_Atom.src" "Libraries/iLLD/TC27D/Tricore/Gtm/Std/subdir.mk"
	astc -Og -Os --no-warnings= --error-limit=42 -o  "$@" "$<"
"Libraries/iLLD/TC27D/Tricore/Gtm/Std/IfxGtm_Cmu.src":"../Libraries/iLLD/TC27D/Tricore/Gtm/Std/IfxGtm_Cmu.c" "Libraries/iLLD/TC27D/Tricore/Gtm/Std/subdir.mk"
	cctc -cs --dep-file="$*.d" --misrac-version=2012 -D__CPU__=tc27xd "-fD:/aurix_1036_wks/ASCLIN_Shell_UART_1_KIT_TC275_LK/Debug/TASKING_C_C___Compiler-Include_paths__-I_.opt" --iso=99 --c++14 --language=+volatile --exceptions --anachronisms --fp-model=3 -O0 --tradeoff=2 --compact-max-size=200 -Wc-w544 -Wc-w557 -Ctc27xd -Y0 -N0 -Z0 -o "$@" "$<"
"Libraries/iLLD/TC27D/Tricore/Gtm/Std/IfxGtm_Cmu.o":"Libraries/iLLD/TC27D/Tricore/Gtm/Std/IfxGtm_Cmu.src" "Libraries/iLLD/TC27D/Tricore/Gtm/Std/subdir.mk"
	astc -Og -Os --no-warnings= --error-limit=42 -o  "$@" "$<"
"Libraries/iLLD/TC27D/Tricore/Gtm/Std/IfxGtm_Dpll.src":"../Libraries/iLLD/TC27D/Tricore/Gtm/Std/IfxGtm_Dpll.c" "Libraries/iLLD/TC27D/Tricore/Gtm/Std/subdir.mk"
	cctc -cs --dep-file="$*.d" --misrac-version=2012 -D__CPU__=tc27xd "-fD:/aurix_1036_wks/ASCLIN_Shell_UART_1_KIT_TC275_LK/Debug/TASKING_C_C___Compiler-Include_paths__-I_.opt" --iso=99 --c++14 --language=+volatile --exceptions --anachronisms --fp-model=3 -O0 --tradeoff=2 --compact-max-size=200 -Wc-w544 -Wc-w557 -Ctc27xd -Y0 -N0 -Z0 -o "$@" "$<"
"Libraries/iLLD/TC27D/Tricore/Gtm/Std/IfxGtm_Dpll.o":"Libraries/iLLD/TC27D/Tricore/Gtm/Std/IfxGtm_Dpll.src" "Libraries/iLLD/TC27D/Tricore/Gtm/Std/subdir.mk"
	astc -Og -Os --no-warnings= --error-limit=42 -o  "$@" "$<"
"Libraries/iLLD/TC27D/Tricore/Gtm/Std/IfxGtm_Tbu.src":"../Libraries/iLLD/TC27D/Tricore/Gtm/Std/IfxGtm_Tbu.c" "Libraries/iLLD/TC27D/Tricore/Gtm/Std/subdir.mk"
	cctc -cs --dep-file="$*.d" --misrac-version=2012 -D__CPU__=tc27xd "-fD:/aurix_1036_wks/ASCLIN_Shell_UART_1_KIT_TC275_LK/Debug/TASKING_C_C___Compiler-Include_paths__-I_.opt" --iso=99 --c++14 --language=+volatile --exceptions --anachronisms --fp-model=3 -O0 --tradeoff=2 --compact-max-size=200 -Wc-w544 -Wc-w557 -Ctc27xd -Y0 -N0 -Z0 -o "$@" "$<"
"Libraries/iLLD/TC27D/Tricore/Gtm/Std/IfxGtm_Tbu.o":"Libraries/iLLD/TC27D/Tricore/Gtm/Std/IfxGtm_Tbu.src" "Libraries/iLLD/TC27D/Tricore/Gtm/Std/subdir.mk"
	astc -Og -Os --no-warnings= --error-limit=42 -o  "$@" "$<"
"Libraries/iLLD/TC27D/Tricore/Gtm/Std/IfxGtm_Tim.src":"../Libraries/iLLD/TC27D/Tricore/Gtm/Std/IfxGtm_Tim.c" "Libraries/iLLD/TC27D/Tricore/Gtm/Std/subdir.mk"
	cctc -cs --dep-file="$*.d" --misrac-version=2012 -D__CPU__=tc27xd "-fD:/aurix_1036_wks/ASCLIN_Shell_UART_1_KIT_TC275_LK/Debug/TASKING_C_C___Compiler-Include_paths__-I_.opt" --iso=99 --c++14 --language=+volatile --exceptions --anachronisms --fp-model=3 -O0 --tradeoff=2 --compact-max-size=200 -Wc-w544 -Wc-w557 -Ctc27xd -Y0 -N0 -Z0 -o "$@" "$<"
"Libraries/iLLD/TC27D/Tricore/Gtm/Std/IfxGtm_Tim.o":"Libraries/iLLD/TC27D/Tricore/Gtm/Std/IfxGtm_Tim.src" "Libraries/iLLD/TC27D/Tricore/Gtm/Std/subdir.mk"
	astc -Og -Os --no-warnings= --error-limit=42 -o  "$@" "$<"
"Libraries/iLLD/TC27D/Tricore/Gtm/Std/IfxGtm_Tom.src":"../Libraries/iLLD/TC27D/Tricore/Gtm/Std/IfxGtm_Tom.c" "Libraries/iLLD/TC27D/Tricore/Gtm/Std/subdir.mk"
	cctc -cs --dep-file="$*.d" --misrac-version=2012 -D__CPU__=tc27xd "-fD:/aurix_1036_wks/ASCLIN_Shell_UART_1_KIT_TC275_LK/Debug/TASKING_C_C___Compiler-Include_paths__-I_.opt" --iso=99 --c++14 --language=+volatile --exceptions --anachronisms --fp-model=3 -O0 --tradeoff=2 --compact-max-size=200 -Wc-w544 -Wc-w557 -Ctc27xd -Y0 -N0 -Z0 -o "$@" "$<"
"Libraries/iLLD/TC27D/Tricore/Gtm/Std/IfxGtm_Tom.o":"Libraries/iLLD/TC27D/Tricore/Gtm/Std/IfxGtm_Tom.src" "Libraries/iLLD/TC27D/Tricore/Gtm/Std/subdir.mk"
	astc -Og -Os --no-warnings= --error-limit=42 -o  "$@" "$<"

clean: clean-Libraries-2f-iLLD-2f-TC27D-2f-Tricore-2f-Gtm-2f-Std

clean-Libraries-2f-iLLD-2f-TC27D-2f-Tricore-2f-Gtm-2f-Std:
	-$(RM) ./Libraries/iLLD/TC27D/Tricore/Gtm/Std/IfxGtm.d ./Libraries/iLLD/TC27D/Tricore/Gtm/Std/IfxGtm.o ./Libraries/iLLD/TC27D/Tricore/Gtm/Std/IfxGtm.src ./Libraries/iLLD/TC27D/Tricore/Gtm/Std/IfxGtm_Atom.d ./Libraries/iLLD/TC27D/Tricore/Gtm/Std/IfxGtm_Atom.o ./Libraries/iLLD/TC27D/Tricore/Gtm/Std/IfxGtm_Atom.src ./Libraries/iLLD/TC27D/Tricore/Gtm/Std/IfxGtm_Cmu.d ./Libraries/iLLD/TC27D/Tricore/Gtm/Std/IfxGtm_Cmu.o ./Libraries/iLLD/TC27D/Tricore/Gtm/Std/IfxGtm_Cmu.src ./Libraries/iLLD/TC27D/Tricore/Gtm/Std/IfxGtm_Dpll.d ./Libraries/iLLD/TC27D/Tricore/Gtm/Std/IfxGtm_Dpll.o ./Libraries/iLLD/TC27D/Tricore/Gtm/Std/IfxGtm_Dpll.src ./Libraries/iLLD/TC27D/Tricore/Gtm/Std/IfxGtm_Tbu.d ./Libraries/iLLD/TC27D/Tricore/Gtm/Std/IfxGtm_Tbu.o ./Libraries/iLLD/TC27D/Tricore/Gtm/Std/IfxGtm_Tbu.src ./Libraries/iLLD/TC27D/Tricore/Gtm/Std/IfxGtm_Tim.d ./Libraries/iLLD/TC27D/Tricore/Gtm/Std/IfxGtm_Tim.o ./Libraries/iLLD/TC27D/Tricore/Gtm/Std/IfxGtm_Tim.src ./Libraries/iLLD/TC27D/Tricore/Gtm/Std/IfxGtm_Tom.d ./Libraries/iLLD/TC27D/Tricore/Gtm/Std/IfxGtm_Tom.o ./Libraries/iLLD/TC27D/Tricore/Gtm/Std/IfxGtm_Tom.src

.PHONY: clean-Libraries-2f-iLLD-2f-TC27D-2f-Tricore-2f-Gtm-2f-Std

