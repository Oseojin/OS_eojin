#include "../../includes/idt.h"
#include "../../includes/utils.h"
#include "../../includes/memory.h"
#include "../../includes/pmm.h"
#include "../../includes/kheap.h"
#include "../../includes/ata.h"
#include "../../includes/fat.h"
#include "../../includes/process.h"

// process.h
#include "../../includes/process.h"

volatile char*  video_memory = (volatile char*)0xb8000;

// 외부 함수 선언
extern void     kprint(char* message);

/*
void task_a()
{
    while(1)
    {
        kprint("A");
        for(int i=0; i<10000000; i++); // Delay
    }
}

void task_b()
{
    while(1)
    {
        kprint("B");
        for(int i=0; i<10000000; i++); // Delay
    }
}
*/

// 외부 함수 선언
// idt.h
extern void     set_idt_gate(int n, uint64_t handler);
extern void     set_idt();
extern void     isr0();
// utils.h
extern int      strcmp(char s1[], char s2[]);
extern int      get_next_token(char* input, char* buffer, int* offset);
extern void     strcpy(char* dest, const char* src);
// memory.h
extern void     print_memory_map();
// pmm.h
extern void     init_pmm();
// kheap.h
extern void     init_kheap();
extern void*    kmalloc(size_t size);
extern void     kfree(void* ptr);
// ata.h
extern void     ata_read_sector(uint32_t lba, uint8_t* buffer);
extern void     ata_write_sector(uint32_t lba, uint8_t* data);
// ports.c
extern void     outb(uint16_t port, uint8_t data);
extern uint8_t  inb(uint16_t port);
// pic.c
extern void     pic_remap();
// gdt.c
extern void     init_gdt();
// screen.c
extern void     kprint_at(char* message, int col, int row);
extern void     clear_screen();
// interrupt.asm
extern void     irq0();
extern void     irq1();
extern void     isr128();
extern void     isr8();
extern void     isr13();
extern void     isr14();

// timer.c
extern void     init_timer(uint32_t freq);

void    user_input(char* input)
{
    char    command[32];
    char    arg[128];
    int     offset = 0;

    if (!get_next_token(input, command, &offset))
    {
        kprint("OS_eojin:");
        kprint(fat_get_current_path());
        kprint("> ");
        return;
    }

    if (strcmp(command, "help") == 0)
    {
        kprint("Available commands:\n");
        kprint("    help            - Show this list\n");
        kprint("    clear           - Clear the screen\n");
        kprint("    halt            - Halt the CPU\n");
        kprint("    memory          - Show Memory Map\n");
        kprint("    echo [message]  - Print message\n");
        kprint("    write [data]    - Write Data to Disk\n");
        kprint("    read            - Read Data from Disk\n");
        kprint("    mount           - Connect fat_init() and ATA Driver\n");
        kprint("    ls              - List Root Directory\n");
        kprint("    cat [file]      - Print File Content\n");
        kprint("    exec [file]     - Execute User Program\n");
        kprint("    cd [dir]        - Change Directory\n");
        kprint("    touch [file]    - Create Empty File\n");
        kprint("    mkdir [dir]     - Create Directory\n");
    }
    else if (strcmp(command, "clear") == 0)
    {
        clear_screen();
    }
    else if (strcmp(command, "halt") == 0)
    {
        kprint("Halting CPU. Bye!\n");
        __asm__ volatile("hlt");
    }
    else if (strcmp(command, "memory") == 0)
    {
        print_memory_map();
    }
    else if (strcmp(command, "echo") == 0)
    {
        while (input[offset] == ' ')
        {
            offset++;
        }

        kprint(&input[offset]);
        kprint("\n");
    }
    else if (strcmp(command, "write") == 0)
    {
        // write <문자열>

        // 버퍼 준비 (512바이트)
        uint8_t*    buf = (uint8_t*)kmalloc(512);
        memset(buf, 0, 512);

        // 인자 파싱
        char    arg[128];
        int     arg_offset = offset;

        if (get_next_token(input, arg, &arg_offset))
        {
            strcpy((char*)buf, arg);
        }
        else
        {
            strcpy((char*)buf, "TEST_DATA_DEFAULT");
        }

        kprint("Writing to LBA 200: ");
        kprint((char*)buf);
        kprint("\n");

        // 디스크 쓰기
        ata_write_sector(200, buf);

        kfree(buf);
        kprint("Write Done.\n");
    }
    else if (strcmp(command, "read") == 0)
    {
        // read

        // 버퍼 준비
        uint8_t *buf = (uint8_t*)kmalloc(512);
        memset(buf, 0, 512);

        kprint("Reading from LBA 200...\n");

        // 디스크 읽기
        ata_read_sector(200, buf);

        // 결과 출력
        kprint("Data: ");
        kprint((char*)buf); // 문자열이라고 가정하고 출력
        kprint("\n");

        kfree(buf);
    }
    else if (strcmp(command, "mount") == 0)
    {
        fat_init();
    }
    else if (strcmp(command, "ls") == 0)
    {
        fat_list();
    }
    else if (strcmp(command, "cat") == 0)
    {
        char    filename[32];
        int     arg_offset = offset;

        if (!get_next_token(input, filename, &arg_offset))
        {
            kprint("Usage: cat <filename>\n");
        }
        else
        {
            fat_dir_entry_t entry;
            if (fat_find_file(filename, &entry))
            {
                uint16_t cluster = entry.first_cluster_low;
                uint32_t size = entry.file_size;
                uint8_t* buf = (uint8_t*)kmalloc(512);
                
                if (!buf)
                {
                    kprint("cat: Memory allocation failed\n");
                }
                else
                {
                    while (size > 0 && cluster < 0xFFF8)
                    {
                        uint32_t lba = fat_lba_of_cluster(cluster);
                        ata_read_sector(lba, buf);

                        // 1바이트씩 출력
                        for (int i = 0; i < 512 && size > 0; i++, size--)
                        {
                            char c[2] = {buf[i], 0};
                            kprint(c);
                        }

                        cluster = fat_next_cluster(cluster);
                    }
                    kprint("\n");
                    kfree(buf);
                }
            }
            else
            {
                kprint("File not found: ");
                kprint(filename);
                kprint("\n");
            }
        }
    }
    else if (strcmp(command, "exec") == 0)
    {
        char    filename[32];
        int     arg_offset = offset;

        if (!get_next_token(input, filename, &arg_offset))
        {
            kprint("Usage: exec <filename>\n");
        }
        else
        {
            fat_dir_entry_t entry;
            if (fat_find_file(filename, &entry))
            {
                // 프로그램 로드 주소 (64MB) - 힙(4MB~)과 충돌 방지
                void* load_addr = (void*)0x4000000;
                
                uint16_t cluster = entry.first_cluster_low;
                uint32_t size = entry.file_size;
                uint8_t* ptr = (uint8_t*)load_addr;
                
                // 임시 버퍼 (섹터 읽기용)
                uint8_t* buf = (uint8_t*)kmalloc(512);
                if (!buf)
                {
                    kprint("exec: Memory allocation failed\n");
                }
                else
                {
                    while (size > 0 && cluster < 0xFFF8)
                    {
                        uint32_t lba = fat_lba_of_cluster(cluster);
                        ata_read_sector(lba, buf);
                        
                        // 메모리로 복사
                        int copy_size = (size > 512) ? 512 : size;
                        for(int i=0; i<copy_size; i++) {
                            ptr[i] = buf[i];
                        }
                        ptr += 512;
                        size -= copy_size;
                        
                        cluster = fat_next_cluster(cluster);
                    }
                    kfree(buf);

                    kprint("Loaded ");
                    char size_buf[16];
                    hex_to_ascii(entry.file_size, size_buf);
                    kprint(size_buf);
                    kprint(" bytes at 0x4000000\n");

                    kprint("Spawning Process for ");
                    kprint(filename);
                    kprint("...\n");

                    // 함수 포인터로 형변환 후 실행
                    // void (*prog)() = (void (*)())load_addr;
                    // prog();
                    create_kernel_process((void (*)())load_addr);

                    kprint("Process Created.\n");
                    
                    // Don't print prompt here, wait for process exit
                    return;
                }
            }
            else
            {
                kprint("File not found: ");
                kprint(filename);
                kprint("\n");
            }
        }
    }
    else if (strcmp(command, "cd") == 0)
    {
        char    dirname[32];
        int     arg_offset = offset;

        if (!get_next_token(input, dirname, &arg_offset))
        {
            kprint("Usage: cd <dirname>\n");
        }
        else
        {
            fat_change_dir(dirname);
        }
    }
    else if (strcmp(command, "touch") == 0)
    {
        char    filename[32];
        int     arg_offset = offset;

        if (!get_next_token(input, filename, &arg_offset))
        {
            kprint("Usage: touch <filename>\n");
        }
        else
        {
            // 파일 생성 (클러스터 0, 크기 0)
            if (fat_create_entry(filename, 0, 0, 0))
            {
                kprint("File created.\n");
            }
        }
    }
    else if (strcmp(command, "mkdir") == 0)
    {
        char    dirname[32];
        int     arg_offset = offset;

        if (!get_next_token(input, dirname, &arg_offset))
        {
            kprint("Usage: mkdir <dirname>\n");
        }
        else
        {
            // 1. 클러스터 할당
            uint16_t cluster = fat_allocate_cluster();
            if (cluster == 0)
            {
                kprint("Disk full (No free clusters).\n");
            }
            else
            {
                // 2. 클러스터 초기화 (. 및 .. 생성)
                uint8_t* buf = (uint8_t*)kmalloc(512);
                memset(buf, 0, 512);
                
                fat_dir_entry_t* dot = (fat_dir_entry_t*)buf;
                fat_dir_entry_t* dotdot = (fat_dir_entry_t*)(buf + 32); // 2nd entry

                // . Entry
                memset(dot->name, ' ', 11);
                dot->name[0] = '.';
                dot->attributes = ATTR_DIRECTORY;
                dot->first_cluster_low = cluster;

                // .. Entry
                memset(dotdot->name, ' ', 11);
                dotdot->name[0] = '.';
                dotdot->name[1] = '.';
                dotdot->attributes = ATTR_DIRECTORY;
                dotdot->first_cluster_low = fat_get_current_cluster();

                uint32_t lba = fat_lba_of_cluster(cluster);
                ata_write_sector(lba, buf);
                kfree(buf);

                // 3. 현재 디렉토리에 엔트리 생성
                if (fat_create_entry(dirname, ATTR_DIRECTORY, cluster, 0))
                {
                    kprint("Directory created.\n");
                }
                else
                {
                    kprint("Failed to create entry.\n");
                }
            }
        }
    }
    // alloc test
    else if (strcmp(command, "alloc") == 0)
    {
        void    *ptr = pmm_alloc_block();

        if (ptr == 0)
        {
            kprint("Allocation Failed (Out of Memory)\n");
        }
        else
        {
            char    buf[32];
            kprint("Allocated at: ");
            hex_to_ascii((uint64_t)ptr, buf);
            kprint(buf);
            kprint("\n");
        }
    }
    // malloc test
    else if (strcmp(command, "heap_test") == 0)
    {
        char    buf[32];

        void*   a = kmalloc(64);
        kprint("Alloc A: "); hex_to_ascii((uint64_t)a, buf); kprint(buf); kprint("\n");

        void*   b = kmalloc(64);
        kprint("Alloc B: "); hex_to_ascii((uint64_t)b, buf); kprint(buf); kprint("\n");

        void*   c = kmalloc(64);
        kprint("Alloc C: "); hex_to_ascii((uint64_t)c, buf); kprint(buf); kprint("\n");

        kprint("Free B...\n");
        kfree(b);

        void*   d = kmalloc(64);
        kprint("Alloc D: "); hex_to_ascii((uint64_t)d, buf); kprint(buf); kprint("\n");

        if (d == b)
        {
            kprint("Heap Test Passed! (Address Reused)\n");
        }
        else
        {
            kprint("Heap Test Failed (Address Not Reused)\n");
        }

        kfree(a);
        kfree(c);
        kfree(d);
    }
    else
    {
        kprint("Unknown command: ");
        kprint(input);
        kprint("\n");
    }

    kprint("OS_eojin:");
    kprint(fat_get_current_path());
    kprint("> ");
}

void    main()
{
    kprint_at("Kernel loaded.\n", 0, 4);

    set_idt_gate(0, (uint64_t)isr0);    // ISR 0번(Divide by Zero) 등록
    set_idt_gate(8, (uint64_t)isr8);
    set_idt_gate(13, (uint64_t)isr13);
    set_idt_gate(14, (uint64_t)isr14);
    set_idt_gate(32, (uint64_t)irq0);   // 32번 (IRQ 0) 등록, Timer
    set_idt_gate(33, (uint64_t)irq1);   // 33번 (IRQ 1) 등록, Keyboard
    set_idt_gate(128, (uint64_t)isr128);// 128번 (0x80) Syscall

    // IDT 로드
    set_idt();

    __asm__ volatile("int $0");

    kprint("After Interrupt.\n");

    outb(0x20, 0x11);

    kprint("outb\n");

    pic_remap(); // PIC 초기화 및 리매핑

    kprint("init & remap\n");

    // GDT & TSS 초기화
    init_gdt();

    // PMM 초기화
    init_pmm();

    // Heap 초기화
    init_kheap();

    // 멀티태스킹 초기화
    init_multitasking();
    
    // create_kernel_process(task_a);
    // create_kernel_process(task_b);

    // 타이머 인터럽트(IRQ 0) 마스크 해제
    // 키보드 인터럽트(IRQ 1) 마스크 해제
    // 1111 1100 = 0xfc
    outb(0x21, 0xfc);

    // CPU 인터럽트 허용 (STI)
    __asm__ volatile("sti");

    // 타이머 인터럽트
    init_timer(50);

    kprint("OS_eojin:");
    kprint(fat_get_current_path());
    kprint("> ");

    while(1)
    {
        __asm__ volatile("hlt");
    }
}