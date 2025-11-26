#include "kernel.h"

// 메모리 복사 (memcpy와 동일)
void memory_copy(char *source, char *dest, int n_bytes) {
    int i;
    for (i = 0; i < n_bytes; i++) {
        *(dest + i) = *(source + i);
    }
}

// 메모리 초기화 (memset과 동일)
void memory_set(unsigned char *dest, unsigned char val, int len) {
    unsigned char *temp = (unsigned char *)dest;
    for ( ; len != 0; len--) *temp++ = val;
}

// 문자열 길이 반환
int strlen(char s[]) {
    int i = 0;
    while (s[i] != 0) ++i;
    return i;
}