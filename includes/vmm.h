#ifndef VMM_H
#define VMM_H

#include <stdint.h>

#define PAGE_SIZE       4096

// 페이징 플래그
#define PAGING_FLAG_PRESENT     0x01
#define PAGING_FLAG_WRITABLE    0x02
#define PAGING_FLAG_USER        0x04
#define PAGING_FLAG_WRITE_THROUGH 0x08
#define PAGING_FLAG_NO_CACHE    0x10
#define PAGING_FLAG_ACCESSED    0x20
#define PAGING_FLAG_DIRTY       0x40
#define PAGING_FLAG_HUGE_PAGE   0x80 // PD, PDPT Only
#define PAGING_FLAG_GLOBAL      0x100
#define PAGING_FLAG_NX          0x8000000000000000 // No Execute

// 페이지 테이블 엔트리 (64-bit)
typedef uint64_t pt_entry_t;

// 페이지 테이블 (512 Entries)
typedef struct {
    pt_entry_t entries[512];
} __attribute__((aligned(4096))) page_table_t;

// 함수 선언
void        vmm_init();
page_table_t* vmm_create_pml4();
page_table_t* vmm_create_user_pml4();
void        vmm_map_page(page_table_t* pml4, uint64_t vaddr, uint64_t paddr, uint64_t flags);
void        vmm_unmap_page(page_table_t* pml4, uint64_t vaddr);
void        vmm_switch_pml4(page_table_t* pml4);
uint64_t    vmm_get_phys(page_table_t* pml4, uint64_t vaddr);

extern page_table_t* kernel_pml4;

#endif