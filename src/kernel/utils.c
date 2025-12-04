#include "../../includes/utils.h"

// 두 문자열이 같으면 0, 다르면 값의 차이를 반환
int strcmp(char s1[], char s2[])
{
    int i;
    for (i = 0; s1[i] == s2[i]; i++)
    {
        if (s1[i] == '\0')
        {
            return 0;
        }
    }
    return s1[i] - s2[i];
}

// 메모리 복사
void    memcpy(char* source, char* dest, int nbytes)
{
    int i;
    for (i = 0; i < nbytes; i++)
    {
        *(dest + i) = *(source + i);
    }
}

// 메모리의 특정 영역을 지정된 값으로 채움
void*   memset(void* dest, int val, uint64_t count)
{
    unsigned char*  ptr = (unsigned char*)dest;
    for (uint64_t i = 0; i < count; i++)
    {
        ptr[i] = (unsigned char)val;
    }
    return dest;
}

// 숫자를 16진수 문자열로 변환
void    hex_to_ascii(uint64_t n, char* str)
{
    str[0] = '0';
    str[1] = 'x';
    char    *hex = "0123456789abcdef";
    for (int i = 0; i < 16; i++)
    {
        str[17 - i] = hex[n & 0xf];
        n >>= 4;
    }
    str[18] = '\0';
}

size_t  strlen(const char* str)
{
    size_t  len = 0;
    while (str[len])
    {
        len++;
    }
    return len;
}

int get_next_token(char* input, char* buffer, int* offset)
{
    int i = *offset;
    int j = 0;

    while (input[i] == ' ')
    {
        i++;
    }

    if (input[i] == '\0')
    {
        *offset = i;
        return 0;
    }

    while (input[i] != ' ' && input[i] != '\0')
    {
        buffer[j++] = input[i++];
    }
    buffer[j] = '\0';

    *offset = i;
    return 1;
}