/* linker.ld */
ENTRY(_start)

MEMORY {
    /* IMEM: 16KB, nơi code được nạp vào qua AXI */
    IMEM (rx)  : ORIGIN = 0x00000000, LENGTH = 16K

    /* DMEM: 16KB, địa chỉ data memory của processor */
    DMEM (rwx) : ORIGIN = 0x00000000, LENGTH = 16K
}

SECTIONS {
    /* ===== Code section → nạp vào IMEM ===== */
    .text : {
        *(.text.startup)   /* _start phải ở đầu tiên */
        *(.text*)
        *(.rodata*)
    } > IMEM

    /* Padding để align */
    . = ALIGN(4);
    _text_end = .;

    /* ===== Data section → nạp vào DMEM ===== */
    /* LMA (load address) = ngay sau .text trong file binary  */
    /* VMA (virtual address) = địa chỉ thực tế processor dùng */
    .data : AT(_text_end) {
        _data_start = .;
        *(.data*)
        . = ALIGN(4);
        _data_end = .;
    } > DMEM

    .bss : {
        _bss_start = .;
        *(.bss*)
        *(COMMON)
        . = ALIGN(4);
        _bss_end = .;
    } > DMEM

    /* Stack tại cuối DMEM */
    _stack_top = ORIGIN(DMEM) + LENGTH(DMEM);
}