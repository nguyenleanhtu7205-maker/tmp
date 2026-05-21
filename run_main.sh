#!/bin/bash
echo "=== Bien dich ==="
gcc -o main main.c
if [ $? -ne 0 ]; then
    echo "Bien dich that bai!"
    exit 1
fi

echo "=== Chay chuong trinh ==="
sudo ./main