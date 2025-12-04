#ifndef UTILS_H
#define UTILS_H

#include <stdint.h>
#include <stddef.h>

// utils.c
int     strcmp(char s1[], char s2[]);
void    memcpy(char* source, char* dest, int nbytes);
void*   memset(void* dest, int val, uint64_t count);
void    hex_to_ascii(uint64_t n, char* str);
size_t  strlen(const char* str);
int     get_next_token(char* input, char* buffer, int* offset);
void    strcpy(char* dest, const char* src);

// ports.c
void        outb(uint16_t port, uint8_t data);
uint8_t     inb(uint16_t port);
void        outw(uint16_t port, uint16_t data);
uint16_t    inw(uint16_t port);

#endif