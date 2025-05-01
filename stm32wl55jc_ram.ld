/* Pure RAM execution linker script for STM32WL55JC */

/* Entry point */
ENTRY(Reset_Handler)

/* Memory definitions */
MEMORY
{
  RAM (rwx) : ORIGIN = 0x20000000, LENGTH = 64K
}

/* Highest address of the user mode stack */
_estack = ORIGIN(RAM) + LENGTH(RAM);

/* Define sections - everything in RAM */
SECTIONS
{
  /* Vector table in RAM at the very beginning */
  .vectors :
  {
    . = ALIGN(4);
    KEEP(*(.vectors))    /* Main vector table */
    . = ALIGN(4);
  } >RAM
  
  /* Code section in RAM */
  .text :
  {
    . = ALIGN(4);
    *(.text)             /* Code */
    *(.text*)
    *(.rodata)           /* Read-only data */
    *(.rodata*)
    . = ALIGN(4);
  } >RAM

  /* Data section in RAM */
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