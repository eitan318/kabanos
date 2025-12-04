SECTIONS {
  . = 0x100000;
  
  _kernel_start = .;
  
  .text : {
    _text_start = .;
    *(.text*)
    _text_end = .;
  }
  
  .data : {
    _data_start = .;
    *(.data*)
    _data_end = .;
  }
  
  .bss : {
    _bss_start = .;
    *(.bss*)
    _bss_end = .;
  }
  
  _kernel_end = .;
}
