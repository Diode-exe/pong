ENTRY(kernel_main)
OUTPUT_FORMAT(elf32-i386)

SECTIONS {
    /* GRUB expects the code to be loaded at 1MB */
    . = 1M;

    .text : {
        /* This ensures the Multiboot header is at the very beginning */
        *(.multiboot)
        *(.text)
    }

    .data : { *(.data) }
    .bss  : { *(.bss)  }
}