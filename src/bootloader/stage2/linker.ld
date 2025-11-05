/* Stage2 Linker Script */
/* Stage2 loads at 0x7E00 */

OUTPUT_FORMAT("elf32-i386")
ENTRY(stage2_entry)

SECTIONS
{
    /* Where stage2 is loaded in memory */
    . = 0x7E00;

    /* 16-bit entry code */
    .entry : {
        *(.entry)
    }

    /* Real-mode helper code */
    .text16 : {
        *(.text16)
    }

    /* 32-bit protected-mode code */
    .text : ALIGN(4) {
        *(.text)
        *(.text.*)
    }

    /* Read-only data */
    .rodata : ALIGN(4) {
        *(.rodata)
        *(.rodata.*)
    }

    /* Initialized data */
    .data : ALIGN(4) {
        *(.data)
        *(.data.*)
    }

    /* BSS */
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
