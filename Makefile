BUILD_DIR = build
# Ensure build directory exists automatically
$(shell mkdir -p $(BUILD_DIR))

SRC_BOOT = src/boot
SRC_KERNEL = src/kernel
SRC_DRIVERS = src/drivers
SRC_FS = src/fs
SRC_PROCESS = src/kernel/process
SRC_CPU = src/kernel/cpu

KERNEL_SRCS = $(wildcard $(SRC_KERNEL)/*.c)
DRIVER_SRCS = $(wildcard $(SRC_DRIVERS)/*.c)
FS_SRCS = $(wildcard $(SRC_FS)/*.c)
PROCESS_SRCS = $(wildcard $(SRC_PROCESS)/*.c)
CPU_SRCS = $(wildcard $(SRC_CPU)/*.c)
ASM_SRCS = $(filter-out $(SRC_KERNEL)/kernel_entry.asm, $(wildcard $(SRC_KERNEL)/*.asm) $(wildcard $(SRC_CPU)/*.asm))

KERNEL_OBJS = $(patsubst $(SRC_KERNEL)/%.c, $(BUILD_DIR)/%.o, $(KERNEL_SRCS))
DRIVER_OBJS = $(patsubst $(SRC_DRIVERS)/%.c, $(BUILD_DIR)/%.o, $(DRIVER_SRCS))
FS_OBJS = $(patsubst $(SRC_FS)/%.c, $(BUILD_DIR)/%.o, $(FS_SRCS))
PROCESS_OBJS = $(patsubst $(SRC_PROCESS)/%.c, $(BUILD_DIR)/%.o, $(PROCESS_SRCS))
CPU_OBJS = $(patsubst $(SRC_CPU)/%.c, $(BUILD_DIR)/%.o, $(CPU_SRCS))
ASM_OBJS = $(patsubst $(SRC_KERNEL)/%.asm, $(BUILD_DIR)/%.o, $(patsubst $(SRC_CPU)/%.asm, $(BUILD_DIR)/%.o, $(ASM_SRCS)))

# ASM_OBJS is tricky with patsubst because source paths vary.
# Let's simplify ASM object generation rule.
# ASM sources are in src/kernel/ and src/kernel/cpu/

ASM_KERNEL_SRCS = $(wildcard $(SRC_KERNEL)/*.asm)
ASM_CPU_SRCS = $(wildcard $(SRC_CPU)/*.asm)
ASM_OBJS = $(patsubst $(SRC_KERNEL)/%.asm, $(BUILD_DIR)/%.o, $(filter-out $(SRC_KERNEL)/kernel_entry.asm, $(ASM_KERNEL_SRCS))) \
           $(patsubst $(SRC_CPU)/%.asm, $(BUILD_DIR)/%.o, $(ASM_CPU_SRCS))

DISK_SIZE_KB = 10240

all: $(BUILD_DIR)/os-image.bin

run: $(BUILD_DIR)/os-image.bin
	qemu-system-x86_64 -m 512M -drive file=$<,format=raw,index=0,media=disk

rerun: clean $(BUILD_DIR)/os-image.bin
	qemu-system-x86_64 -m 512M -hda $(BUILD_DIR)/os-image.bin

$(BUILD_DIR)/system.bin: $(BUILD_DIR)/loader.bin $(BUILD_DIR)/kernel.bin
	cat $^ > $@

$(BUILD_DIR)/os-image.bin: $(BUILD_DIR)/boot_fat16.bin $(BUILD_DIR)/system.bin $(BUILD_DIR)/user.bin $(BUILD_DIR)/hello.elf
	dd if=/dev/zero of=$@ bs=1k count=$(DISK_SIZE_KB)

	mkfs.fat -F 16 -R 1 -s 1 -S 512 -r 512 -n "OS_EOJIN" $@

	dd if=$(BUILD_DIR)/boot_fat16.bin of=$@ bs=1 count=3 conv=notrunc

	dd if=$(BUILD_DIR)/boot_fat16.bin of=$@ bs=1 skip=62 seek=62 conv=notrunc

	mcopy -i $@ $(BUILD_DIR)/system.bin ::LOADER.BIN
	mcopy -i $@ $(BUILD_DIR)/user.bin ::USER.BIN
	mcopy -i $@ $(BUILD_DIR)/hello.elf ::HELLO.ELF
	echo "Hello, OS_EOJIN!" > $(BUILD_DIR)/hello.txt
	mcopy -i $@ $(BUILD_DIR)/hello.txt ::HELLO.TXT
	
	mmd -i $@ ::TESTDIR
	mcopy -i $@ $(BUILD_DIR)/hello.txt ::TESTDIR/INNER.TXT

$(BUILD_DIR)/user.bin: src/user/user.asm
	nasm -f bin $< -o $@

# ELF User Program
$(BUILD_DIR)/hello.o: src/user/hello.c
	gcc -m64 -ffreestanding -fno-pie -c $< -o $@

$(BUILD_DIR)/hello.elf: $(BUILD_DIR)/hello.o
	# Link at 0x1000000 (16MB), Entry: _start
	ld -m elf_x86_64 -Ttext 0x1000000 -e _start -o $@ $<

$(BUILD_DIR)/boot_fat16.bin: $(SRC_BOOT)/boot_fat16.asm
	nasm -f bin $< -o $@

$(BUILD_DIR)/loader.bin: $(SRC_BOOT)/loader.asm
	nasm -f bin $< -o $@

$(BUILD_DIR)/kernel.bin: $(BUILD_DIR)/kernel_entry.o $(KERNEL_OBJS) $(DRIVER_OBJS) $(FS_OBJS) $(PROCESS_OBJS) $(CPU_OBJS) $(ASM_OBJS)
	ld -m elf_x86_64 -o $(BUILD_DIR)/kernel.elf -Ttext 0x8600 $(BUILD_DIR)/kernel_entry.o $(KERNEL_OBJS) $(DRIVER_OBJS) $(FS_OBJS) $(PROCESS_OBJS) $(CPU_OBJS) $(ASM_OBJS)
	objcopy -O binary $(BUILD_DIR)/kernel.elf $@

$(BUILD_DIR)/kernel_entry.o: $(SRC_KERNEL)/kernel_entry.asm
	nasm -f elf64 $< -o $@

$(BUILD_DIR)/%.o: $(SRC_KERNEL)/%.asm
	nasm -f elf64 $< -o $@

$(BUILD_DIR)/%.o: $(SRC_CPU)/%.asm
	nasm -f elf64 $< -o $@

$(BUILD_DIR)/%.o: $(SRC_KERNEL)/%.c
	gcc -m64 -ffreestanding -mcmodel=large -mno-red-zone -mno-mmx -mno-sse -mno-sse2 -fno-pie -c $< -o $@

$(BUILD_DIR)/%.o: $(SRC_FS)/%.c
	gcc -m64 -ffreestanding -mcmodel=large -mno-red-zone -mno-mmx -mno-sse -mno-sse2 -fno-pie -c $< -o $@

$(BUILD_DIR)/%.o: $(SRC_DRIVERS)/%.c
	gcc -m64 -ffreestanding -mcmodel=large -mno-red-zone -mno-mmx -mno-sse -mno-sse2 -fno-pie -c $< -o $@

$(BUILD_DIR)/%.o: $(SRC_PROCESS)/%.c
	gcc -m64 -ffreestanding -mcmodel=large -mno-red-zone -mno-mmx -mno-sse -mno-sse2 -fno-pie -c $< -o $@

$(BUILD_DIR)/%.o: $(SRC_CPU)/%.c
	gcc -m64 -ffreestanding -mcmodel=large -mno-red-zone -mno-mmx -mno-sse -mno-sse2 -fno-pie -c $< -o $@

clean:
	rm -f $(BUILD_DIR)/*.bin $(BUILD_DIR)/*.o $(BUILD_DIR)/*.elf

.PHONY: run rerun all clean