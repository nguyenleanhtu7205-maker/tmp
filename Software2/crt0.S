# crt0.S — khởi động processor trước khi vào main()
    .section .text.startup
    .global  _start

_start:
    # Khởi tạo stack pointer về cuối DMEM (0x00003FFC)
    lui  sp, %hi(_stack_top)
    addi sp, sp, %lo(_stack_top)

    # Zero-fill vùng BSS
    lui  a0, %hi(_bss_start)
    addi a0, a0, %lo(_bss_start)
    lui  a1, %hi(_bss_end)
    addi a1, a1, %lo(_bss_end)
bss_loop:
    bge  a0, a1, bss_done
    sw   zero, 0(a0)
    addi a0, a0, 4
    j    bss_loop
bss_done:

    # Gọi main
    call main

    # Halt: vòng lặp vô tận sau khi main() trả về
halt:
    j halt