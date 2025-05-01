/* Minimal linker script for STM32WL55JC */

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

/* Sections */
SECTIONS
{
  /* Vector table and code in FLASH */
  .isr_vector :
  {
    . = ALIGN(4);
    KEEP(*(.isr_vector))   /* Vector table */
    . = ALIGN(4);
  } >FLASH

  .text :
  {
    . = ALIGN(4);
    *(.text)               /* Code */
    *(.text*)
    *(.rodata)             /* Read-only data */
    *(.rodata*)
    . = ALIGN(4);
    _etext = .;
  } >FLASH

  /* Initialized data copied from FLASH to RAM on startup */
  _sidata = LOADADDR(.data);

  .data :
  {
    . = ALIGN(4);
    _sdata = .;            /* Start of data section in RAM */
    *(.data)
    *(.data*)
    . = ALIGN(4);
    _edata = .;            /* End of data section in RAM */
  } >RAM AT> FLASH

  /* Zero-initialized data section in RAM */
  .bss :
  {
    . = ALIGN(4);
    _sbss = .;             /* Start of BSS section */
    *(.bss)
    *(.bss*)
    *(COMMON)
    . = ALIGN(4);
    _ebss = .;             /* End of BSS section */
  } >RAM

  /* Discard anything from standard libraries we don't need */
  /DISCARD/ :
  {
    libc.a ( * )
    libm.a ( * )
    libgcc.a ( * )
  }
} 