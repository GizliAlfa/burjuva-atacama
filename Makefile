# Burjuva Motor Control - Makefile
# STM32F103 ARM Cortex-M3 Build System

###############################################################################
# Configuration
###############################################################################

# Project name
PROJECT = burjuva-motor

# Target MCU
MCU = cortex-m3
MCU_FAMILY = STM32F1xx

# Toolchain
PREFIX = arm-none-eabi-
CC = $(PREFIX)gcc
AS = $(PREFIX)as
LD = $(PREFIX)ld
OBJCOPY = $(PREFIX)objcopy
OBJDUMP = $(PREFIX)objdump
SIZE = $(PREFIX)size
GDB = $(PREFIX)gdb

# Directories
SRC_DIR = firmware/src
INC_DIR = firmware/inc
BUILD_DIR = build
OBJ_DIR = $(BUILD_DIR)/obj
BIN_DIR = $(BUILD_DIR)/bin

# Main source selection (can be overridden)
MAIN_SRC ?= main.c

# Source files
C_SOURCES = \
    $(SRC_DIR)/$(MAIN_SRC) \
    $(SRC_DIR)/motor.c \
    $(SRC_DIR)/fpga_driver.c \
    $(SRC_DIR)/spi.c \
    $(SRC_DIR)/uart.c \
    $(SRC_DIR)/analog_io.c \
    $(SRC_DIR)/digital_io.c

# Add real-time scheduler if using main_rtos.c
ifeq ($(MAIN_SRC),main_rtos.c)
    C_SOURCES += $(SRC_DIR)/rtos_scheduler.c
endif

# If using STM32 HAL, add HAL sources:
# HAL_SRC_DIR = ../STM32Cube_FW_F1_V1.8.0/Drivers/STM32F1xx_HAL_Driver/Src
# C_SOURCES += \
#     $(HAL_SRC_DIR)/stm32f1xx_hal.c \
#     $(HAL_SRC_DIR)/stm32f1xx_hal_cortex.c \
#     $(HAL_SRC_DIR)/stm32f1xx_hal_rcc.c \
#     $(HAL_SRC_DIR)/stm32f1xx_hal_gpio.c \
#     $(HAL_SRC_DIR)/stm32f1xx_hal_spi.c

# ASM sources
ASM_SOURCES = 
# startup_stm32f103xb.s

# Include paths
INCLUDES = \
    -I$(INC_DIR)

# If using STM32 HAL:
# INCLUDES += \
#     -I../STM32Cube_FW_F1_V1.8.0/Drivers/STM32F1xx_HAL_Driver/Inc \
#     -I../STM32Cube_FW_F1_V1.8.0/Drivers/CMSIS/Device/ST/STM32F1xx/Include \
#     -I../STM32Cube_FW_F1_V1.8.0/Drivers/CMSIS/Include

# Compiler flags
CFLAGS = -mcpu=$(MCU) -mthumb
CFLAGS += -O2 -g3
CFLAGS += -Wall -Wextra -Wpedantic
CFLAGS += -ffunction-sections -fdata-sections
CFLAGS += -std=c11
CFLAGS += $(INCLUDES)

# Defines
DEFS = -DSTM32F103xB
# DEFS += -DUSE_HAL_DRIVER
# DEFS += -DUSE_STM32_HAL

CFLAGS += $(DEFS)

# Assembler flags
ASFLAGS = -mcpu=$(MCU) -mthumb -g

# Linker flags
LDFLAGS = -mcpu=$(MCU) -mthumb
LDFLAGS += -specs=nosys.specs
LDFLAGS += -specs=nano.specs
# LDFLAGS += -T STM32F103C8Tx_FLASH.ld
LDFLAGS += -Wl,-Map=$(BUILD_DIR)/$(PROJECT).map
LDFLAGS += -Wl,--gc-sections
LDFLAGS += -lc -lm -lnosys

# Object files
OBJECTS = $(addprefix $(OBJ_DIR)/,$(notdir $(C_SOURCES:.c=.o)))
OBJECTS += $(addprefix $(OBJ_DIR)/,$(notdir $(ASM_SOURCES:.s=.o)))

# Output files
ELF = $(BUILD_DIR)/$(PROJECT).elf
BIN = $(BUILD_DIR)/$(PROJECT).bin
HEX = $(BUILD_DIR)/$(PROJECT).hex
LST = $(BUILD_DIR)/$(PROJECT).lst

###############################################################################
# Build Rules
###############################################################################

.PHONY: all clean flash size help rtos standard

all: $(BUILD_DIR) $(BIN) $(HEX) size

# Create build directory
$(BUILD_DIR):
	@echo "Creating build directories..."
	@mkdir -p $(OBJ_DIR) $(BIN_DIR)

# Compile C sources
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@echo "Compiling $<..."
	@$(CC) $(CFLAGS) -c $< -o $@

# Compile ASM sources
$(OBJ_DIR)/%.o: %.s
	@echo "Assembling $<..."
	@$(AS) $(ASFLAGS) -c $< -o $@

# Link
$(ELF): $(OBJECTS)
	@echo "Linking..."
	@$(CC) $(OBJECTS) $(LDFLAGS) -o $@

# Generate BIN
$(BIN): $(ELF)
	@echo "Creating binary..."
	@$(OBJCOPY) -O binary $< $@
	@cp $@ $(BIN_DIR)/$(notdir $@)

# Generate HEX
$(HEX): $(ELF)
	@echo "Creating hex..."
	@$(OBJCOPY) -O ihex $< $@

# Generate listing
$(LST): $(ELF)
	@$(OBJDUMP) -h -S $< > $@

# Print size
size: $(ELF)
	@echo ""
	@echo "Build complete!"
	@echo "----------------"
	@$(SIZE) $<
	@echo ""

# Clean build files
clean:
	@echo "Cleaning..."
	@rm -rf $(BUILD_DIR)

# Flash to target (using st-flash)
flash: $(BIN)
	@echo "Flashing to STM32..."
	st-flash write $(BIN) 0x08000000

# Flash using OpenOCD
flash-openocd: $(ELF)
	@echo "Flashing via OpenOCD..."
	openocd -f interface/stlink.cfg -f target/stm32f1x.cfg \
	        -c "program $(ELF) verify reset exit"

# Debug with GDB
debug: $(ELF)
	$(GDB) $(ELF)

# Build variants
rtos: clean
	@echo "Building REAL-TIME version..."
	@$(MAKE) all MAIN_SRC=main_rtos.c

standard: clean
	@echo "Building STANDARD version..."
	@$(MAKE) all MAIN_SRC=main.c

# Help
help:
	@echo "Burjuva Motor Control - Build System"
	@echo ""
	@echo "Targets:"
	@echo "  all          - Build project (default)"
	@echo "  rtos         - Build REAL-TIME version (main_rtos.c)"
	@echo "  standard     - Build STANDARD version (main.c)"
	@echo "  clean        - Remove build files"
	@echo "  flash        - Flash firmware to STM32 (st-flash)"
	@echo "  flash-openocd - Flash firmware via OpenOCD"
	@echo "  size         - Print size information"
	@echo "  debug        - Start GDB debugger"
	@echo "  help         - Show this help"
	@echo ""
	@echo "Configuration:"
	@echo "  MCU: $(MCU)"
	@echo "  Project: $(PROJECT)"
	@echo "  Toolchain: $(PREFIX)"
	@echo "  Main Source: $(MAIN_SRC)"
	@echo ""
	@echo "Examples:"
	@echo "  make rtos              # Build real-time version"
	@echo "  make standard          # Build standard version"
	@echo "  make MAIN_SRC=main.c   # Specify main file manually"

###############################################################################
# Dependency generation
###############################################################################

-include $(OBJECTS:.o=.d)

$(OBJ_DIR)/%.d: $(SRC_DIR)/%.c
	@$(CC) $(CFLAGS) -MM -MT $(OBJ_DIR)/$*.o $< -MF $@
