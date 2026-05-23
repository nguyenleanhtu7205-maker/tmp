#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>

#define RV_IP_BASE    0xA0000000UL
#define MAP_SIZE      0x10000UL
#define MAP_MASK      (MAP_SIZE - 1)

#define REG_IMEM_BASE 0x0000
#define REG_CTRL      0x4000
#define REG_STATUS    0x4004
#define REG_RESULT    0x4008

#define IMEM_MAX_WORDS 4096   /* 16KB / 4 */

static volatile uint32_t* map_base = NULL;
static int mem_fd = -1;

void axi_open() {
    mem_fd = open("/dev/mem", O_RDWR | O_SYNC);
    if (mem_fd < 0) { perror("open /dev/mem"); exit(1); }
    map_base = (volatile uint32_t*)mmap(
        NULL, MAP_SIZE, PROT_READ | PROT_WRITE,
        MAP_SHARED, mem_fd, RV_IP_BASE
    );
    if (map_base == MAP_FAILED) { perror("mmap"); exit(1); }
}

void axi_close() {
    munmap((void*)map_base, MAP_SIZE);
    close(mem_fd);
}

void reg_write(uint32_t offset, uint32_t value) {
    map_base[offset / 4] = value;
}

uint32_t reg_read(uint32_t offset) {
    return map_base[offset / 4];
}

/* ============================================================
 * Đọc file .bin và trả về mảng uint32_t
 * Caller chịu trách nhiệm free() bộ nhớ
 * ============================================================ */
uint32_t* load_binary(const char* path, int* out_word_count) {
    FILE* f = fopen(path, "rb");
    if (!f) { perror(path); exit(1); }

    /* Lấy kích thước file */
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    rewind(f);

    if (size % 4 != 0) {
        fprintf(stderr, "Loi: file size (%ld) khong chia het cho 4!\n", size);
        fclose(f);
        exit(1);
    }

    int word_count = (int)(size / 4);
    if (word_count > IMEM_MAX_WORDS) {
        fprintf(stderr, "Loi: chuong trinh qua lon (%d words > %d)\n",
            word_count, IMEM_MAX_WORDS);
        fclose(f);
        exit(1);
    }

    uint32_t* buf = (uint32_t*)malloc(size);
    if (!buf) { perror("malloc"); exit(1); }

    if (fread(buf, 4, word_count, f) != (size_t)word_count) {
        perror("fread"); exit(1);
    }
    fclose(f);

    *out_word_count = word_count;
    return buf;
}

int main(int argc, char* argv[]) {
    /* Nhận đường dẫn .bin từ tham số dòng lệnh */
    const char* bin_path = (argc > 1) ? argv[1] : "program.bin";

    printf("=== RV32I Pipeline Processor - KR260 ===\n");
    printf("Binary: %s\n\n", bin_path);

    /* [0] Đọc binary */
    int word_count = 0;
    uint32_t* program = load_binary(bin_path, &word_count);
    printf("[0] Da doc %d lenh (binary: %d bytes)\n\n",
        word_count, word_count * 4);

    axi_open();

    /* [1] Giữ reset */
    printf("[1] Giu reset processor...\n");
    reg_write(REG_CTRL, 0x00000001);

    /* [2] Nạp chương trình vào IMEM */
    printf("[2] Nap %d lenh vao IMEM...\n", word_count);
    for (int i = 0; i < word_count; i++) {
        reg_write(REG_IMEM_BASE + i * 4, program[i]);
    }
    free(program);
    printf("    Nap xong.\n\n");

    /* [3] Thả reset, bật enable */
    printf("[3] Tha reset, bat processor...\n");
    reg_write(REG_CTRL, 0x00000002);

    /* [4] Poll ip_done */
    printf("[4] Doi ip_done = 1...\n");
    uint32_t status = 0;
    int count = 0;
    while (!(status & 0x1)) {
        status = reg_read(REG_STATUS);
        if (++count > 1000000) {
            printf("    [TIMEOUT]\n");
            axi_close();
            return 1;
        }
    }
    printf("    ip_done = 1 sau %d polls\n\n", count);

    /* [5] Đọc kết quả */
    uint32_t result = reg_read(REG_RESULT);
    printf("[5] ip_result = %u (0x%08X)\n", result, result);

    axi_close();
    return 0;
}