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