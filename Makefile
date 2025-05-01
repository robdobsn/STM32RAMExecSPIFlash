# Makefile for minimal STM32WL55JC LED blink example - Pure RAM Execution

# Detect operating system
ifeq ($(OS),Windows_NT)
  # Windows-specific settings
  PROGRAMMER = "C:\Program Files\STMicroelectronics\STM32Cube\STM32CubeProgrammer\bin\STM32_Programmer_CLI.exe"
  RM = del /Q
  MKDIR = mkdir
else
  # Linux/Unix-specific settings
  PROGRAMMER = STM32_Programmer_CLI
  RM = rm -f
  MKDIR = mkdir -p
endif

# Toolchain definitions
CC = arm-none-eabi-gcc
LD = arm-none-eabi-gcc
OBJCOPY = arm-none-eabi-objcopy
OBJDUMP = arm-none-eabi-objdump
SIZE = arm-none-eabi-size

# Project name
PROJECT = ram_only_blink

# MCU flags
MCU = -mcpu=cortex-m4 -mthumb -mfloat-abi=soft

# C flags
CFLAGS = $(MCU) -Wall -g -Os -ffunction-sections -fdata-sections

# Linker flags
LDFLAGS = $(MCU) -specs=nano.specs -specs=nosys.specs -Wl,--gc-sections -T stm32wl55jc_ram.ld -Wl,-Map=$(PROJECT).map -Wl,-e,Reset_Handler -Wl,-u,Write -Wl,-u,Read -Wl,-u,SectorErase -Wl,-u,MassErase -Wl,-u,Init

# Source files and object files
SRC = main.c Dev_Inf.c
OBJ = main.o Dev_Inf.o

# Build target
all: $(PROJECT).elf

# Link object files to create elf file
$(PROJECT).elf: $(OBJ)
	$(CC) $(LDFLAGS) -Wl,--whole-archive main.o -Wl,--no-whole-archive Dev_Inf.o -o $@
	$(OBJCOPY) -O binary $@ $(PROJECT).bin
	$(OBJCOPY) -O ihex $@ $(PROJECT).hex
	$(SIZE) $@

%.o: %.c
	$(CC) -c $(CFLAGS) $< -o $@

clean:
	$(RM) $(OBJ) $(PROJECT).elf $(PROJECT).hex $(PROJECT).bin $(PROJECT).dump

# Flash to RAM and start execution from RAM - only option now
flash:
	$(PROGRAMMER) -c port=SWD -w $(PROJECT).bin 0x20000000 -s 0x20000000

# Dump the binary for debugging
dump:
	$(OBJDUMP) -D $(PROJECT).elf > $(PROJECT).dump

.PHONY: all clean size flash dump 