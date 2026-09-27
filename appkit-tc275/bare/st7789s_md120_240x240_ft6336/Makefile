# st7789s_md120_240x240_ft6336 - TC275 (TC27x) build/flash
#
# Targets:   make            build build/st7789s_md120_240x240_ft6336.elf (link)
#            make hex        build build/st7789s_md120_240x240_ft6336.hex
#            make flash      program build/st7789s_md120_240x240_ft6336.hex (builds it first if missing)
#            make size       print section sizes
#            make clean      remove build/
#
# Toolchain: run from MSYS2 (C:\msys64\usr\bin\bash.exe) or Git Bash.
#            Set TRICORE_GCC if tricore-elf-gcc is not on PATH, e.g.
#            make TRICORE_GCC=/d/aurixgcc_03_2026/bin/tricore-elf-gcc
#            Override the flasher with: make flash AURIX_FLASHER=...

PROJ := st7789s_md120_240x240_ft6336

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
LIBS  := $(abspath $(ROOT)/../../Libraries)
BUILD := build
ARCH  := tc27xx

# --- Include paths (from .cproject) ---
INCLUDES := \
    -I$(ROOT)/Configurations \
    -I$(LIBS) \
    -I$(LIBS)/iLLD \
    -I$(LIBS)/iLLD/TC27D \
    -I$(LIBS)/iLLD/TC27D/Tricore \
    -I$(LIBS)/iLLD/TC27D/Tricore/_Impl \
    -I$(LIBS)/iLLD/TC27D/Tricore/_Lib \
    -I$(LIBS)/iLLD/TC27D/Tricore/_Lib/DataHandling \
    -I$(LIBS)/iLLD/TC27D/Tricore/_Lib/InternalMux \
    -I$(LIBS)/iLLD/TC27D/Tricore/_PinMap \
    -I$(LIBS)/iLLD/TC27D/Tricore/Asclin \
    -I$(LIBS)/iLLD/TC27D/Tricore/Asclin/Asc \
    -I$(LIBS)/iLLD/TC27D/Tricore/Asclin/Lin \
    -I$(LIBS)/iLLD/TC27D/Tricore/Asclin/Spi \
    -I$(LIBS)/iLLD/TC27D/Tricore/Asclin/Std \
    -I$(LIBS)/iLLD/TC27D/Tricore/Ccu6 \
    -I$(LIBS)/iLLD/TC27D/Tricore/Ccu6/Icu \
    -I$(LIBS)/iLLD/TC27D/Tricore/Ccu6/PwmBc \
    -I$(LIBS)/iLLD/TC27D/Tricore/Ccu6/PwmHl \
    -I$(LIBS)/iLLD/TC27D/Tricore/Ccu6/Std \
    -I$(LIBS)/iLLD/TC27D/Tricore/Ccu6/Timer \
    -I$(LIBS)/iLLD/TC27D/Tricore/Ccu6/TimerWithTrigger \
    -I$(LIBS)/iLLD/TC27D/Tricore/Ccu6/TPwm \
    -I$(LIBS)/iLLD/TC27D/Tricore/Cif \
    -I$(LIBS)/iLLD/TC27D/Tricore/Cif/Cam \
    -I$(LIBS)/iLLD/TC27D/Tricore/Cif/Std \
    -I$(LIBS)/iLLD/TC27D/Tricore/Cpu \
    -I$(LIBS)/iLLD/TC27D/Tricore/Cpu/CStart \
    -I$(LIBS)/iLLD/TC27D/Tricore/Cpu/Irq \
    -I$(LIBS)/iLLD/TC27D/Tricore/Cpu/Std \
    -I$(LIBS)/iLLD/TC27D/Tricore/Cpu/Trap \
    -I$(LIBS)/iLLD/TC27D/Tricore/Dma \
    -I$(LIBS)/iLLD/TC27D/Tricore/Dma/Dma \
    -I$(LIBS)/iLLD/TC27D/Tricore/Dma/Std \
    -I$(LIBS)/iLLD/TC27D/Tricore/Dsadc \
    -I$(LIBS)/iLLD/TC27D/Tricore/Dsadc/Dsadc \
    -I$(LIBS)/iLLD/TC27D/Tricore/Dsadc/Rdc \
    -I$(LIBS)/iLLD/TC27D/Tricore/Dsadc/Std \
    -I$(LIBS)/iLLD/TC27D/Tricore/Dts \
    -I$(LIBS)/iLLD/TC27D/Tricore/Dts/Dts \
    -I$(LIBS)/iLLD/TC27D/Tricore/Dts/Std \
    -I$(LIBS)/iLLD/TC27D/Tricore/Emem \
    -I$(LIBS)/iLLD/TC27D/Tricore/Emem/Std \
    -I$(LIBS)/iLLD/TC27D/Tricore/Eray \
    -I$(LIBS)/iLLD/TC27D/Tricore/Eray/Eray \
    -I$(LIBS)/iLLD/TC27D/Tricore/Eray/Std \
    -I$(LIBS)/iLLD/TC27D/Tricore/Eth \
    -I$(LIBS)/iLLD/TC27D/Tricore/Eth/Phy_Pef7071 \
    -I$(LIBS)/iLLD/TC27D/Tricore/Eth/Std \
    -I$(LIBS)/iLLD/TC27D/Tricore/Fce \
    -I$(LIBS)/iLLD/TC27D/Tricore/Fce/Crc \
    -I$(LIBS)/iLLD/TC27D/Tricore/Fce/Std \
    -I$(LIBS)/iLLD/TC27D/Tricore/Flash \
    -I$(LIBS)/iLLD/TC27D/Tricore/Flash/Std \
    -I$(LIBS)/iLLD/TC27D/Tricore/Gpt12 \
    -I$(LIBS)/iLLD/TC27D/Tricore/Gpt12/IncrEnc \
    -I$(LIBS)/iLLD/TC27D/Tricore/Gpt12/Std \
    -I$(LIBS)/iLLD/TC27D/Tricore/Gtm \
    -I$(LIBS)/iLLD/TC27D/Tricore/Gtm/Atom \
    -I$(LIBS)/iLLD/TC27D/Tricore/Gtm/Atom/Pwm \
    -I$(LIBS)/iLLD/TC27D/Tricore/Gtm/Atom/PwmHl \
    -I$(LIBS)/iLLD/TC27D/Tricore/Gtm/Atom/Timer \
    -I$(LIBS)/iLLD/TC27D/Tricore/Gtm/Std \
    -I$(LIBS)/iLLD/TC27D/Tricore/Gtm/Tim \
    -I$(LIBS)/iLLD/TC27D/Tricore/Gtm/Tim/In \
    -I$(LIBS)/iLLD/TC27D/Tricore/Gtm/Tom \
    -I$(LIBS)/iLLD/TC27D/Tricore/Gtm/Tom/Pwm \
    -I$(LIBS)/iLLD/TC27D/Tricore/Gtm/Tom/PwmHl \
    -I$(LIBS)/iLLD/TC27D/Tricore/Gtm/Tom/Timer \
    -I$(LIBS)/iLLD/TC27D/Tricore/Gtm/Trig \
    -I$(LIBS)/iLLD/TC27D/Tricore/Hssl \
    -I$(LIBS)/iLLD/TC27D/Tricore/Hssl/Hssl \
    -I$(LIBS)/iLLD/TC27D/Tricore/Hssl/Std \
    -I$(LIBS)/iLLD/TC27D/Tricore/I2c \
    -I$(LIBS)/iLLD/TC27D/Tricore/I2c/I2c \
    -I$(LIBS)/iLLD/TC27D/Tricore/I2c/Std \
    -I$(LIBS)/iLLD/TC27D/Tricore/Iom \
    -I$(LIBS)/iLLD/TC27D/Tricore/Iom/Driver \
    -I$(LIBS)/iLLD/TC27D/Tricore/Iom/Std \
    -I$(LIBS)/iLLD/TC27D/Tricore/Msc \
    -I$(LIBS)/iLLD/TC27D/Tricore/Msc/Msc \
    -I$(LIBS)/iLLD/TC27D/Tricore/Msc/Std \
    -I$(LIBS)/iLLD/TC27D/Tricore/Mtu \
    -I$(LIBS)/iLLD/TC27D/Tricore/Mtu/Std \
    -I$(LIBS)/iLLD/TC27D/Tricore/Multican \
    -I$(LIBS)/iLLD/TC27D/Tricore/Multican/Can \
    -I$(LIBS)/iLLD/TC27D/Tricore/Multican/Std \
    -I$(LIBS)/iLLD/TC27D/Tricore/Port \
    -I$(LIBS)/iLLD/TC27D/Tricore/Port/Io \
    -I$(LIBS)/iLLD/TC27D/Tricore/Port/Std \
    -I$(LIBS)/iLLD/TC27D/Tricore/Psi5 \
    -I$(LIBS)/iLLD/TC27D/Tricore/Psi5/Psi5 \
    -I$(LIBS)/iLLD/TC27D/Tricore/Psi5/Std \
    -I$(LIBS)/iLLD/TC27D/Tricore/Psi5s \
    -I$(LIBS)/iLLD/TC27D/Tricore/Psi5s/Psi5s \
    -I$(LIBS)/iLLD/TC27D/Tricore/Psi5s/Std \
    -I$(LIBS)/iLLD/TC27D/Tricore/Qspi \
    -I$(LIBS)/iLLD/TC27D/Tricore/Qspi/SpiMaster \
    -I$(LIBS)/iLLD/TC27D/Tricore/Qspi/SpiSlave \
    -I$(LIBS)/iLLD/TC27D/Tricore/Qspi/Std \
    -I$(LIBS)/iLLD/TC27D/Tricore/Scu \
    -I$(LIBS)/iLLD/TC27D/Tricore/Scu/Std \
    -I$(LIBS)/iLLD/TC27D/Tricore/Sent \
    -I$(LIBS)/iLLD/TC27D/Tricore/Sent/Sent \
    -I$(LIBS)/iLLD/TC27D/Tricore/Sent/Std \
    -I$(LIBS)/iLLD/TC27D/Tricore/Smu \
    -I$(LIBS)/iLLD/TC27D/Tricore/Smu/Std \
    -I$(LIBS)/iLLD/TC27D/Tricore/Src \
    -I$(LIBS)/iLLD/TC27D/Tricore/Src/Std \
    -I$(LIBS)/iLLD/TC27D/Tricore/Stm \
    -I$(LIBS)/iLLD/TC27D/Tricore/Stm/Std \
    -I$(LIBS)/iLLD/TC27D/Tricore/Stm/Timer \
    -I$(LIBS)/iLLD/TC27D/Tricore/Vadc \
    -I$(LIBS)/iLLD/TC27D/Tricore/Vadc/Adc \
    -I$(LIBS)/iLLD/TC27D/Tricore/Vadc/Std \
    -I$(LIBS)/Infra \
    -I$(LIBS)/Infra/Platform \
    -I$(LIBS)/Infra/Platform/Tricore \
    -I$(LIBS)/Infra/Platform/Tricore/Compilers \
    -I$(LIBS)/Infra/Sfr \
    -I$(LIBS)/Infra/Sfr/TC27D \
    -I$(LIBS)/Infra/Sfr/TC27D/_Reg \
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
    -I$(LIBS)/Service/CpuGeneric/SysSe/Time

# Project root include (project-local headers live here).
INCLUDES += -I$(ROOT)
# LCD/touch driver sources live under src/ (ported from the STM32 ports).
INCLUDES += -I$(ROOT)/src
INCLUDES += -I$(ROOT)/src/lcd
INCLUDES += -I$(ROOT)/src/blockwrite

# --- Compile flags ---
CFLAGS  := -c -mcpu=$(ARCH) -D__HIGHTEC__ -D__TRICORE__ -std=c11 -Wall -O2 \
           -ffunction-sections -fdata-sections -MMD -MP -g
CFLAGS += $(INCLUDES)

# --- Sources -> objects (mangled paths avoid name collisions) ---
SRCS := $(shell find $(LIBS) $(ROOT) -maxdepth 10 -name '*.c')
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
