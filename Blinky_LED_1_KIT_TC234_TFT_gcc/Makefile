# Blinky_LED_1_KIT_TC234_TFT_gcc - TC234 (TC23A) build/flash
#
# GCC/CLI port of the official ADS example Blinky_LED_1_KIT_TC234_TFT
# (TASKING/IDE), used as a boot baseline: the official example boots and
# blinks D107 on this board, this port blinks all 4 user LEDs.
#
# Targets:   make            build build/Blinky_LED_1_KIT_TC234_TFT_gcc.elf
#            make hex        build build/Blinky_LED_1_KIT_TC234_TFT_gcc.hex
#            make flash      program the hex via AURIXFlasher
#            make size       print section sizes
#            make clean      remove build/
#
# NOTE: LIBS points at the ORIGINAL example's own Libraries copy so this
# baseline is 100% faithful to the vendor project.

PROJ := Blinky_LED_1_KIT_TC234_TFT_gcc

# Bare `make` builds the ELF (not the first generated object rule).
.DEFAULT_GOAL := all

# --- Toolchain ---
TRICORE_GCC  ?= D:/aurixgcc_03_2026/bin/tricore-elf-gcc
CC           := $(TRICORE_GCC)
OBJCOPY      := $(patsubst %-gcc,%-objcopy,$(TRICORE_GCC))
SIZE         := $(patsubst %-gcc,%-size,$(TRICORE_GCC))
AURIX_FLASHER ?= D:/Infineon/AURIX-Studio-1.10.36/tools/AurixFlasherSoftwareTool_v3.0.18/AURIXFlasher.exe

# --- Paths ---
ROOT  := $(CURDIR)
LIBS  := $(abspath $(ROOT)/../Blinky_LED_1_KIT_TC234_TFT/Libraries)
BUILD := build
ARCH  := tc23xx

# --- Include paths ---
INCLUDES := \
    -I$(ROOT)/Configurations \
    -I$(LIBS) \
    -I$(LIBS)/iLLD \
    -I$(LIBS)/iLLD/TC23A \
    -I$(LIBS)/iLLD/TC23A/Tricore \
    -I$(LIBS)/iLLD/TC23A/Tricore/_Impl \
    -I$(LIBS)/iLLD/TC23A/Tricore/_Lib \
    -I$(LIBS)/iLLD/TC23A/Tricore/_Lib/DataHandling \
    -I$(LIBS)/iLLD/TC23A/Tricore/_Lib/InternalMux \
    -I$(LIBS)/iLLD/TC23A/Tricore/_PinMap \
    -I$(LIBS)/iLLD/TC23A/Tricore/Asclin \
    -I$(LIBS)/iLLD/TC23A/Tricore/Asclin/Asc \
    -I$(LIBS)/iLLD/TC23A/Tricore/Asclin/Lin \
    -I$(LIBS)/iLLD/TC23A/Tricore/Asclin/Spi \
    -I$(LIBS)/iLLD/TC23A/Tricore/Asclin/Std \
    -I$(LIBS)/iLLD/TC23A/Tricore/Ccu6 \
    -I$(LIBS)/iLLD/TC23A/Tricore/Ccu6/Icu \
    -I$(LIBS)/iLLD/TC23A/Tricore/Ccu6/PwmBc \
    -I$(LIBS)/iLLD/TC23A/Tricore/Ccu6/PwmHl \
    -I$(LIBS)/iLLD/TC23A/Tricore/Ccu6/Std \
    -I$(LIBS)/iLLD/TC23A/Tricore/Ccu6/Timer \
    -I$(LIBS)/iLLD/TC23A/Tricore/Ccu6/TimerWithTrigger \
    -I$(LIBS)/iLLD/TC23A/Tricore/Ccu6/TPwm \
    -I$(LIBS)/iLLD/TC23A/Tricore/Cpu \
    -I$(LIBS)/iLLD/TC23A/Tricore/Cpu/CStart \
    -I$(LIBS)/iLLD/TC23A/Tricore/Cpu/Irq \
    -I$(LIBS)/iLLD/TC23A/Tricore/Cpu/Std \
    -I$(LIBS)/iLLD/TC23A/Tricore/Cpu/Trap \
    -I$(LIBS)/iLLD/TC23A/Tricore/Gpt12 \
    -I$(LIBS)/iLLD/TC23A/Tricore/Gpt12/IncrEnc \
    -I$(LIBS)/iLLD/TC23A/Tricore/Gpt12/Std \
    -I$(LIBS)/iLLD/TC23A/Tricore/Gtm \
    -I$(LIBS)/iLLD/TC23A/Tricore/Gtm/Atom \
    -I$(LIBS)/iLLD/TC23A/Tricore/Gtm/Atom/Pwm \
    -I$(LIBS)/iLLD/TC23A/Tricore/Gtm/Atom/PwmHl \
    -I$(LIBS)/iLLD/TC23A/Tricore/Gtm/Atom/Timer \
    -I$(LIBS)/iLLD/TC23A/Tricore/Gtm/Std \
    -I$(LIBS)/iLLD/TC23A/Tricore/Gtm/Tim \
    -I$(LIBS)/iLLD/TC23A/Tricore/Gtm/Tim/In \
    -I$(LIBS)/iLLD/TC23A/Tricore/Gtm/Tom \
    -I$(LIBS)/iLLD/TC23A/Tricore/Gtm/Tom/Pwm \
    -I$(LIBS)/iLLD/TC23A/Tricore/Gtm/Tom/PwmHl \
    -I$(LIBS)/iLLD/TC23A/Tricore/Gtm/Tom/Timer \
    -I$(LIBS)/iLLD/TC23A/Tricore/Multican \
    -I$(LIBS)/iLLD/TC23A/Tricore/Multican/Can \
    -I$(LIBS)/iLLD/TC23A/Tricore/Multican/Std \
    -I$(LIBS)/iLLD/TC23A/Tricore/Port \
    -I$(LIBS)/iLLD/TC23A/Tricore/Port/Io \
    -I$(LIBS)/iLLD/TC23A/Tricore/Port/Std \
    -I$(LIBS)/iLLD/TC23A/Tricore/Scu \
    -I$(LIBS)/iLLD/TC23A/Tricore/Scu/Std \
    -I$(LIBS)/iLLD/TC23A/Tricore/Sent \
    -I$(LIBS)/iLLD/TC23A/Tricore/Sent/Sent \
    -I$(LIBS)/iLLD/TC23A/Tricore/Sent/Std \
    -I$(LIBS)/iLLD/TC23A/Tricore/Stm \
    -I$(LIBS)/iLLD/TC23A/Tricore/Stm/Std \
    -I$(LIBS)/iLLD/TC23A/Tricore/Stm/Timer \
    -I$(LIBS)/iLLD/TC23A/Tricore/Vadc \
    -I$(LIBS)/iLLD/TC23A/Tricore/Vadc/Adc \
    -I$(LIBS)/iLLD/TC23A/Tricore/Vadc/Std \
    -I$(LIBS)/Infra \
    -I$(LIBS)/Infra/Platform \
    -I$(LIBS)/Infra/Platform/Tricore \
    -I$(LIBS)/Infra/Platform/Tricore/Compilers \
    -I$(LIBS)/Infra/Sfr \
    -I$(LIBS)/Infra/Sfr/TC23A \
    -I$(LIBS)/Infra/Sfr/TC23A/_Reg \
    -I$(LIBS)/Service \
    -I$(LIBS)/Service/CpuGeneric \
    -I$(LIBS)/Service/CpuGeneric/_Utilities \
    -I$(LIBS)/Service/CpuGeneric/If \
    -I$(LIBS)/Service/CpuGeneric/If/Ccu6If \
    -I$(LIBS)/Service/CpuGeneric/StdIf \
    -I$(LIBS)/Service/CpuGeneric/SysSe \
    -I$(LIBS)/Service/CpuGeneric/SysSe/Bsp \
    -I$(LIBS)/Service/CpuGeneric/SysSe/Comm \
    -I$(LIBS)/Service/CpuGeneric/SysSe/General \
    -I$(LIBS)/Service/CpuGeneric/SysSe/Math \
    -I$(LIBS)/Service/CpuGeneric/SysSe/Time \
    -I$(ROOT)

# --- Compile flags (same profile as the other appkit-tc234 projects) ---
CFLAGS  := -c -mcpu=$(ARCH) -D__HIGHTEC__ -D__TRICORE__ -std=c11 -Wall -O2 \
           -ffunction-sections -fdata-sections -MMD -MP -g
CFLAGS += $(INCLUDES)

# --- Sources -> objects (mangled paths avoid name collisions) ---
SRCS := $(shell find $(LIBS) -maxdepth 10 -name '*.c'; find $(ROOT) -maxdepth 1 -name '*.c')
rel   = $(if $(filter $(LIBS)/%,$(1)),$(patsubst $(LIBS)/%,%,$(1)),$(patsubst $(ROOT)/%,%,$(1)))
OBJS := $(foreach src,$(SRCS),$(BUILD)/$(subst /,_,$(call rel,$(src))).o)

define CC_rule
$(BUILD)/$(subst /,_,$(call rel,$(1))).o: $(1)
	@mkdir -p $$(dir $$@)
	$(CC) $(CFLAGS) -o $$@ $$<
endef
$(foreach src,$(SRCS),$(eval $(call CC_rule,$(src))))

# --- Header dependencies (generated with -MMD -MP) ---
DEPS := $(OBJS:.o=.d)
-include $(DEPS)

# --- Link / hex ---
ELF := $(BUILD)/$(PROJ).elf
HEX := $(BUILD)/$(PROJ).hex
LDS := $(ROOT)/Lcf_Gnuc_Tricore_Tc.lsl

all: $(ELF)
hex: $(HEX)

$(ELF): $(OBJS)
	$(CC) -mcpu=$(ARCH) -T $(LDS) -nostdlib -Wl,--gc-sections \
		-Wl,-Map=$(BUILD)/$(PROJ).map -o $@ $^ -lgcc -lc -lnosys -lgcc

$(HEX): $(ELF)
	$(OBJCOPY) -O ihex $< $@

flash: $(HEX)
	"$(AURIX_FLASHER)" -hex "$(HEX)" -prog on -ver on -start on

size: $(ELF)
	$(SIZE) $<

clean:
	rm -rf $(BUILD)

.PHONY: all hex flash size clean
