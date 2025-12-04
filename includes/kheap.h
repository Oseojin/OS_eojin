#ifndef KHEAP_H
#define KHEAP_H

#include <stdint.h>
#include <stddef.h>

typedef struct  block_header
{
    struct  block_header* next;
    size_t  size;
    uint8_t is_free;
}   block_header_t;

void    init_kheap();
void*   kmalloc(size_t size);
void    kfree(void *ptr);

#endif