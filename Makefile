OPENCM3_DIR = libopencm3
TARGET_CPU = cortex-m3
TARGET_DEVICE = STM32F10X_MD
DEVICE = stm32f103c8t6

Q = @

BUILD_DIR = build
SRC_DIR = src
INCLUDE_DIR = include
ELF = $(BUILD_DIR)/main.elf
BIN = $(BUILD_DIR)/main.bin
DEBUG = 1

CFLAGS = \
	$(ARCH_FLAGS) \
	-std=c99 \
	-Wno-implicit-function-declaration -Wdouble-promotion \
	-Wextra -Wshadow -Wimplicit-function-declaration -Wredundant-decls \
	-fno-common -ffunction-sections -fdata-sections

ifeq ($(DEBUG), 1)
	CFLAGS += -g
else
	CFLAGS += -O2
endif

include $(OPENCM3_DIR)/mk/genlink-config.mk
include $(OPENCM3_DIR)/mk/gcc-config.mk

LDFLAGS += \
	-l$(LIBNAME) \
	$(ARCH_FLAGS) \
	-Wl,--start-group -lc -lgcc -lnosys -Wl,--end-group \
	--static \
	-nostartfiles \
	--specs=nano.specs \
	--specs=nosys.specs

INCFLAGS = \
	-I $(INCLUDE_DIR) \
	-I $(OPENCM3_DIR)/include

SOURCES := $(wildcard $(SRC_DIR)/*.c)

OBJS := $(notdir $(basename $(SOURCES)))
OBJS := $(addsuffix .o, $(OBJS))
OBJS := $(addprefix $(BUILD_DIR)/, $(OBJS))

VPATH = $(dir $(SOURCES))

DEFS = -D STM32F1

all: prepare $(BIN)

check:
	@echo $(LDFLAGS)

prepare:
	@mkdir -p $(BUILD_DIR)

$(BUILD_DIR)/%.o: %.c
	@echo "$(notdir $<) -> $(notdir $@)"
	@$(CC) -c $(CFLAGS) $(INCFLAGS) $(DEFS) -o $@ $<

# $(ELF): $(OBJS) $(LDSCRIPT)
# 	@echo "$(notdir $<) -> $(notdir $@)"
# 	$(LD) $(LDFLAGS) -o $(ELF) $(OBJS)

# $(BIN): $(ELF)
# 	@$(OC) -O binary $(ELF) $(BIN)

clean:
	@rm -rf $(BUILD_DIR)

flash: all
	st-flash --reset write $(BIN) 0x08000000

erase:
	st-flash erase

include $(OPENCM3_DIR)/mk/genlink-rules.mk
include $(OPENCM3_DIR)/mk/gcc-rules.mk

.PHONY: clean flash erase gdb
