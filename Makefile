# Makefile for minimal STM32WL55JC LED blink example

# Toolchain definitions
CC = arm-none-eabi-gcc
LD = arm-none-eabi-gcc
OBJCOPY = arm-none-eabi-objcopy
OBJDUMP = arm-none-eabi-objdump
SIZE = arm-none-eabi-size

# Project name
PROJECT = minimal_blink

# MCU flags
MCU = -mcpu=cortex-m4 -mthumb -mfloat-abi=soft

# C flags
CFLAGS = $(MCU) -Wall -g -Os -ffunction-sections -fdata-sections

# Linker flags
LDFLAGS = $(MCU) -specs=nano.specs -specs=nosys.specs -Wl,--gc-sections -T stm32wl55jc_flash.ld

# Source files
SRCS = main.c

# Object files
OBJS = $(SRCS:.c=.o)

# Build targets
all: $(PROJECT).elf $(PROJECT).bin $(PROJECT).hex size

%.o: %.c
	$(CC) -c $(CFLAGS) $< -o $@

$(PROJECT).elf: $(OBJS) stm32wl55jc_flash.ld
	$(LD) $(LDFLAGS) $(OBJS) -o $@

$(PROJECT).hex: $(PROJECT).elf
	$(OBJCOPY) -O ihex $< $@

$(PROJECT).bin: $(PROJECT).elf
	$(OBJCOPY) -O binary $< $@

size: $(PROJECT).elf
	$(SIZE) $(PROJECT).elf

clean:
	rm -f $(OBJS) $(PROJECT).elf $(PROJECT).hex $(PROJECT).bin

flash:
	"C:\Program Files\STMicroelectronics\STM32Cube\STM32CubeProgrammer\bin\STM32_Programmer_CLI.exe" -c port=SWD -w $(PROJECT).bin 0x08000000 -s 0x08000000

.PHONY: all clean size flash 