#include "../../../includes/gdt.h"
#include "../../../includes/utils.h"

// 7 Entries: Null, KCode, KData, UData, UCode, TSS(Low), TSS(High)
gdt_entry_t gdt_entries[7];
gdt_ptr_t   gdt_ptr;
tss_entry_t tss_entry;

extern void gdt_flush(uint64_t);
extern void tss_flush();
extern void* memset(void* dest, int val, uint64_t count);

void set_gdt_gate(int num, uint64_t base, uint64_t limit, uint8_t access, uint8_t gran)
{
    gdt_entries[num].base_low    = (base & 0xFFFF);
    gdt_entries[num].base_middle = (base >> 16) & 0xFF;
    gdt_entries[num].base_high   = (base >> 24) & 0xFF;

    gdt_entries[num].limit_low   = (limit & 0xFFFF);
    gdt_entries[num].granularity = (limit >> 16) & 0x0F;

    gdt_entries[num].granularity |= gran & 0xF0;
    gdt_entries[num].access      = access;
}

// System Segment (TSS) 설정 (16바이트)
void set_tss_gate(int num, uint64_t base, uint64_t limit)
{
    // Lower 8 bytes (Standard Descriptor format)
    // 0x89 = Present(1), DPL(0), S(0-System), Type(1001-Available TSS)
    set_gdt_gate(num, base, limit, 0x89, 0x00);

    // Upper 8 bytes (Base address upper part)
    // 구조체를 직접 조작하여 다음 엔트리에 상위 주소 저장
    // gdt_entry_t 구조체는 8바이트. TSS는 두 슬롯 차지.
    
    // num+1 번째 슬롯의 base_low, base_middle 등에 상위 주소 매핑
    // 리틀 엔디안 고려
    
    uint64_t upper_base = (base >> 32);
    
    gdt_entries[num + 1].limit_low = (uint16_t)(upper_base & 0xFFFF);
    gdt_entries[num + 1].base_low  = (uint16_t)((upper_base >> 16) & 0xFFFF);
    gdt_entries[num + 1].base_middle = (uint8_t)((upper_base >> 32) & 0xFF);
    gdt_entries[num + 1].access = 0; // Reserved (Must be 0)
    gdt_entries[num + 1].granularity = 0; // Reserved
    gdt_entries[num + 1].base_high = 0; // Reserved
}

void init_gdt()
{
    gdt_ptr.limit = (sizeof(gdt_entry_t) * 7) - 1;
    gdt_ptr.base  = (uint64_t)&gdt_entries;

    // 0: Null
    set_gdt_gate(0, 0, 0, 0, 0);

    // 1: Kernel Code (0x08)
    // Access: 0x9A (1001 1010) -> Present, Ring 0, Code, Readable
    // Gran:   0x20 (0010 0000) -> Long Mode (L=1, D=0)
    set_gdt_gate(1, 0, 0xFFFFF, 0x9A, 0x20);

    // 2: Kernel Data (0x10)
    // Access: 0x92 (1001 0010) -> Present, Ring 0, Data, Writable
    set_gdt_gate(2, 0, 0xFFFFF, 0x92, 0x00);

    // 3: User Data (0x18) | 3 = 0x1B
    // Access: 0xF2 (1111 0010) -> Present, Ring 3, Data, Writable
    set_gdt_gate(3, 0, 0xFFFFF, 0xF2, 0x00);

    // 4: User Code (0x20) | 3 = 0x23
    // Access: 0xFA (1111 1010) -> Present, Ring 3, Code, Readable
    // Gran:   0x20 (L=1)
    set_gdt_gate(4, 0, 0xFFFFF, 0xFA, 0x20);

    // 5: TSS (0x28)
    // TSS 초기화
    memset(&tss_entry, 0, sizeof(tss_entry));
    tss_entry.iomap_base = sizeof(tss_entry); // No IO Map

    set_tss_gate(5, (uint64_t)&tss_entry, sizeof(tss_entry)-1);

    // GDT 로드 및 세그먼트 레지스터 갱신
    gdt_flush((uint64_t)&gdt_ptr);
    tss_flush();
}

void set_tss_rsp0(uint64_t rsp)
{
    tss_entry.rsp0 = rsp;
}
