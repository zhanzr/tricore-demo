# coremark_133m - TC212 (TC22x) build/flash
#
# Targets:   make            build <proj>.hex (same as `make hex`)
#            make hex        build <proj>.hex
#            make flash      program <proj>.hex via AURIXFlasher
#            make size       print section sizes
#            make clean      remove build/
#
# Toolchain: run from MSYS2 (C:\msys64\usr\bin\bash.exe) or Git Bash.
#            Set TRICORE_GCC if tricore-elf-gcc is not on PATH, e.g.
#            make TRICORE_GCC=/d/aurixgcc_03_2026/bin/tricore-elf-gcc
#            Override the flasher with: make flash AURIX_FLASHER=...

PROJ := coremark_133m

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
ARCH  := tc22xx

# --- Compile flags ---
CFLAGS  := -c -mcpu=$(ARCH) -D__HIGHTEC__ -D__TRICORE__ -std=c11 -Wall -O3 -ffast-math -funroll-loops -finline-functions -fno-math-errno -MMD -MP -g
INCLUDES := \
	-I$(ROOT)/Configurations \
	-I$(ROOT) \
	-I$(ROOT)/coremark_1_0_1 \
	-I$(ROOT)/coremark_1_0_1/tc212 \
	-I$(LIBS)/iLLD/TC22A/Tricore \
	-I$(LIBS)/iLLD/TC22A/Tricore/_Impl \
	-I$(LIBS)/iLLD/TC22A/Tricore/_Lib \
	-I$(LIBS)/iLLD/TC22A/Tricore/_Lib/DataHandling \
	-I$(LIBS)/iLLD/TC22A/Tricore/_PinMap \
	-I$(LIBS)/iLLD/TC22A/Tricore/Asclin \
	-I$(LIBS)/iLLD/TC22A/Tricore/Asclin/Asc \
	-I$(LIBS)/iLLD/TC22A/Tricore/Asclin/Std \
	-I$(LIBS)/iLLD/TC22A/Tricore/Cpu \
	-I$(LIBS)/iLLD/TC22A/Tricore/Cpu/CStart \
	-I$(LIBS)/iLLD/TC22A/Tricore/Cpu/Irq \
	-I$(LIBS)/iLLD/TC22A/Tricore/Cpu/Std \
	-I$(LIBS)/iLLD/TC22A/Tricore/Cpu/Trap \
	-I$(LIBS)/iLLD/TC22A/Tricore/Dts/Dts \
	-I$(LIBS)/iLLD/TC22A/Tricore/Dts/Std \
	-I$(LIBS)/iLLD/TC22A/Tricore/Flash/Std \
	-I$(LIBS)/iLLD/TC22A/Tricore/Gtm \
	-I$(LIBS)/iLLD/TC22A/Tricore/Gtm/Std \
	-I$(LIBS)/iLLD/TC22A/Tricore/Gtm/Tom/Pwm \
	-I$(LIBS)/iLLD/TC22A/Tricore/Port/Std \
	-I$(LIBS)/iLLD/TC22A/Tricore/Port/Io \
	-I$(LIBS)/iLLD/TC22A/Tricore/Scu/Std \
	-I$(LIBS)/iLLD/TC22A/Tricore/Src/Std \
	-I$(LIBS)/iLLD/TC22A/Tricore/Stm/Std \
	-I$(LIBS)/iLLD/TC22A/Tricore/Vadc \
	-I$(LIBS)/iLLD/TC22A/Tricore/Vadc/Std \
	-I$(LIBS)/iLLD/TC22A/Tricore/Vadc/Adc \
	-I$(LIBS)/Infra/Sfr/TC22A/_Reg \
	-I$(LIBS)/Infra/Platform/Tricore/Compilers \
	-I$(LIBS)/Infra/Platform/Tricore \
	-I$(LIBS)/Infra/Platform \
	-I$(LIBS)/Infra/Sfr/TC22A \
	-I$(LIBS)/Infra/Sfr \
	-I$(LIBS)/Infra \
	-I$(LIBS)/Service/CpuGeneric/StdIf \
	-I$(LIBS)/Service/CpuGeneric/SysSe/Bsp \
	-I$(LIBS)/Service/CpuGeneric/SysSe/Comm \
	-I$(LIBS)/Service/CpuGeneric/SysSe/General \
	-I$(LIBS)/Service/CpuGeneric/SysSe/Time \
	-I$(LIBS)/Service/CpuGeneric/SysSe \
	-I$(LIBS)/Service/CpuGeneric/If \
	-I$(LIBS)/Service/CpuGeneric \
	-I$(LIBS)/Service \
	-I$(LIBS)
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

all: $(HEX)
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
