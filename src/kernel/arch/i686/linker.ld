OUTPUT_FORMAT("elf32-i386")
ENTRY (kernel_start)
KERNEL_BASE_VIRT = 0xC0000000;
KERNEL_BASE_PHYS = 0x00100000;

SECTIONS
{
    . = KERNEL_BASE_PHYS;
    .multiboot.data : {
        *(.multiboot.data)
    }

   .multiboot.text : {
       *(.multiboot.text)
    }

	. += KERNEL_BASE_VIRT;

	_kernel_start = .;

	.text ALIGN (4K) : AT (ADDR (.text) - KERNEL_BASE_VIRT)
	{
		*(.text)
	}
	.rodata ALIGN (4K) : AT (ADDR (.rodata) - KERNEL_BASE_VIRT)
	{
		*(.rodata)
	}
	.data ALIGN (4K) : AT (ADDR (.data) - KERNEL_BASE_VIRT)
	{
		*(.data)
	}

    _bss_start = .;
    .bss ALIGN(4K) : AT(ADDR(.bss) - KERNEL_BASE_VIRT)
    {
        *(COMMON)
        *(.bss)
    }
    _bss_end = .;

    .bootstrap_stack ALIGN(16) : { *(.bootstrap_stack) }

	_kernel_end = .;
}

