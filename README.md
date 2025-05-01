# STM32WL55JC External SPI Flash Loader for iFlow1000

This project implements an external SPI Flash loader for STM32CubeProgrammer to program SPI Flash on the Infersens iFlow1000 board. The loader runs entirely from RAM on an STM32WL55JC microcontroller, providing a complete SPI Flash programming interface.

## Features

- 100% RAM execution - no dependency on Flash memory
- Full implementation of STM32 external loader interface
- SPI communication with external Flash chip
- Support for reading, writing, sector erase, and mass erase operations
- Power control for the SPI Flash via PA4
- Status indication via LED connected to PB15
- Compatible with STM32CubeProgrammer

## Project Structure

- `main.c` - Flash loader implementation with direct register access
- `stm32wl55jc_ram.ld` - Linker script for pure RAM execution
- `Makefile` - Build system with flash command and external loader generation
- `Dev_Inf.c` - Device information for STM32CubeProgrammer

## Hardware Configuration

- STM32WL55JC microcontroller
- LED connected to PB15
- SPI Flash connected to:
  - PA4: Power control (high = on, low = off)
  - PA5: SCK (SPI1_SCK)
  - PA6: MISO (SPI1_MISO)
  - PA7: MOSI (SPI1_MOSI) 
  - PB5: CS (GPIO output)

## Build Instructions

1. Ensure you have the ARM GCC toolchain installed (`arm-none-eabi-gcc` and related tools)

2. Build the project:
   ```bash
   make
   ```

3. The output files will be:
   - `Infersens_iFlow1000_SPIFlash_ExtLoader.elf` - ELF file with debug symbols
   - `Infersens_iFlow1000_SPIFlash_ExtLoader.bin` - Binary file for loading into RAM
   - `Infersens_iFlow1000_SPIFlash_ExtLoader.hex` - HEX file (alternative format)
   - `Infersens_iFlow1000_SPIFlash_ExtLoader.stldr` - STM32 External Loader file

4. The build process will:
   - Compile all source files
   - Link the objects to create the ELF file
   - Generate binary and hex versions
   - Create a `.stldr` file by copying the ELF file

## Using as STM32CubeProgrammer External Loader

The `.stldr` file can be used by STM32CubeProgrammer as an external loader:

1. You can manually copy the `.stldr` file to your STM32CubeProgrammer's ExternalLoader directory:
   - Windows: `C:\Program Files\STMicroelectronics\STM32Cube\STM32CubeProgrammer\bin\ExternalLoader`
   - Linux: `STM32CubeProgrammer/bin/ExternalLoader`

2. In STM32CubeProgrammer, select "External Loaders" and choose "Infersens_iFlow1000_SPIFlash_ExtLoader"

## Loading and Running (for Development)

### Using the Makefile 

```bash
# Load the binary into RAM and start execution
make flash
```

This command uses STM32CubeProgrammer to:
1. Load the binary to RAM at address 0x20000000
2. Set the PC register to start execution at 0x20000000

### Using STM32CubeProgrammer (CLI) manually

```bash
STM32_Programmer_CLI -c port=SWD -w Infersens_iFlow1000_SPIFlash_ExtLoader.bin 0x20000000 -s 0x20000000
```

## How It Works

1. The loader implements four main functions required by STM32CubeProgrammer:
   - `Write()` - Programs data to flash
   - `Read()` - Reads data from flash
   - `SectorErase()` - Erases sectors (4KB blocks)
   - `MassErase()` - Erases the entire chip

2. Additionally, it provides power control for the SPI Flash:
   - `SPIFlash_PowerOn(1)` - Powers on the flash chip (PA4 set high)
   - `SPIFlash_PowerOn(0)` - Powers off the flash chip (PA4 set low)
   - Power is enabled at initialization before SPI communication

3. All code runs from RAM:
   - Vector table is placed at address 0x20000000
   - All functions execute from RAM
   - No dependency on Flash memory

## Memory Layout

- Start Address: 0x20000000
- Vector Table: At the beginning of RAM
- Code (.text): Follows the vector table, includes loader functions
- Data (.data): After code section
- BSS (.bss): Zeroed variables
- Stack: Grows down from top of RAM (0x20010000)

## Troubleshooting

- If execution doesn't start, make sure your debugger is properly setting the PC register
- If SPI communication fails, check your hardware connections
- For loader issues in STM32CubeProgrammer, enable verbose logging 