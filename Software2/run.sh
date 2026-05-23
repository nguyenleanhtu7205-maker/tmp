#!/bin/bash
set -e   # dừng ngay nếu có lỗi

PROGRAM_SRC="program.c"
BIN_FILE="program.bin"
HOST_CC="gcc"
RV_MAKE="make"

echo "=== [1] Build RV32I program ==="
$RV_MAKE clean
$RV_MAKE all
echo ""

echo "=== [2] Build host control program ==="
$HOST_CC -o main main.c
echo ""

echo "=== [3] Nap bitstream (neu can) ==="
# sudo bash -c "cat system.bit > /lib/firmware/system.bit"
# sudo fpgautil -b /lib/firmware/system.bit
echo "    (bo qua buoc nap bit - da nap truoc)"
echo ""

echo "=== [4] Chay ==="
sudo ./main $BIN_FILE