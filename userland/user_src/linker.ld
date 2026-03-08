ENTRY(_start)
SECTIONS
{
    . = 0x400000;
    .text : { *(.text) }
    .data : { *(.data) }
    .bss  : { 
        *(.bss) 
        *(COMMON) /* Important: TCC/GCC put some variables here */
    }
    
    /* The Magic Markers */
    _end = .; 
    end = .;
}
