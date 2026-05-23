/* program.c — chương trình chạy trên RV32I processor */
#include <stdint.h>

/* Địa chỉ ALU_IP theo BusInterconnect:
   sel_ip = addr[31:8] == 24'h080000
   → base = 0x08000000                    */
#define ALU_IP_BASE   0x08000000UL

#define ALU_OPERAND_A  (*(volatile uint32_t*)(ALU_IP_BASE + 0x00))
#define ALU_OPERAND_B  (*(volatile uint32_t*)(ALU_IP_BASE + 0x04))
#define ALU_OPCODE     (*(volatile uint32_t*)(ALU_IP_BASE + 0x08))
#define ALU_RESULT     (*(volatile uint32_t*)(ALU_IP_BASE + 0x0C))
#define ALU_DONE       (*(volatile uint32_t*)(ALU_IP_BASE + 0x10))

#define ALU_ADD  0
#define ALU_SUB  1

   /* Kết quả lưu vào DMEM để ARM đọc ra sau */
volatile uint32_t result_add __attribute__((section(".data")));
volatile uint32_t result_sub __attribute__((section(".data")));

static uint32_t alu_compute(uint32_t a, uint32_t b, uint32_t op) {
    ALU_OPERAND_A = a;
    ALU_OPERAND_B = b;
    ALU_OPCODE = op;           /* trigger tính toán */
    while (!ALU_DONE);            /* poll cho đến khi xong */
    return ALU_RESULT;
}

int main(void) {
    result_add = alu_compute(25, 10, ALU_ADD);   /* 25 + 10 = 35 */
    result_sub = alu_compute(25, 10, ALU_SUB);   /* 25 - 10 = 15 */

    /* Halt — crt0.S sẽ nhảy vào vòng lặp halt sau khi return */
    return 0;
}