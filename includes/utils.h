#ifndef UTILS_H
#define UTILS_H

#include <stdint.h>
#include <stddef.h>

int     strcmp(char s1[], char s2[]);
void    memcpy(char* source, char* dest, int nbytes);
void*   memset(void* dest, int val, uint64_t count);
void    hex_to_ascii(uint64_t n, char* str);
size_t  strlen(const char* str);
int     get_next_token(char* input, char* buffer, int* offset);

#endif