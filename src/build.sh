#!/bin/bash
# 이 스크립트는 Linux/WSL 환경 또는 Git Bash 등에서 실행해야 합니다.

# 1. 이전 빌드 파일 삭제
rm -f *.bin *.o *.dis

# 2. 부트로더 어셈블 (바이너리 포맷)
# 이전 단계에서 만든 boot.asm이 필요합니다.
nasm -f bin boot.asm -o boot.bin

# 3. 커널 엔트리 어셈블 (ELF 포맷)
# C 코드와 링크하기 위해 ELF 포맷으로 컴파일합니다.
nasm -f elf kernel_entry.asm -o kernel_entry.o

# 4. C 커널 컴파일 (Object 파일 생성)
# -ffreestanding: 표준 라이브러리를 사용하지 않음
# -m32: 32비트 코드로 컴파일 (호스트가 64비트일 경우 필수)
# -c: 링크하지 않고 컴파일만 수행
gcc -ffreestanding -m32 -fno-pie -c kmain.c -o kmain.o

# 5. 링킹 (Kernel Entry + Kernel Main -> Kernel Binary)
# -Ttext 0x1000: 코드가 메모리 0x1000에 로드된다고 가정하고 주소 계산
# --oformat binary: 실행 파일 헤더 없이 순수 기계어만 출력
ld -m elf_i386 -o kernel.bin -Ttext 0x1000 kernel_entry.o kmain.o --oformat binary

# 6. OS 이미지 생성 (부트로더 + 커널 바이너리)
cat boot.bin kernel.bin > os-image.bin

echo "Build Complete: os-image.bin created."
echo "Run with: qemu-system-i386 -fda os-image.bin"