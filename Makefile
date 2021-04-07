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
	-Wall \
	-std=c99 \
	-Wno-implicit-function-declaration -Wdouble-promotion \
	-Wextra -Wshadow -Wimplicit-function-declaration -Wredundant-decls \
	-fno-common -ffunction-sections -fdata-sections -Wno-unused-function

ifeq ($(DEBUG), 1)
	CFLAGS += -g
else
	CFLAGS += -O2
endif

# ifeq (, $(wildcard ./generated.*.ld))
include $(OPENCM3_DIR)/mk/genlink-config.mk
include $(OPENCM3_DIR)/mk/gcc-config.mk
# endif

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
HEADERS := $(wildcard $(INCLUDE_DIR)/*.h)

OBJS := $(notdir $(basename $(SOURCES)))
OBJS := $(addsuffix .o, $(OBJS))
OBJS := $(addprefix $(BUILD_DIR)/, $(OBJS))

VPATH = $(dir $(SOURCES))

DEFS = -D STM32F1

all: prepare $(BIN)

prepare:
	@mkdir -p $(BUILD_DIR)

$(BUILD_DIR)/%.o: %.c $(HEADERS) Makefile
	$(CC) -c $(CFLAGS) $(INCFLAGS) $(DEFS) -o $@ $<

clean:
	@rm -rf $(BUILD_DIR)

flash: all
	st-flash --reset write $(BIN) 0x08000000

erase:
	st-flash erase

openocd:
	@rm itm-dump.fifo
	openocd

include $(OPENCM3_DIR)/mk/genlink-rules.mk
include $(OPENCM3_DIR)/mk/gcc-rules.mk

.PRECIOUS: $(OBJS) $(ELF)
.PHONY: clean flash erase openocd
