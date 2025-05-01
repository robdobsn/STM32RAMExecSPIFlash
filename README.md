# STM32WL55JC Pure RAM Execution Example

This project demonstrates running code on an STM32WL55JC microcontroller entirely from RAM, without using Flash memory at all. The example blinks an LED connected to PB15 and reads the JEDEC ID from an external SPI Flash memory.

## Features

- 100% RAM execution - no dependency on Flash memory
- LED blinking on PB15
- SPI communication with external Flash chip
- Reads JEDEC ID (manufacturer, memory type, capacity) from SPI Flash
- Minimal code size optimized for RAM usage

## Project Structure

- `main.c` - Application code with direct register access
- `stm32wl55jc_ram.ld` - Linker script for pure RAM execution
- `Makefile` - Build system with flash command for loading to RAM

## Hardware Configuration

- STM32WL55JC microcontroller
- LED connected to PB15
- SPI Flash connected to:
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
   - `ram_only_blink.elf` - ELF file with debug symbols
   - `ram_only_blink.bin` - Binary file for loading into RAM
   - `ram_only_blink.hex` - HEX file (alternative format)

## Loading and Running

### Using the Makefile (recommended)

```bash
# Load the binary into RAM and start execution
make flash
```

This command uses STM32CubeProgrammer to:
1. Load the binary to RAM at address 0x20000000
2. Set the PC register to start execution at 0x20000000

### Using STM32CubeProgrammer (CLI) manually

```bash
STM32_Programmer_CLI -c port=SWD -w ram_only_blink.bin 0x20000000 -s 0x20000000
```

## How It Works

1. Vector table is placed at the beginning of RAM (address 0x20000000)
2. All code and data are placed in RAM
3. The reset handler:
   - Sets VTOR register to point to the RAM vector table
   - Initializes the BSS section
   - Jumps to main
4. Main function:
   - Initializes GPIO for LED
   - Initializes SPI1 for Flash communication
   - Enters an infinite loop that blinks the LED and reads SPI Flash ID

## RAM Memory Layout

- Start Address: 0x20000000
- Vector Table: At the beginning of RAM
- Code (.text): Follows the vector table
- Data (.data): After code section
- BSS (.bss): Zeroed variables
- Stack: Grows down from top of RAM (0x20010000)

## Troubleshooting

- If execution doesn't start, make sure your debugger is properly setting the PC register
- If SPI communication fails, check your hardware connections
- Some debuggers may disconnect after loading, which is expected behavior 