#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>

// ============================================================
// Địa chỉ base của RV_IP_AXI_Wrapper trên AXI bus
// Lấy từ Vivado Block Design → Address Editor
// Ví dụ: Zynq GP0 AXI master thường map tại 0x4000_0000
// Giá trị chính xác phải xem trong Address Editor của bạn
// ============================================================
#define RV_IP_BASE    0xA0000000UL
#define MAP_SIZE      0x10000UL    // 64KB (bao phủ toàn bộ ADDR_W=16)
#define MAP_MASK      (MAP_SIZE - 1)

// Register offsets (theo register map của AXI Wrapper)
#define REG_IMEM_BASE 0x0000       // 0x0000~0x3FFF: IMEM
#define REG_CTRL      0x4000       // bit0=reset, bit1=enable
#define REG_STATUS    0x4004       // bit0=ip_done
#define REG_RESULT    0x4008       // ip_result

// ============================================================
// Chương trình RV32I (16 lệnh đã decode ở trên)
// ============================================================
uint32_t program[] = {
    0x080000B7,  // lui  ra,  0x08000     -> ra = 0x0800_0000
    0x01900113,  // addi sp,  x0, 25      -> operand_a = 25
    0x0020A023,  // sw   sp,  0(ra)       -> ALU_IP[0x00] = 25
    0x00A00193,  // addi gp,  x0, 10      -> operand_b = 10
    0x0030A223,  // sw   gp,  4(ra)       -> ALU_IP[0x04] = 10
    0x00000213,  // addi tp,  x0, 0       -> opcode ADD
    0x0040A423,  // sw   tp,  8(ra)       -> trigger ADD
    0x0100A283,  // lw   t0, 16(ra)       -> doc done flag
    0xFE028FE3,  // beq  t0,  x0, -4     -> poll done
    0x00C0A303,  // lw   t1, 12(ra)       -> result ADD -> t1
    0x00100213,  // addi tp,  x0, 1       -> opcode SUB
    0x0040A423,  // sw   tp,  8(ra)       -> trigger SUB
    0x0100A283,  // lw   t0, 16(ra)       -> doc done flag
    0xFE028FE3,  // beq  t0,  x0, -4     -> poll done
    0x00C0A383,  // lw   t2, 12(ra)       -> result SUB -> t2
    0x00000063,  // beq  x0,  x0, 0      -> halt
};
#define PROGRAM_LEN (sizeof(program) / sizeof(program[0]))

// ============================================================
// Hàm đọc/ghi thanh ghi AXI qua /dev/mem
// ============================================================
static volatile uint32_t* map_base = NULL;
static int mem_fd = -1;

void axi_open() {
    mem_fd = open("/dev/mem", O_RDWR | O_SYNC);
    if (mem_fd < 0) {
        perror("Khong mo duoc /dev/mem");
        exit(1);
    }
    map_base = (volatile uint32_t*)mmap(
        NULL, MAP_SIZE,
        PROT_READ | PROT_WRITE,
        MAP_SHARED,
        mem_fd,
        RV_IP_BASE
    );
    if (map_base == MAP_FAILED) {
        perror("mmap that bai");
        exit(1);
    }
}

void axi_close() {
    munmap((void*)map_base, MAP_SIZE);
    close(mem_fd);
}

void reg_write(uint32_t offset, uint32_t value) {
    // offset tính bằng byte, chia 4 vì map_base là uint32_t*
    map_base[offset / 4] = value;
}

uint32_t reg_read(uint32_t offset) {
    return map_base[offset / 4];
}

// ============================================================
// Main
// ============================================================
int main() {
    printf("=== RV32I Pipeline Processor - KR260 ===\n\n");

    // Mở /dev/mem và map địa chỉ
    axi_open();

    // --------------------------------------------------
    // Bước 1: Giữ reset trước khi nạp chương trình
    // --------------------------------------------------
    printf("[1] Giu reset processor...\n");
    reg_write(REG_CTRL, 0x00000001);  // reset=1, enable=0

    // --------------------------------------------------
    // Bước 2: Nạp chương trình vào IMEM
    // --------------------------------------------------
    printf("[2] Nap %d lenh vao IMEM...\n", PROGRAM_LEN);
    for (int i = 0; i < PROGRAM_LEN; i++) {
        reg_write(REG_IMEM_BASE + i * 4, program[i]);
        printf("    [0x%04X] = 0x%08X\n", i * 4, program[i]);
    }

    // --------------------------------------------------
    // Bước 3: Thả reset, bật enable
    // --------------------------------------------------
    printf("[3] Tha reset, bat processor...\n");
    reg_write(REG_CTRL, 0x00000002);  // reset=0, enable=1

    // --------------------------------------------------
    // Bước 4: Chờ ip_done = 1
    // --------------------------------------------------
    printf("[4] Doi processor hoan thanh (poll STATUS)...\n");
    uint32_t status = 0;
    int      count = 0;
    while ((status & 0x1) == 0) {
        status = reg_read(REG_STATUS);
        count++;
        if (count > 1000000) {
            printf("    [TIMEOUT] Processor khong phan hoi!\n");
            axi_close();
            return 1;
        }
    }
    printf("    ip_done = 1 sau %d lan poll\n", count);

    // --------------------------------------------------
    // Bước 5: Đọc kết quả
    // --------------------------------------------------
    uint32_t result = reg_read(REG_RESULT);
    printf("[5] Ket qua tu ALU_IP: %u (0x%08X)\n", result, result);

    // --------------------------------------------------
    // Kiểm tra
    // --------------------------------------------------
    printf("\n=== Kiem tra ===\n");
    printf("  ADD (25 + 10) = 35 : duoc tinh trong processor\n");
    printf("  SUB (25 - 10) = 15 : ");
    if (result == 15)
        printf("PASS (result = %u)\n", result);
    else
        printf("FAIL (result = %u, expected = 15)\n", result);

    // Đọc CTRL để xác nhận AXI read hoạt động
    uint32_t ctrl = reg_read(REG_CTRL);
    printf("  CTRL register     = 0x%08X : ", ctrl);
    if (ctrl == 0x00000002)
        printf("PASS\n");
    else
        printf("FAIL (expected 0x00000002)\n");

    axi_close();
    printf("\nHoan thanh.\n");
    return 0;
}