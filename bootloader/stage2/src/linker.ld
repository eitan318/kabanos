OUTPUT_FORMAT("elf32-i386")
ENTRY(stage2_entry)

SECTIONS
{
    /* Stage 2 starts at 0x9000 */
    . = 0x9000;

    /* Combine all code and read-only data into one segment */
    .text : {
        *(.entry)
        *(.text16)
        *(.text .text.*)
        *(.rodata .rodata.*)
    }

    .data : ALIGN(4) {
        *(.data .data.*)
    }

    .bss : ALIGN(4) {
        __bss_start = .;
        *(.bss .bss.*)
        *(COMMON)
        __bss_end = .;
    }

    /* --- THE ALIGNMENT FIX --- */
    /* This forces the output file to be a multiple of 512 bytes */
    .padding : {
        FILL(0x00);
        . = ALIGN(512);
        /* We place a single byte at the very end of the last sector 
           to ensure the linker doesn't 'optimize out' the empty space */
        BYTE(0x00)
        __end = .;
    }

    /DISCARD/ : { *(.comment) *(.note*) *(.eh_frame) }
}
