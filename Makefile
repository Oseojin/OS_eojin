BUILD_DIR = build
SRC_BOOT = src/boot
SRC_KERNEL = src/kernel

C_SOURCES = $(wildcard $(SRC_KERNEL)/*.c)
OBJ_FILES = $(patsubst $(SRC_KERNEL)/%.c, $(BUILD_DIR)/%.o, $(C_SOURCES))

run: $(BUILD_DIR)/os-image.bin
	qemu-system-x86_64 -hda $<

all: $(BUILD_DIR)/os-image.bin


$(BUILD_DIR)/os-image.bin: $(BUILD_DIR)/boot.bin $(BUILD_DIR)/kernel.bin
	cat $^ > $@
	truncate -s 20480 $@

$(BUILD_DIR)/boot.bin: $(SRC_BOOT)/boot.asm
	nasm -f bin $< -o $@

$(BUILD_DIR)/kernel.bin: $(BUILD_DIR)/kernel_entry.o $(OBJ_FILES)
	ld -m elf_i386 -o $@ -Ttext 0x1000 $(BUILD_DIR)/kernel_entry.o $(OBJ_FILES) --oformat binary

$(BUILD_DIR)/kernel_entry.o: $(SRC_KERNEL)/kernel_entry.asm
	nasm -f elf $< -o $@

$(BUILD_DIR)/%.o: $(SRC_KERNEL)/%.c
	gcc -m32 -ffreestanding -fno-pie -c $< -o $@

clean:
	rm -f $(BUILD_DIR)/*.bin $(BUILD_DIR)/*.o
