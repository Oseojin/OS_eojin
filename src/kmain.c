/*
 * Minimal C Kernel
 * 기능: 비디오 메모리에 직접 접근하여 화면에 문자열 출력
 */

// VGA 텍스트 모드 메모리 주소 (컬러 모니터 기준)
#define VIDEO_MEMORY 0xB8000
// 화면 너비/높이
#define MAX_ROWS 25
#define MAX_COLS 80

// 속성 바이트: 흰색 글자(0x0F) on 검은 배경(0x00)
#define WHITE_ON_BLACK 0x0f

// 비디오 메모리에 문자를 출력하는 저수준 함수
void print_char(char character, int col, int row, char attribute_byte) {
    // 비디오 메모리는 '문자' + '속성'의 쌍으로 이루어져 있음
    unsigned char *vidmem = (unsigned char *) VIDEO_MEMORY;

    // 2바이트씩 건너뛰어야 하므로 오프셋 계산 (2 * (row * 80 + col))
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
    // 화면 첫 번째 줄에 메시지 출력
    print_string("Welcome to My Operating System!", 0);
    
    // 화면 두 번째 줄에 메시지 출력
    print_string("Running in 32-bit Protected Mode", 1);

    // C 코드 끝. 리턴하면 kernel_entry.asm의 jmp $로 돌아감.
}