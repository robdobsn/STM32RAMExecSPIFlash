# STM32WL55JC Dual-Mode Execution Example

This project demonstrates running code on an STM32WL55JC Nucleo board from either Flash (default) or RAM.
The example simply blinks the LED connected to PB15.

## Project Structure

- `main.c` - Main application code (can run from Flash or RAM)
- `startup_ram.c` - Startup code that initializes the system
- `stm32wl55jc_ram.ld` - Linker script with dual-mode support
- `Makefile` - Build system supporting both execution modes
- `stm32wl55xx.h` - Header with register definitions

## Build Instructions

1. Ensure you have the ARM GCC toolchain installed (`arm-none-eabi-gcc` and related tools)

2. Build for Flash execution (default):
   ```bash
   make
   ```

3. Build for RAM execution:
   ```bash
   make RUN_MODE=RAM
   ```

4. The output files will be:
   - `stm32_blink.elf` / `stm32_blink_ram.elf` - ELF file with debug symbols
   - `stm32_blink.bin` / `stm32_blink_ram.bin` - Binary file for flashing
   - `stm32_blink.hex` / `stm32_blink_ram.hex` - HEX file for flashing

## How to Flash and Run

You can use STM32CubeProgrammer, OpenOCD, or any other compatible tool to load the binary.

### Using the Makefile (recommended)

```bash
# Flash the binary (automatically uses the correct memory location based on build mode)
make flash
# OR specifically for RAM mode
make flash RUN_MODE=RAM
```

### Using STM32CubeProgrammer (CLI) manually

For Flash mode:
```bash
STM32_Programmer_CLI -c port=SWD -w stm32_blink.bin 0x08000000 -s 0x08000000
```

For RAM mode:
```bash
STM32_Programmer_CLI -c port=SWD -w stm32_blink_ram.bin 0x20000000 -s 0x20000000
```

### Using OpenOCD

For Flash mode:
```bash
# Connect to the board
openocd -f board/st_nucleo_wl55jc.cfg

# In another terminal
telnet localhost 4444

# In the OpenOCD telnet session
> halt
> flash write_image erase stm32_blink.bin 0x08000000
> reset
```

For RAM mode:
```bash
# Connect to the board
openocd -f board/st_nucleo_wl55jc.cfg

# In another terminal
telnet localhost 4444

# In the OpenOCD telnet session
> halt
> load_image stm32_blink_ram.bin 0x20000000
> reg pc 0x20000000
> resume
```

## How It Works

### Flash Mode
1. Code and constant data are placed in Flash
2. The standard vector table is used from Flash
3. Variable data is copied from Flash to RAM during startup

### RAM Mode
1. The vector table is placed in RAM
2. All code and data are placed in RAM
3. The startup code redirects execution to RAM using the VTOR register

## Hardware Configuration

- STM32WL55JC Nucleo board
- LED on PB15 (on-board LED)

## Troubleshooting

- If RAM execution doesn't work, make sure your debugger is properly configured
- For STM32CubeProgrammer, ensure you're using the correct `-s` parameter to set the PC register
- Some debuggers may reset the device after programming, which might cause RAM content to be lost

## Memory Usage

- Flash memory is not used for code execution
- SRAM1 (64KB) is used for:
  - Vector table
  - Code
  - Data
  - Stack (4KB) 