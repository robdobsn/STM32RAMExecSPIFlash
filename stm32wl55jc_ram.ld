/* Pure RAM execution linker script for STM32WL55JC */

/* Entry point */
ENTRY(Reset_Handler)

/* Program header sections */
PHDRS {Loader PT_LOAD ; SgInfo PT_LOAD ; }

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
  } >RAM :Loader
  
  /* Code section in RAM */
  .text :
  {
    . = ALIGN(4);
    *(.text)             /* Code */
    *(.text*)
    *(.rodata)           /* Read-only data */
    *(.rodata*)
    . = ALIGN(4);
  } >RAM :Loader

  /* Data section in RAM */
  .data :
  {
    . = ALIGN(4);
    _sdata = .;
    *(.data)
    *(.data*)
    . = ALIGN(4);
    _edata = .;
  } >RAM :Loader

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
  } >RAM :Loader

  .Dev_Info :
  {
  	__Dev_info_START = .;
  	*(.Dev_info*)
  	KEEP(*(.Dev_info))
  	__Dev_info_END = .;
  } >RAM :SgInfo

  /* Discard anything from standard libraries we don't need */
  /DISCARD/ :
  {
    libc.a ( * )
    libm.a ( * )
    libgcc.a ( * )
  }
} 