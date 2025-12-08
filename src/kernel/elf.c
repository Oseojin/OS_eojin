#include "../../includes/elf.h"
#include "../../includes/utils.h"
#include "../../includes/kheap.h"

// 외부 함수
extern void kprint(char* message);
extern void memcpy(char* dest, char* source, int nbytes);
extern void* memset(void* dest, int val, uint64_t count);

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
void* elf_load_file(void* file_buffer)
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
            // 파일에서 세그먼트 데이터 위치
            void*   segment_file_ptr = (void*)((uint64_t)file_buffer + phdr[i].p_offset); 
            
            // 메모리에 로드할 주소 (Virtual Address)
            void*   segment_mem_ptr = (void*)phdr[i].p_vaddr;
            
            // 메모리 크기
            uint64_t mem_size = phdr[i].p_memsz;
            // 파일 크기
            uint64_t file_size = phdr[i].p_filesz;

            // 1. 파일 데이터 복사
            memcpy((char*)segment_mem_ptr, (char*)segment_file_ptr, file_size);

            // 2. BSS 영역 초기화 (메모리 크기가 파일 크기보다 크면 나머지는 0)
            if (mem_size > file_size)
            {
                void* bss_ptr = (void*)((uint64_t)segment_mem_ptr + file_size);
                memset(bss_ptr, 0, mem_size - file_size);
            }
            
            /*
            kprint("ELF Segment Loaded at ");
            char buf[32];
            hex_to_ascii(phdr[i].p_vaddr, buf);
            kprint(buf);
            kprint("\n");
            */
        }
    }

    /*
    kprint("ELF Entry: ");
    char buf2[32];
    hex_to_ascii(hdr->e_entry, buf2);
    kprint(buf2);
    kprint("\n");
    */

    return (void*)hdr->e_entry;
}
