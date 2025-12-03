BUILD_DIR = build
SRC_BOOT = src/boot
SRC_KERNEL = src/kernel
SRC_DRIVERS = src/drivers

KERNEL_SRCS = $(wildcard $(SRC_KERNEL)/*.c)
DRIVER_SRCS = $(wildcard $(SRC_DRIVERS)/*.c)
ASM_SRCS = $(filter-out $(SRC_KERNEL)/kernel_entry.asm, $(wildcard $(SRC_KERNEL)/*.asm))

KERNEL_OBJS = $(patsubst $(SRC_KERNEL)/%.c, $(BUILD_DIR)/%.o, $(KERNEL_SRCS))
DRIVER_OBJS = $(patsubst $(SRC_DRIVERS)/%.c, $(BUILD_DIR)/%.o, $(DRIVER_SRCS))
ASM_OBJS = $(patsubst $(SRC_KERNEL)/%.asm, $(BUILD_DIR)/%.o, $(ASM_SRCS))

run: $(BUILD_DIR)/os-image.bin
	qemu-system-x86_64 -hda $<

rerun: clean $(BUILD_DIR)/os-image.bin
	qemu-system-x86_64 -hda $(BUILD_DIR)/os-image.bin

all: $(BUILD_DIR)/os-image.bin

$(BUILD_DIR)/os-image.bin: $(BUILD_DIR)/boot.bin $(BUILD_DIR)/loader.bin $(BUILD_DIR)/kernel.bin
	cat $^ > $@
	truncate -s 20480 $@

$(BUILD_DIR)/boot.bin: $(SRC_BOOT)/boot.asm
	nasm -f bin $< -o $@

$(BUILD_DIR)/loader.bin: $(SRC_BOOT)/loader.asm
	nasm -f bin $< -o $@

$(BUILD_DIR)/kernel.bin: $(BUILD_DIR)/kernel_entry.o $(KERNEL_OBJS) $(DRIVER_OBJS) $(ASM_OBJS)
	ld -m elf_x86_64 -o $@ -Ttext 0x1000 $(BUILD_DIR)/kernel_entry.o $(KERNEL_OBJS) $(DRIVER_OBJS) $(ASM_OBJS) --oformat binary

$(BUILD_DIR)/kernel_entry.o: $(SRC_KERNEL)/kernel_entry.asm
	nasm -f elf64 $< -o $@

$(BUILD_DIR)/%.o: $(SRC_KERNEL)/%.asm
	nasm -f elf64 $< -o $@

$(BUILD_DIR)/%.o: $(SRC_KERNEL)/%.c
	gcc -m64 -ffreestanding -mcmodel=large -mno-red-zone -mno-mmx -mno-sse -mno-sse2 -fno-pie -c $< -o $@

$(BUILD_DIR)/%.o: $(SRC_DRIVERS)/%.c
	gcc -m64 -ffreestanding -mcmodel=large -mno-red-zone -mno-mmx -mno-sse -mno-sse2 -fno-pie -c $< -o $@

clean:
	rm -f $(BUILD_DIR)/*.bin $(BUILD_DIR)/*.o

.PHONY: run rerun all clean