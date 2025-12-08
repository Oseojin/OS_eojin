// src/user/hello.c

// 시스템 콜 래퍼
void sys_print(const char* msg) {
    // RAX=1 (Print), RBX=msg
    __asm__ volatile (
        "mov $1, %%rax\n"
        "mov %0, %%rbx\n"
        "int $0x80"
        : : "r"(msg) : "rax", "rbx"
    );
}

void sys_exit() {
    // RAX=0 (Exit)
    __asm__ volatile (
        "mov $0, %%rax\n"
        "int $0x80"
        : : : "rax"
    );
}

// 엔트리 포인트
void _start() {
    sys_print("Hello from ELF User Program!\n");
    sys_exit();
}

