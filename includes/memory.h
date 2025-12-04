#ifndef MEMORY_H
#define MEMORY_H

#include <stdint.h>

// e820 메모리 맵 엔트리 구조체 (24 bytes)
typedef struct
{
    uint64_t    base_addr;
    uint64_t    length;
    uint32_t    type;
    uint32_t    acpi;
} __attribute__((packed))   memory_map_entry_t;

void    print_memory_map();

#endif