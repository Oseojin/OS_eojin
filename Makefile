BUILD_DIR = build
SRC_BOOT = src/boot
SRC_KERNEL = src/kernel
SRC_DRIVERS = src/drivers
SRC_FS = src/fs

KERNEL_SRCS = $(wildcard $(SRC_KERNEL)/*.c)
DRIVER_SRCS = $(wildcard $(SRC_DRIVERS)/*.c)
FS_SRCS = $(wildcard $(SRC_FS)/*.c)
ASM_SRCS = $(filter-out $(SRC_KERNEL)/kernel_entry.asm, $(wildcard $(SRC_KERNEL)/*.asm))

KERNEL_OBJS = $(patsubst $(SRC_KERNEL)/%.c, $(BUILD_DIR)/%.o, $(KERNEL_SRCS))
DRIVER_OBJS = $(patsubst $(SRC_DRIVERS)/%.c, $(BUILD_DIR)/%.o, $(DRIVER_SRCS))
FS_OBJS = $(patsubst $(SRC_FS)/%.c, $(BUILD_DIR)/%.o, $(FS_SRCS))
ASM_OBJS = $(patsubst $(SRC_KERNEL)/%.asm, $(BUILD_DIR)/%.o, $(ASM_SRCS))

DISK_SIZE_KB = 10240

run: $(BUILD_DIR)/os-image.bin
	qemu-system-x86_64 -drive file=$<,format=raw,index=0,media=disk

rerun: clean $(BUILD_DIR)/os-image.bin
	qemu-system-x86_64 -hda $(BUILD_DIR)/os-image.bin

all: $(BUILD_DIR)/os-image.bin

$(BUILD_DIR)/system.bin: $(BUILD_DIR)/loader.bin $(BUILD_DIR)/kernel.bin
	cat $^ > $@

$(BUILD_DIR)/os-image.bin: $(BUILD_DIR)/boot_fat16.bin $(BUILD_DIR)/system.bin
	# 빈 파일 생성 (10MB)
	dd if=/dev/zero of=$@ bs=1k count=$(DISK_SIZE_KB)

	# FAT16 포맷 (mkfs.fat)
	# -F 16: FAT16
	# -R 1: Reserved Sector 1개 (부트섹터용)
	mkfs.fat -F 16 -R 1 -s 1 -S 512 -r 512 -n "OS_EOJIN" $@

	# 부트로더 덮어쓰기 (conv=notrunc: 파일 크기 유지)
	# mkfs.fat에서 생성한 BPB 정보를 유지하면서 코드만 복사해야함.
	# 부트섹터의 앞부분 (JMP 명령, 3바이트) 복사
	dd if=$(BUILD_DIR)/boot_fat16.bin of=$@ bs=1 count=3 conv=notrunc

	# 부트섹터의 뒷부분 (BPB 이후 코드, 62바이트부터 끝까지) 복사
	dd if=$(BUILD_DIR)/boot_fat16.bin of=$@ bs=1 skip=62 seek=62 conv=notrunc

	# 파일 복사 (mcopy)
	# -i: 이미지 파일 지정
	# :: 경로 사용
	mcopy -i $@ $(BUILD_DIR)/system.bin ::LOADER.BIN

$(BUILD_DIR)/boot_fat16.bin: $(SRC_BOOT)/boot_fat16.asm
	nasm -f bin $< -o $@

$(BUILD_DIR)/loader.bin: $(SRC_BOOT)/loader.asm
	nasm -f bin $< -o $@

$(BUILD_DIR)/kernel.bin: $(BUILD_DIR)/kernel_entry.o $(KERNEL_OBJS) $(DRIVER_OBJS) $(FS_OBJS) $(ASM_OBJS)
	ld -m elf_x86_64 -o $(BUILD_DIR)/kernel.elf -Ttext 0x8600 $(BUILD_DIR)/kernel_entry.o $(KERNEL_OBJS) $(DRIVER_OBJS) $(FS_OBJS) $(ASM_OBJS)
	objcopy -O binary $(BUILD_DIR)/kernel.elf $@

$(BUILD_DIR)/kernel_entry.o: $(SRC_KERNEL)/kernel_entry.asm
	nasm -f elf64 $< -o $@

$(BUILD_DIR)/%.o: $(SRC_KERNEL)/%.asm
	nasm -f elf64 $< -o $@

$(BUILD_DIR)/%.o: $(SRC_KERNEL)/%.c
	gcc -m64 -ffreestanding -mcmodel=large -mno-red-zone -mno-mmx -mno-sse -mno-sse2 -fno-pie -c $< -o $@

$(BUILD_DIR)/%.o: $(SRC_FS)/%.c
	gcc -m64 -ffreestanding -mcmodel=large -mno-red-zone -mno-mmx -mno-sse -mno-sse2 -fno-pie -c $< -o $@

$(BUILD_DIR)/%.o: $(SRC_DRIVERS)/%.c
	gcc -m64 -ffreestanding -mcmodel=large -mno-red-zone -mno-mmx -mno-sse -mno-sse2 -fno-pie -c $< -o $@

clean:
	rm -f $(BUILD_DIR)/*.bin $(BUILD_DIR)/*.o

.PHONY: run rerun all clean