ENTRY(Reset_Handler)
_estack = ORIGIN(SRAM) + LENGTH(SRAM); /* end of "RAM" Ram type memory */
_Min_Heap_Size = 0x200; /* required amount of heap */
_Min_Stack_Size = 0x400; /* required amount of stack */

MEMORY
{
  FLASH(rx):ORIGIN =0x08000000,LENGTH =64K
  SRAM(rwx):ORIGIN =0x20000000,LENGTH =20K
}

SECTIONS
{
  .text :
  {
    KEEP (*(.isr_vector))
    *(.text)
    *(.text.*)
    *(.init)
    *(.fini)
    *(.rodata)
    *(.rodata.*)
    . = ALIGN(4);
    _etext = .;
  }> FLASH

  _la_data = LOADADDR(.data);

  .data :
  {
    _sdata = .;
    *(.data)
    *(.data.*)
    . = ALIGN(4);
    _edata = .;
  }> SRAM AT> FLASH

  .bss :
  {
    _sbss = .;
    __bss_start__ = _sbss;
    *(.bss)
    *(.bss.*)
    *(COMMON)
    . = ALIGN(4);
    _ebss = .;
    __bss_end__ = _ebss;
     . = ALIGN(4);
    end = .;
    __end__ = .;
  }> SRAM

  ._user_heap_stack :
  {
    . = ALIGN(8);
    PROVIDE ( end = . );
    PROVIDE ( _end = . );
    . = . + _Min_Heap_Size;
    . = . + _Min_Stack_Size;
    . = ALIGN(8);
  } >SRAM
  
}
