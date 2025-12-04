#include "../../includes/fat.h"
#include "../../includes/ata.h"
#include "../../includes/kheap.h"
#include "../../includes/utils.h"

// 외부 함수 선언
// screen.c
extern void kprint(char* message);
extern void hex_to_ascii(uint64_t n, char* str);
extern void memcpy(char* dest, char* source, int nbytes);

// 파일 시스템 정보 유지(전역 변수)
fat_bpb_t   fat_info;
uint32_t    fat_start_sector;
uint32_t    data_start_sector;
uint32_t    root_dir_sector;

void    fat_init()
{
    // 부트 섹터(LBA 0) 읽기
    uint8_t*    buffer = (uint8_t*)kmalloc(512);
    if (!buffer)
    {
        kprint("FAT: Memory allocation failed\n");
        return;
    }
    ata_read_sector(0, buffer);

    // BPB 파싱
    memcpy((char*)&fat_info, (char*)buffer, sizeof(fat_bpb_t));

    char    buf[32];
    kprint("---FAT16 Init ---\n");

    // 시그니처 확인 (FAT16 부트 섹터는 일반적으로 0xaa55로 끝남)

    kprint("OEM Name: ");
    for (int i = 0; i < 8; i++)
    {
        char    c[2] = {fat_info.oem_name[i], 0};
        kprint(c);
    }
    kprint("\n");

    kprint("Bytes per Sector: ");
    hex_to_ascii(fat_info.bytes_per_sector, buf);
    kprint(buf);
    kprint("\n");

    kprint("Sectors per Cluster: ");
    hex_to_ascii(fat_info.sectors_per_cluster, buf);
    kprint(buf);
    kprint("\n");

    kprint("Reserved Sectors: ");
    hex_to_ascii(fat_info.reserved_sectors, buf);
    kprint(buf);
    kprint("\n");

    kprint("Root Dir Entries: ");
    hex_to_ascii(fat_info.root_dir_entries, buf);
    kprint(buf);
    kprint("\n");

    kfree(buffer);
}