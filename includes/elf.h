#ifndef ELF_H
#define ELF_H

#include <stdint.h>

// ELF64 자료형
typedef uint64_t    Elf64_Addr;
typedef uint64_t    Elf64_Off;
typedef uint16_t    Elf64_Half;
typedef uint32_t    Elf64_Word;
typedef int32_t     Elf64_Sword;
typedef uint64_t    Elf64_Xword;
typedef int64_t     Elf64_Sxword;

// ELF 파일 헤더
#define EI_NIDENT   16

typedef struct {
    unsigned char   e_ident[EI_NIDENT]; // Magic number and other info
    Elf64_Half      e_type;             // Object file type
    Elf64_Half      e_machine;          // Architecture
    Elf64_Word      e_version;          // Object file version
    Elf64_Addr      e_entry;            // Entry point virtual address
    Elf64_Off       e_phoff;            // Program header table file offset
    Elf64_Off       e_shoff;            // Section header table file offset
    Elf64_Word      e_flags;            // Processor-specific flags
    Elf64_Half      e_ehsize;           // ELF header size in bytes
    Elf64_Half      e_phentsize;        // Program header table entry size
    Elf64_Half      e_phnum;            // Program header table entry count
    Elf64_Half      e_shentsize;        // Section header table entry size
    Elf64_Half      e_shnum;            // Section header table entry count
    Elf64_Half      e_shstrndx;         // Section header string table index
} Elf64_Ehdr;

// e_ident 인덱스
#define EI_MAG0     0
#define EI_MAG1     1
#define EI_MAG2     2
#define EI_MAG3     3
#define EI_CLASS    4
#define EI_DATA     5
#define EI_VERSION  6
#define EI_OSABI    7
#define EI_ABIVERSION 8

// ELF Magic Numbers
#define ELFMAG0     0x7F
#define ELFMAG1     'E'
#define ELFMAG2     'L'
#define ELFMAG3     'F'

// e_type
#define ET_NONE     0
#define ET_REL      1
#define ET_EXEC     2
#define ET_DYN      3
#define ET_CORE     4

// e_machine
#define EM_X86_64   62

// 프로그램 헤더
typedef struct {
    Elf64_Word      p_type;             // Segment type
    Elf64_Word      p_flags;            // Segment flags
    Elf64_Off       p_offset;           // Segment file offset
    Elf64_Addr      p_vaddr;            // Segment virtual address
    Elf64_Addr      p_paddr;            // Segment physical address
    Elf64_Xword     p_filesz;           // Segment size in file
    Elf64_Xword     p_memsz;            // Segment size in memory
    Elf64_Xword     p_align;            // Segment alignment
} Elf64_Phdr;

// p_type
#define PT_NULL     0
#define PT_LOAD     1
#define PT_DYNAMIC  2
#define PT_INTERP   3
#define PT_NOTE     4
#define PT_SHLIB    5
#define PT_PHDR     6

// p_flags
#define PF_X        0x1
#define PF_W        0x2
#define PF_R        0x4

// 함수 선언
int     elf_check_file(Elf64_Ehdr* hdr);
int     elf_check_supported(Elf64_Ehdr* hdr);
void*   elf_load_file(void* file);

#endif