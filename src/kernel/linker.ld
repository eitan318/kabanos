OUTPUT_FORMAT("elf32-i386")
ENTRY(start)

SECTIONS {
  . = 0x100000;
  
  _kernel_start = .;
  
  /* CRITICAL: Entry point MUST come first at 0x100000 */
  .entry : {
    *(.entry)
  }
  
  .text : {
    _text_start = .;
    *(.text*)
    _text_end = .;
  }
  
  .rodata : {
    *(.rodata*)
  }
  
  .data : {
    _data_start = .;
    *(.data*)
    _data_end = .;
  }
  
  .bss : ALIGN(16) {
    _bss_start = .;
    *(.bss*)
    *(COMMON)
    _bss_end = .;
  }
  
  _kernel_end = .;
  
  /DISCARD/ : {
    *(.comment)
    *(.note*)
    *(.eh_frame)
  }
}