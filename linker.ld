OUTPUT_FORMAT(elf64-x86-64)
ENTRY(kmain)

PHDRS {
    limine_requests PT_LOAD;
    text PT_LOAD;
    rodata PT_LOAD;
    data PT_LOAD;
}

SECTIONS {
    . = 0xffffffff80000000;

    __kernel_start = .;

    .limine_requests : {
        KEEP(*(.limine_requests_start))
        KEEP(*(.limine_base_revision))
        KEEP(*(.limine_mmap_request))
        KEEP(*(.limine_hhdm_request))
        KEEP(*(.limine_executable_address_request))
        KEEP(*(.limine_module_request))
        KEEP(*(.limine_requests_end))
    } :limine_requests

    . = ALIGN(CONSTANT(MAXPAGESIZE));

    .text : {
        *(.text .text.*)
    } :text

    . = ALIGN(CONSTANT(MAXPAGESIZE));

    .rodata : {
        *(.rodata .rodata.*)
    } :rodata

    . = ALIGN(CONSTANT(MAXPAGESIZE));

    .data : {
        *(.data .data.*)
    } :data

    .bss : {
        *(COMMON)
        *(.bss .bss.*)
    } :data

    __kernel_end = .;

    /DISCARD/ : {
        *(.eh_frame*)
        *(.note .note.*)
    }
}