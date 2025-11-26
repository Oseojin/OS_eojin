#include "kernel.h"

// VGA 제어 포트
#define REG_SCREEN_CTRL 0x3d4
#define REG_SCREEN_DATA 0x3d5

// 커서 위치를 가져오는 내부 함수
int get_cursor_offset() {
    port_byte_out(REG_SCREEN_CTRL, 14);
    int offset = port_byte_in(REG_SCREEN_DATA) << 8;
    port_byte_out(REG_SCREEN_CTRL, 15);
    offset += port_byte_in(REG_SCREEN_DATA);
    return offset * 2; // 문자+속성(2바이트) 단위로 변환
}

// 커서 위치를 설정하는 내부 함수
void set_cursor_offset(int offset) {
    offset /= 2;
    port_byte_out(REG_SCREEN_CTRL, 14);
    port_byte_out(REG_SCREEN_DATA, (unsigned char)(offset >> 8));
    port_byte_out(REG_SCREEN_CTRL, 15);
    port_byte_out(REG_SCREEN_DATA, (unsigned char)(offset & 0xff));
}

// 내부 함수: 화면 좌표를 메모리 오프셋으로 변환
int get_offset(int col, int row) { return 2 * (row * MAX_COLS + col); }
int get_offset_row(int offset) { return offset / (2 * MAX_COLS); }
int get_offset_col(int offset) { return (offset - (get_offset_row(offset) * 2 * MAX_COLS)) / 2; }

// 문자 하나를 화면에 찍는 핵심 함수
void print_char(char character, int col, int row, char attribute_byte) {
    unsigned char *vidmem = (unsigned char *) VIDEO_ADDRESS;

    if (!attribute_byte) attribute_byte = WHITE_ON_BLACK;

    int offset;
    if (col >= 0 && row >= 0) {
        offset = get_offset(col, row);
    } else {
        offset = get_cursor_offset();
    }

    if (character == '\n') {
        // 개행 문자: 다음 줄의 0번째 칸으로 이동
        int rows = offset / (2 * MAX_COLS);
        offset = get_offset(0, rows + 1);
    } else {
        vidmem[offset] = character;
        vidmem[offset + 1] = attribute_byte;
        offset += 2;
    }

    // 스크롤링 처리
    if (offset >= MAX_ROWS * MAX_COLS * 2) {
        int i;
        // 1. 모든 줄을 한 칸씩 위로 복사
        for (i = 1; i < MAX_ROWS; i++) {
            memory_copy((char*)(get_offset(0, i) + VIDEO_ADDRESS),
                        (char*)(get_offset(0, i-1) + VIDEO_ADDRESS),
                        MAX_COLS * 2);
        }
        // 2. 마지막 줄을 공백으로 채움
        char *last_line = (char*)(get_offset(0, MAX_ROWS-1) + VIDEO_ADDRESS);
        for (i = 0; i < MAX_COLS * 2; i++) last_line[i] = 0;
        
        offset -= 2 * MAX_COLS;
    }

    set_cursor_offset(offset);
}

// 특정 위치에 문자열 출력
void kprint_at(char *message, int col, int row) {
    int offset;
    if (col >= 0 && row >= 0)
        set_cursor_offset(get_offset(col, row));

    int i = 0;
    while (message[i] != 0) {
        print_char(message[i++], col, row, WHITE_ON_BLACK);
        // 다음 문자를 위해 위치 재계산이 필요하지만, 
        // print_char 내부에서 커서를 업데이트하므로 -1을 넘겨서 커서 위치를 쓰게 함
        col = -1; row = -1; 
    }
}

// 현재 커서 위치에 문자열 출력
void kprint(char *message) {
    kprint_at(message, -1, -1);
}

// 화면 지우기
void clear_screen() {
    int screen_size = MAX_COLS * MAX_ROWS;
    int i;
    unsigned char *screen = (unsigned char *) VIDEO_ADDRESS;

    for (i = 0; i < screen_size; i++) {
        screen[i*2] = ' ';
        screen[i*2+1] = WHITE_ON_BLACK;
    }
    set_cursor_offset(get_offset(0, 0));
}

// 백스페이스 처리
void kprint_backspace() {
    int offset = get_cursor_offset() - 2;
    int row = get_offset_row(offset);
    int col = get_offset_col(offset);
    print_char(0x08, col, row, WHITE_ON_BLACK); // 0x08은 백스페이스 문자지만 여기선 의미 없음
    // 실제로는 문자를 지우고 커서를 뒤로 당겨야 함
    unsigned char *vidmem = (unsigned char *) VIDEO_ADDRESS;
    vidmem[offset] = ' ';
    vidmem[offset+1] = WHITE_ON_BLACK;
    set_cursor_offset(offset);
}