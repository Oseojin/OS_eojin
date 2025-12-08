#include "../../includes/elf.h"
#include "../../includes/utils.h"
#include "../../includes/kheap.h"
#include "../../includes/vmm.h" // For vmm_map_page

// 외부 함수
extern void kprint(char* message);
extern void memcpy(char* dest, char* source, int nbytes);
extern void* memset(void* dest, int val, uint64_t count);
extern void* pmm_alloc_block();
extern void pmm_free_block(void* p);

int elf_check_file(Elf64_Ehdr* hdr)
{
    if (!hdr) return 0;

    if (hdr->e_ident[EI_MAG0] != ELFMAG0) {
        kprint("ELF Header Error: MAG0\n");
        return 0;
    }
    if (hdr->e_ident[EI_MAG1] != ELFMAG1) {
        kprint("ELF Header Error: MAG1\n");
        return 0;
    }
    if (hdr->e_ident[EI_MAG2] != ELFMAG2) {
        kprint("ELF Header Error: MAG2\n");
        return 0;
    }
    if (hdr->e_ident[EI_MAG3] != ELFMAG3) {
        kprint("ELF Header Error: MAG3\n");
        return 0;
    }
    return 1;
}

int elf_check_supported(Elf64_Ehdr* hdr)
{
    if (!elf_check_file(hdr)) return 0;

    if (hdr->e_ident[EI_CLASS] != 2) { // 2 = 64-bit
        kprint("ELF Error: Not 64-bit\n");
        return 0;
    }
    if (hdr->e_ident[EI_DATA] != 1) { // 1 = Little Endian
        kprint("ELF Error: Not Little Endian\n");
        return 0;
    }
    if (hdr->e_machine != EM_X86_64) {
        kprint("ELF Error: Not x86-64\n");
        return 0;
    }
    if (hdr->e_type != ET_EXEC && hdr->e_type != ET_DYN) {
        kprint("ELF Error: Not Executable\n");
        return 0;
    }
    return 1;
}

// 파일 데이터(file_buffer)를 파싱하여 메모리에 로드하고 Entry Point 반환
void* elf_load_file(page_table_t* pml4, void* file_buffer)
{
    Elf64_Ehdr* hdr = (Elf64_Ehdr*)file_buffer;

    if (!elf_check_supported(hdr)) {
        return 0;
    }

    // Program Header Table 순회
    Elf64_Phdr* phdr = (Elf64_Phdr*)((uint64_t)file_buffer + hdr->e_phoff);

    for (int i = 0; i < hdr->e_phnum; i++)
    {
        if (phdr[i].p_type == PT_LOAD)
        {
            uint64_t vaddr_start = phdr[i].p_vaddr;
            uint64_t mem_size = phdr[i].p_memsz;
            uint64_t file_size = phdr[i].p_filesz;
            uint64_t file_offset = phdr[i].p_offset;

            // 페이지 단위로 루프
            // 시작 주소가 페이지 정렬이 안 되어 있을 수 있으므로 내림
            uint64_t vaddr = vaddr_start & ~(PAGE_SIZE - 1); 
            // 끝 주소는 올림
            uint64_t end_vaddr = (vaddr_start + mem_size + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);

            for (; vaddr < end_vaddr; vaddr += PAGE_SIZE)
            {
                // 1. 물리 페이지 할당
                uint64_t paddr = (uint64_t)pmm_alloc_block();
                if (!paddr)
                {
                    kprint("ELF Load Error: OOM\n");
                    return 0;
                }

                // 2. PML4 매핑 (User, RW, Present)
                vmm_map_page(pml4, vaddr, paddr, PAGING_FLAG_PRESENT | PAGING_FLAG_WRITABLE | PAGING_FLAG_USER);

                // 3. 데이터 복사 및 BSS 초기화
                // 물리 주소(paddr)에 직접 씀 (커널은 1:1 매핑 사용 중)
                memset((void*)paddr, 0, PAGE_SIZE); // 일단 0으로 초기화 (BSS 처리 포함)

                // 복사할 범위 계산 (Overlap)
                // [vaddr, vaddr + 4096)  <->  [vaddr_start, vaddr_start + file_size)
                
                uint64_t seg_data_start = vaddr_start;
                uint64_t seg_data_end = vaddr_start + file_size;
                
                uint64_t page_start = vaddr;
                uint64_t page_end = vaddr + PAGE_SIZE;

                // 겹치는 구간 구하기 (Intersection)
                uint64_t copy_start = (page_start < seg_data_start) ? seg_data_start : page_start;
                uint64_t copy_end = (page_end > seg_data_end) ? seg_data_end : page_end;

                if (copy_start < copy_end)
                {
                    uint64_t len = copy_end - copy_start;
                    uint64_t file_src_offset = file_offset + (copy_start - vaddr_start);
                    uint64_t page_dst_offset = copy_start - page_start;

                    memcpy((char*)(paddr + page_dst_offset), (char*)((uint64_t)file_buffer + file_src_offset), len);
                }
            }
        }
    }

    return (void*)hdr->e_entry;
}