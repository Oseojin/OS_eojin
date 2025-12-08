#include "../../includes/vmm.h"
#include "../../includes/pmm.h"
#include "../../includes/utils.h"

// 외부 함수
extern void* memset(void* dest, int val, uint64_t count);
extern void kprint(char* msg);

// 현재 활성화된 PML4
page_table_t* current_pml4 = 0;
page_table_t* kernel_pml4 = 0;

// CR3 리로드 (TLB 플러시)
void vmm_flush_tlb()
{
    uint64_t cr3;
    __asm__ volatile("mov %%cr3, %0" : "=r"(cr3));
    __asm__ volatile("mov %0, %%cr3" : : "r"(cr3));
}

// CR3 값 설정
void vmm_switch_pml4(page_table_t* pml4)
{
    current_pml4 = pml4;
    __asm__ volatile("mov %0, %%cr3" : : "r"((uint64_t)pml4));
}

// 인덱스 추출 매크로
#define PML4_INDEX(addr) (((addr) >> 39) & 0x1FF)
#define PDPT_INDEX(addr) (((addr) >> 30) & 0x1FF)
#define PD_INDEX(addr)   (((addr) >> 21) & 0x1FF)
#define PT_INDEX(addr)   (((addr) >> 12) & 0x1FF)

void vmm_map_page(page_table_t* pml4, uint64_t vaddr, uint64_t paddr, uint64_t flags)
{
    uint64_t pml4_idx = PML4_INDEX(vaddr);
    uint64_t pdpt_idx = PDPT_INDEX(vaddr);
    uint64_t pd_idx   = PD_INDEX(vaddr);
    uint64_t pt_idx   = PT_INDEX(vaddr);

    // 1. PML4 -> PDPT
    if (!(pml4->entries[pml4_idx] & PAGING_FLAG_PRESENT))
    {
        uint64_t* pdpt = (uint64_t*)pmm_alloc_block();
        if (!pdpt) return; // Panic?
        memset(pdpt, 0, PAGE_SIZE);
        
        // 상위 테이블은 유저 접근 허용해야 하위 테이블 접근 가능 (보통 User=1, RW=1)
        pml4->entries[pml4_idx] = (uint64_t)pdpt | PAGING_FLAG_PRESENT | PAGING_FLAG_WRITABLE | PAGING_FLAG_USER;
    }
    
    page_table_t* pdpt = (page_table_t*)(pml4->entries[pml4_idx] & 0xFFFFFFFFFF000);

    // 2. PDPT -> PD
    if (!(pdpt->entries[pdpt_idx] & PAGING_FLAG_PRESENT))
    {
        uint64_t* pd = (uint64_t*)pmm_alloc_block();
        if (!pd) return;
        memset(pd, 0, PAGE_SIZE);
        
        pdpt->entries[pdpt_idx] = (uint64_t)pd | PAGING_FLAG_PRESENT | PAGING_FLAG_WRITABLE | PAGING_FLAG_USER;
    }

    page_table_t* pd = (page_table_t*)(pdpt->entries[pdpt_idx] & 0xFFFFFFFFFF000);

    // 3. PD -> PT
    // 주의: 2MB Huge Page가 아닌 4KB Page 사용 가정
    if (!(pd->entries[pd_idx] & PAGING_FLAG_PRESENT))
    {
        uint64_t* pt = (uint64_t*)pmm_alloc_block();
        if (!pt) return;
        memset(pt, 0, PAGE_SIZE);
        
        pd->entries[pd_idx] = (uint64_t)pt | PAGING_FLAG_PRESENT | PAGING_FLAG_WRITABLE | PAGING_FLAG_USER;
    }

    page_table_t* pt = (page_table_t*)(pd->entries[pd_idx] & 0xFFFFFFFFFF000);

    // 4. PT Entry 설정
    pt->entries[pt_idx] = (paddr & 0xFFFFFFFFFF000) | flags;
    
    // TLB Flush (특정 주소만)
    __asm__ volatile("invlpg (%0)" : : "r"(vaddr) : "memory");
}

void vmm_unmap_page(page_table_t* pml4, uint64_t vaddr)
{
    // ... (생략: map과 유사하게 찾아가서 Present 비트 끔)
    // 구현 필요 시 작성
}

void vmm_init()
{
    // 커널용 PML4 생성
    kernel_pml4 = (page_table_t*)pmm_alloc_block();
    memset(kernel_pml4, 0, PAGE_SIZE);

    // Identity Mapping (0 ~ 128MB)
    // 커널 코드, 데이터, 비디오 메모리 등
    uint64_t identity_map_end = 0x8000000; // 128MB

    for (uint64_t addr = 0; addr < identity_map_end; addr += PAGE_SIZE)
    {
        // 커널 영역이므로 User 비트는 끔? -> 아니오, 지금은 1:1 매핑이므로 User 접근도 허용해야 함 (임시)
        // 나중에 커널/유저 공간 분리 시 수정.
        // 지금은 모든 프로세스가 동일 공간을 공유하므로 User=1.
        vmm_map_page(kernel_pml4, addr, addr, PAGING_FLAG_PRESENT | PAGING_FLAG_WRITABLE | PAGING_FLAG_USER);
    }

    // CR3 로드
    vmm_switch_pml4(kernel_pml4);
    kprint("VMM Initialized.\n");
}

page_table_t* vmm_create_user_pml4()
{
    page_table_t* pml4 = (page_table_t*)pmm_alloc_block();
    if (!pml4) return 0;
    memset(pml4, 0, PAGE_SIZE);

    // 커널 영역 매핑 공유 (PML4 Entry 0: 0 ~ 512GB)
    // 현재 커널은 0~128MB Identity Mapping을 사용하므로 Entry 0에 다 들어있음.
    // 이를 복사하여 공유함.
    pml4->entries[0] = kernel_pml4->entries[0];

    return pml4;
}
