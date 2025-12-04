#include "../../includes/memory.h"
#include "../../includes/utils.h"

// 외부 함수 선언
// screen.c
extern void kprint(char* msg);
// utils.c
extern void hex_to_ascii(uint64_t n, char* str);

void    print_memory_map()
{
    uint16_t    entry_count = *(uint16_t*)0x5000;

    char    buf[32];
    kprint("Memory Map Entries: ");
    hex_to_ascii(entry_count, buf);
    kprint(buf);
    kprint("\n");

    memory_map_entry_t  *entry = (memory_map_entry_t*)0x5004;

    for (int i = 0; i < entry_count; i++)
    {
        kprint("Base: ");
        hex_to_ascii(entry[i].base_addr, buf);
        kprint(buf);

        kprint(" Length: ");
        hex_to_ascii(entry[i].length, buf);
        kprint(buf);

        kprint(" Type: ");
        // Type = 1(Usable), 2(Reserved) 등
        if (entry[i].type == 1)
        {
            kprint("1 (Free)\n");
        }
        else
        {
            kprint("Other\n");
        }
    }
}