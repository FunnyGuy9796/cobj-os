ENTRY(_start)

SECTIONS {
    . = 0x400000;

    .text : {
        ./build/userlib/crt0.o(.text)
        *(.text*)
    }
    
    .rodata : { *(.rodata*) }
    .data   : { *(.data*)   }
    .bss    : { *(.bss*)    }
}