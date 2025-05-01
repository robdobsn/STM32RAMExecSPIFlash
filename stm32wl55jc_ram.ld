/* RAM execution linker script for STM32WL55JC */

/* Entry point */
ENTRY(Reset_Handler)

/* Memory definitions */
MEMORY
{
  FLASH (rx)  : ORIGIN = 0x08000000, LENGTH = 256K
  RAM   (rwx) : ORIGIN = 0x20000000, LENGTH = 64K
}

/* Highest address of the user mode stack */
_estack = ORIGIN(RAM) + LENGTH(RAM);

/* Define sections */
SECTIONS
{
  /* Initial vector table in FLASH - required for initial boot */
  .isr_vector :
  {
    . = ALIGN(4);
    KEEP(*(.isr_vector))
    . = ALIGN(4);
  } >FLASH
  
  /* RAM vector table */
  .ram_vectors :
  {
    . = ALIGN(4);
    _ram_vectors_start = .;
    KEEP(*(.ram_vectors))
    . = ALIGN(4);
  } >RAM
  
  /* Code section in RAM */
  .text :
  {
    . = ALIGN(4);
    _stext = .;
    *(.text)
    *(.text*)
    *(.rodata)
    *(.rodata*)
    . = ALIGN(4);
    _etext = .;
  } >RAM

  /* Data section goes in RAM */
  .data :
  {
    . = ALIGN(4);
    _sdata = .;
    *(.data)
    *(.data*)
    . = ALIGN(4);
    _edata = .;
  } >RAM

  /* BSS section in RAM */
  .bss :
  {
    . = ALIGN(4);
    _sbss = .;
    *(.bss)
    *(.bss*)
    *(COMMON)
    . = ALIGN(4);
    _ebss = .;
  } >RAM

  /* Discard anything from standard libraries we don't need */
  /DISCARD/ :
  {
    libc.a ( * )
    libm.a ( * )
    libgcc.a ( * )
  }
} 