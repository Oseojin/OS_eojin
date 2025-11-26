/*
 * Minimal C Kernel
 * 기능: 화면 출력 및 하드웨어 초기화 진입점
 */

#include "kernel.h" // 헤더 파일 포함

// 비디오 메모리에 문자를 출력하는 저수준 함수
void print_char(char character, int col, int row, char attribute_byte) {
    unsigned char *vidmem = (unsigned char *) VIDEO_MEMORY;
    int offset = (row * MAX_COLS + col) * 2;

    vidmem[offset] = character;
    vidmem[offset + 1] = attribute_byte;
}

// 문자열을 출력하는 함수
void print_string(char* message, int row) {
    int i = 0;
    while (message[i] != 0) {
        print_char(message[i], i, row, WHITE_ON_BLACK);
        i++;
    }
}

// 커널의 진입점 (kernel_entry.asm에서 호출됨)
void main() {
    print_string("Welcome to My Operating System!", 0);
    print_string("Initializing Drivers...", 1);

    // IDT 설정 및 인터럽트 활성화 (drivers.c에 정의됨)
    init_drivers();

    print_string("Keyboard Enabled. Type something!", 2);

    // CPU가 종료되지 않도록 무한 루프
    // (이 루프가 도는 동안 키보드를 누르면 인터럽트가 발생해 keyboard_handler가 실행됨)
    while(1) {
        __asm__("hlt"); // CPU를 쉬게 함 (전력 소모 감소)
    }
}