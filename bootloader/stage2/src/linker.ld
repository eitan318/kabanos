/* Stage2 Linker Script */
/* Stage2 loads at 0x7E00 */

OUTPUT_FORMAT("elf32-i386")
ENTRY(stage2_entry)

SECTIONS
{
    . = 0x9000;

    .entry : {
        *(.entry)
    }

    .text16 : {
        *(.text16)
    }

    .text : ALIGN(4) {
        *(.text)
        *(.text.*)
    }

    .rodata : ALIGN(4) {
        *(.rodata)
        *(.rodata.*)
    }

    .data : ALIGN(4) {
        *(.data)
        *(.data.*)
    }

    .bss : ALIGN(4) {
        __bss_start = .;
        *(.bss)
        *(.bss.*)
        *(COMMON)
        __bss_end = .;
        __end = .;  /* Add this symbol for compatibility */
    }

    /DISCARD/ : {
        *(.comment)
        *(.note*)
        *(.eh_frame)
    }
}
