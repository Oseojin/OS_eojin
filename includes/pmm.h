#ifndef PMM_H
#define PMM_H

#include <stdint.h>
#include "memory.h"

#define BLOCK_SIZE 4096
#define SECTORS_PER_BLOCK 8

void        init_pmm();
void*       pmm_alloc_block();
void        pmm_free_block(void* p);
uint64_t    get_total_memory();

#endif