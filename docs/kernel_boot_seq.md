KERNEL BOOTING SEQUENCE
-----------------------

* Start in entry.asm
* Creating page table with:
- low memory (0 -> 1MiB) [contains stage2 stack, boot params (including cmdline, mem-map and more)]
- kernel memory (kernel start -> kernel end
* Create empty page dir with table identity mapped into:
- lower half (0x00000000 -> 0x00000000) 
- heigher half (0xC0000000 -> 0x00000000) 
* Set cr3 with page dir
* Enable paging in CPU
* Far-jump to heigher half
* jump to high half c kmain (kernel.c)
* Initialized early pmm
* Early-allocate boot_params copy,  (including cmdline, mem-map and more)
* Memcopy bootparams and related from the low, id-mapped memory into their high-half copyes
* Unmap all idmaping to low half (low mem and kernel code)
* Early-allocate main pmm structures
* Early-allocate kernel addr space struct
* Init main pmm (No more early pmm, because this is the 
    last and only time the memory it used is marked as used)
* Fill main kernel addr space struct (how to initially map stuff?)
* Switch to this addr space
* Initializing heap


    




