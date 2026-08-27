# dhry_200m - TC234 (TC23A) build/flash
#
# Targets:   make            build build/dhry_200m.elf (link)
#            make hex        build build/dhry_200m.hex
#            make flash      program build/dhry_200m.hex (builds it first if missing)
#            make size       print section sizes
#            make clean      remove build/
#
# Toolchain: run from MSYS2 (C:\msys64\usr\bin\bash.exe) or Git Bash.
#            Set TRICORE_GCC if tricore-elf-gcc is not on PATH, e.g.
#            make TRICORE_GCC=/d/aurixgcc_03_2026/bin/tricore-elf-gcc
#            Override the flasher with: make flash AURIX_FLASHER=...

PROJ := dhry_200m

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
LIBS  := $(abspath $(ROOT)/../Libraries)
BUILD := build
ARCH  := tc23xx

# --- Include paths (from .cproject) ---
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
    -I$(LIBS)/iLLD/TC23A/Tricore/Dma \
    -I$(LIBS)/iLLD/TC23A/Tricore/Dma/Dma \
    -I$(LIBS)/iLLD/TC23A/Tricore/Dma/Std \
    -I$(LIBS)/iLLD/TC23A/Tricore/Dts \
    -I$(LIBS)/iLLD/TC23A/Tricore/Dts/Dts \
    -I$(LIBS)/iLLD/TC23A/Tricore/Dts/Std \
    -I$(LIBS)/iLLD/TC23A/Tricore/Eray \
    -I$(LIBS)/iLLD/TC23A/Tricore/Eray/Eray \
    -I$(LIBS)/iLLD/TC23A/Tricore/Eray/Std \
    -I$(LIBS)/iLLD/TC23A/Tricore/Eth \
    -I$(LIBS)/iLLD/TC23A/Tricore/Eth/Phy_Pef7071 \
    -I$(LIBS)/iLLD/TC23A/Tricore/Eth/Std \
    -I$(LIBS)/iLLD/TC23A/Tricore/Flash \
    -I$(LIBS)/iLLD/TC23A/Tricore/Flash/Std \
    -I$(LIBS)/iLLD/TC23A/Tricore/Gpt12 \
    -I$(LIBS)/iLLD/TC23A/Tricore/Gpt12/IncrEnc \
    -I$(LIBS)/iLLD/TC23A/Tricore/Gpt12/Std \
    -I$(LIBS)/iLLD/TC23A/Tricore/Gtm \
    -I$(LIBS)/iLLD/TC23A/Tricore/Gtm/Std \
    -I$(LIBS)/iLLD/TC23A/Tricore/Gtm/Tim \
    -I$(LIBS)/iLLD/TC23A/Tricore/Gtm/Tim/In \
    -I$(LIBS)/iLLD/TC23A/Tricore/Gtm/Tom \
    -I$(LIBS)/iLLD/TC23A/Tricore/Gtm/Tom/Pwm \
    -I$(LIBS)/iLLD/TC23A/Tricore/Gtm/Tom/PwmHl \
    -I$(LIBS)/iLLD/TC23A/Tricore/Gtm/Tom/Timer \
    -I$(LIBS)/iLLD/TC23A/Tricore/Gtm/Trig \
    -I$(LIBS)/iLLD/TC23A/Tricore/Iom \
    -I$(LIBS)/iLLD/TC23A/Tricore/Iom/Driver \
    -I$(LIBS)/iLLD/TC23A/Tricore/Iom/Std \
    -I$(LIBS)/iLLD/TC23A/Tricore/Mtu \
    -I$(LIBS)/iLLD/TC23A/Tricore/Mtu/Std \
    -I$(LIBS)/iLLD/TC23A/Tricore/Multican \
    -I$(LIBS)/iLLD/TC23A/Tricore/Multican/Can \
    -I$(LIBS)/iLLD/TC23A/Tricore/Multican/Std \
    -I$(LIBS)/iLLD/TC23A/Tricore/Port \
    -I$(LIBS)/iLLD/TC23A/Tricore/Port/Io \
    -I$(LIBS)/iLLD/TC23A/Tricore/Port/Std \
    -I$(LIBS)/iLLD/TC23A/Tricore/Qspi \
    -I$(LIBS)/iLLD/TC23A/Tricore/Qspi/SpiMaster \
    -I$(LIBS)/iLLD/TC23A/Tricore/Qspi/SpiSlave \
    -I$(LIBS)/iLLD/TC23A/Tricore/Qspi/Std \
    -I$(LIBS)/iLLD/TC23A/Tricore/Scu \
    -I$(LIBS)/iLLD/TC23A/Tricore/Scu/Std \
    -I$(LIBS)/iLLD/TC23A/Tricore/Sent \
    -I$(LIBS)/iLLD/TC23A/Tricore/Sent/Sent \
    -I$(LIBS)/iLLD/TC23A/Tricore/Sent/Std \
    -I$(LIBS)/iLLD/TC23A/Tricore/Smu \
    -I$(LIBS)/iLLD/TC23A/Tricore/Smu/Std \
    -I$(LIBS)/iLLD/TC23A/Tricore/Src \
    -I$(LIBS)/iLLD/TC23A/Tricore/Src/Std \
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

# Project root include (project-local headers live here).
INCLUDES += -I$(ROOT)

# --- Compile flags ---
CFLAGS  := -c -mcpu=$(ARCH) -D__HIGHTEC__ -D__TRICORE__ -std=c11 -O3 -ffast-math -funroll-loops -finline-functions -fno-math-errno            -ffunction-sections -fdata-sections -MMD -MP -g
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
