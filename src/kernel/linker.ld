ENTRY(start)
phys = 0x00100000;

SECTIONS
{
    . = phys;
    
    /* Entry point MUST be first */
    .entry : {
        __entry_start = .;
        *(.entry)
    }
    
    /* Code sections */
    .text : {
        __text_start = .;
        *(.text)
        *(.text.*)
    }
    
    /* Read-only data */
    .rodata : {
        __rodata_start = .;
        *(.rodata)
        *(.rodata.*)
    }
    
    /* Initialized data */
    .data : {
        __data_start = .;
        *(.data)
        *(.data.*)
    }
    
    /* Uninitialized data */
    .bss : {
        __bss_start = .;
        *(.bss)
        *(.bss.*)
        *(COMMON)
    }
    
    __end = .;
    
    /* Discard unnecessary sections */
    /DISCARD/ : {
        *(.note.*)
        *(.comment)
        *(.eh_frame)
        *(.eh_frame_hdr)
        *(.gnu.hash)
        *(.gnu.version*)
    }
}
