#include "../../includes/idt.h"
#include "../../includes/utils.h"
#include "../../includes/memory.h"
#include "../../includes/pmm.h"
#include "../../includes/kheap.h"
#include "../../includes/ata.h"
#include "../../includes/fat.h"

volatile char*  video_memory = (volatile char*)0xb8000;

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
// screen.c
extern void     kprint_at(char* message, int col, int row);
extern void     kprint(char* message);
extern void     clear_screen();
// interrupt.asm
extern void     irq0();
extern void     irq1();
// timer.c
extern void     init_timer(uint32_t freq);

void    user_input(char* input)
{
    char    command[32];
    char    arg[128];
    int     offset = 0;

    if (!get_next_token(input, command, &offset))
    {
        kprint("OS_eojin> ");
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

    kprint("OS_eojin> ");
}

void    main()
{
    kprint_at("Kernel loaded.\n", 0, 4);

    set_idt_gate(0, (uint64_t)isr0);    // ISR 0번(Divide by Zero) 등록
    set_idt_gate(32, (uint64_t)irq0);   // 32번 (IRQ 0) 등록, Timer
    set_idt_gate(33, (uint64_t)irq1);   // 33번 (IRQ 1) 등록, Keyboard

    // IDT 로드
    set_idt();

    __asm__ volatile("int $0");

    kprint("After Interrupt.\n");

    outb(0x20, 0x11);

    kprint("outb\n");

    pic_remap(); // PIC 초기화 및 리매핑

    kprint("init & remap\n");

    // PMM 초기화
    init_pmm();

    // Heap 초기화
    init_kheap();

    // 타이머 인터럽트(IRQ 0) 마스크 해제
    // 키보드 인터럽트(IRQ 1) 마스크 해제
    // 1111 1100 = 0xfc
    outb(0x21, 0xfc);

    // CPU 인터럽트 허용 (STI)
    __asm__ volatile("sti");

    // 타이머 인터럽트
    init_timer(50);

    kprint("OS_eojin> ");

    while(1);
}