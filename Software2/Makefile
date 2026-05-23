# ============================================================
# Makefile — build RV32I bare-metal program
# ============================================================
TOOLCHAIN = riscv64-unknown-elf
CC        = $(TOOLCHAIN)-gcc
OBJCOPY   = $(TOOLCHAIN)-objcopy
OBJDUMP   = $(TOOLCHAIN)-objdump

# -march=rv32i: chỉ dùng base integer ISA
# -mabi=ilp32:  32-bit int/long/pointer
# -nostdlib:    không dùng stdlib (không có OS)
# -ffreestanding: môi trường bare-metal
CFLAGS = -march=rv32i -mabi=ilp32 \
         -nostdlib -ffreestanding  \
         -O1 -Wall                 \
         -T linker.ld

TARGET = program

all: $(TARGET).bin

$(TARGET).elf: crt0.S program.c linker.ld
	$(CC) $(CFLAGS) -o $@ crt0.S program.c

# Chỉ lấy phần .text (IMEM) — từ địa chỉ 0x0 đến _text_end
$(TARGET).bin: $(TARGET).elf
	$(OBJCOPY) -O binary \
	    --only-section=.text \
	    --only-section=.rodata \
	    $< $@
	@echo "Binary size: $$(wc -c < $@) bytes"
	@echo "Instruction count: $$(( $$(wc -c < $@) / 4 ))"

# Xem disassembly để debug
dump: $(TARGET).elf
	$(OBJDUMP) -d -M numeric $

clean:
	rm -f $(TARGET).elf $(TARGET).bin

.PHONY: all dump clean