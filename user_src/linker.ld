ENTRY(_start)
SECTIONS
{
    . = 0x400000; /* Standard starting address for many ELFs */
    .text : { *(.text) }
    .data : { *(.data) }
    .bss  : { *(.bss) }
}
