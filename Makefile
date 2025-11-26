# -----------------------------------------------------------------------------
# OS Development Project Makefile (Directory Structured)
# -----------------------------------------------------------------------------

# 디렉토리 정의
SRC_DIR = src
INCLUDE_DIR = include
BUILD_DIR = build

# 컴파일러 및 링커 설정
CC = gcc
LD = ld
ASM = nasm

# 컴파일 플래그
# -I$(INCLUDE_DIR): 헤더 파일을 include 디렉토리에서 찾도록 설정
CFLAGS = -ffreestanding -m32 -fno-pie -g -I$(INCLUDE_DIR)

# 링킹 플래그
LDFLAGS = -m elf_i386 -Ttext 0x1000 --oformat binary

# 소스 파일 목록 찾기
# src 디렉토리 내의 모든 .c 파일
C_SOURCES = $(wildcard $(SRC_DIR)/*.c)
# 소스 파일 경로를 기반으로 빌드 디렉토리 내의 오브젝트 파일 경로 생성
# 예: src/main.c -> build/main.o
C_OBJECTS = $(patsubst $(SRC_DIR)/%.c, $(BUILD_DIR)/%.o, $(C_SOURCES))

# 어셈블리 오브젝트 파일 (커널 링크용)
# 커널 진입점(kernel_entry)과 인터럽트 핸들러 등
# 주의: kernel_entry.o는 링킹 시 가장 먼저 위치해야 함
ASM_SOURCES = $(SRC_DIR)/kernel_entry.asm $(SRC_DIR)/interrupts.asm
ASM_OBJECTS = $(patsubst $(SRC_DIR)/%.asm, $(BUILD_DIR)/%.o, $(ASM_SOURCES))

# 최종 타겟: OS 이미지 (build 디렉토리에 생성)
all: $(BUILD_DIR)/os-image.bin

# QEMU 실행
run: all
	qemu-system-i386 -fda $(BUILD_DIR)/os-image.bin

# OS 이미지 생성 (부트로더 + 커널)
$(BUILD_DIR)/os-image.bin: $(BUILD_DIR)/boot.bin $(BUILD_DIR)/kernel.bin
	cat $^ > $@

# 커널 바이너리 생성 (링킹)
# 중요: $(BUILD_DIR)/kernel_entry.o가 반드시 맨 앞에 와야 합니다.
# ASM_OBJECTS 리스트의 순서를 따르거나 명시적으로 적어주어야 합니다.
# 여기서는 ASM_OBJECTS 변수에 kernel_entry가 먼저 정의되어 있다고 가정합니다.
$(BUILD_DIR)/kernel.bin: $(ASM_OBJECTS) $(C_OBJECTS)
	$(LD) $(LDFLAGS) -o $@ $^

# C 소스 파일 컴파일 규칙
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# 어셈블리 파일 컴파일 규칙 (ELF 포맷 - 커널용)
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.asm
	@mkdir -p $(BUILD_DIR)
	$(ASM) -f elf $< -o $@

# 부트로더 컴파일 규칙 (BIN 포맷 - 단독 실행)
$(BUILD_DIR)/boot.bin: $(SRC_DIR)/boot.asm
	@mkdir -p $(BUILD_DIR)
	$(ASM) -f bin $< -o $@

# 빌드 파일 정리 (build 디렉토리 전체 삭제)
clean:
	rm -rf $(BUILD_DIR)

# .PHONY 설정
.PHONY: all run clean