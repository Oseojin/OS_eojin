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

    // 오프셋 계산
    fat_start_sector = fat_info.reserved_sectors;
    root_dir_sector = fat_start_sector + (fat_info.fat_count * fat_info.fat_size_16);
    
    uint32_t root_dir_size = (fat_info.root_dir_entries * 32) / fat_info.bytes_per_sector;
    data_start_sector = root_dir_sector + root_dir_size;

    kprint("Data Start Sector: ");
    hex_to_ascii(data_start_sector, buf);
    kprint(buf);
    kprint("\n");

    kfree(buffer);
}

void    fat_list()
{
    if (fat_info.root_dir_entries == 0)
    {
        kprint("FAT not initialized.\n");
        return;
    }

    uint32_t root_dir_size_bytes = fat_info.root_dir_entries * 32;
    uint32_t root_dir_sectors = root_dir_size_bytes / fat_info.bytes_per_sector;

    // 1섹터 단위 버퍼 할당 (512바이트)
    uint8_t* buffer = (uint8_t*)kmalloc(512);
    if (!buffer)
    {
        kprint("ls: Memory allocation failed\n");
        return;
    }

    kprint("Filename        Size\n");
    kprint("--------------------\n");

    for (int i = 0; i < root_dir_sectors; i++)
    {
        ata_read_sector(root_dir_sector + i, buffer);
        fat_dir_entry_t* entry = (fat_dir_entry_t*)buffer;

        // 1섹터 당 16개 엔트리 (512 / 32 = 16)
        for (int j = 0; j < 16; j++)
        {
            // 0x00: End of Directory (전체 종료)
            if (entry[j].name[0] == 0x00)
            {
                kfree(buffer);
                return;
            }

            // 0xE5: Deleted File
            if (entry[j].name[0] == 0xE5)
            {
                continue;
            }

            // Long File Name or Volume ID Skip
            if (entry[j].attributes == 0x0F || (entry[j].attributes & ATTR_VOLUME_ID))
            {
                continue;
            }

            // 파일 이름 출력 (8.3 포맷)
            char name[9];
            char ext[4];
            memcpy(name, (char*)entry[j].name, 8);
            memcpy(ext, (char*)entry[j].ext, 3);
            name[8] = 0;
            ext[3] = 0;

            // 공백 제거
            for (int k = 7; k >= 0; k--)
            {
                if (name[k] == ' ') name[k] = 0;
                else break;
            }
            for (int k = 2; k >= 0; k--)
            {
                if (ext[k] == ' ') ext[k] = 0;
                else break;
            }

            kprint(name);
            if (ext[0] != 0)
            {
                kprint(".");
                kprint(ext);
            }

            // 패딩 맞추기 (단순화)
            kprint("        ");
            
            // 크기 출력
            char size_buf[32];
            hex_to_ascii(entry[j].file_size, size_buf);
            kprint(size_buf);
            kprint("\n");
        }
    }

    kfree(buffer);
}