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
uint32_t    root_dir_sector;
uint32_t    data_start_sector; // Missing variable restored
uint16_t    current_dir_cluster = 0; // 0 = Root
char        current_path[256] = "/";
static int  is_fat_initialized = 0;

char*   fat_get_current_path()
{
    return current_path;
}

uint16_t fat_get_current_cluster()
{
    return current_dir_cluster;
}

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
    is_fat_initialized = 1;
}

// 다음 클러스터 찾기
uint16_t fat_next_cluster(uint16_t cluster)
{
    uint32_t fat_offset = cluster * 2;
    uint32_t fat_sector = fat_start_sector + (fat_offset / 512);
    uint32_t ent_offset = fat_offset % 512;

    uint8_t* buffer = (uint8_t*)kmalloc(512);
    ata_read_sector(fat_sector, buffer);

    uint16_t next_cluster = *(uint16_t*)&buffer[ent_offset];

    kfree(buffer);
    return next_cluster;
}

void fat_write_fat_entry(uint16_t cluster, uint16_t value)
{
    uint32_t fat_offset = cluster * 2;
    uint32_t fat_sector = fat_start_sector + (fat_offset / 512);
    uint32_t ent_offset = fat_offset % 512;

    uint8_t* buffer = (uint8_t*)kmalloc(512);
    ata_read_sector(fat_sector, buffer);

    *(uint16_t*)&buffer[ent_offset] = value;

    ata_write_sector(fat_sector, buffer);
    kfree(buffer);
}

uint16_t fat_allocate_cluster()
{
    uint8_t* buffer = (uint8_t*)kmalloc(512);
    
    // FAT 섹터 순회
    // FAT16 size is fat_size_16 sectors
    for (int i = 0; i < fat_info.fat_size_16; i++)
    {
        ata_read_sector(fat_start_sector + i, buffer);
        
        // 섹터 내 엔트리 순회 (512 bytes / 2 bytes = 256 entries)
        for (int j = 0; j < 256; j++)
        {
            uint16_t cluster = (i * 256) + j;
            if (cluster < 2) continue; // 0, 1 reserved

            uint16_t val = *(uint16_t*)&buffer[j*2];
            if (val == 0x0000)
            {
                // Found Free
                fat_write_fat_entry(cluster, 0xFFFF);
                kfree(buffer);
                return cluster;
            }
        }
    }

    kfree(buffer);
    return 0; // No free space
}

uint32_t fat_lba_of_cluster(uint16_t cluster)
{
    return data_start_sector + ((cluster - 2) * fat_info.sectors_per_cluster);
}

// Helper: Read directory sector (Root or Sub)
int fat_read_dir_sector(uint16_t start_cluster, int sector_idx, uint8_t* buffer)
{
    if (start_cluster == 0)
    {
        // Root Directory
        uint32_t root_dir_size_bytes = fat_info.root_dir_entries * 32;
        uint32_t root_dir_sectors = root_dir_size_bytes / fat_info.bytes_per_sector;
        
        if (sector_idx >= root_dir_sectors) return 0;
        
        ata_read_sector(root_dir_sector + sector_idx, buffer);
        return 1;
    }
    else
    {
        // Sub Directory (Cluster Chain)
        int spc = fat_info.sectors_per_cluster;
        int cluster_steps = sector_idx / spc;
        int sector_offset = sector_idx % spc;
        
        uint16_t curr = start_cluster;
        for (int i = 0; i < cluster_steps; i++)
        {
            if (curr >= 0xFFF8) return 0;
            curr = fat_next_cluster(curr);
        }
        
        if (curr >= 0xFFF8) return 0;
        
        uint32_t lba = fat_lba_of_cluster(curr) + sector_offset;
        ata_read_sector(lba, buffer);
        return 1;
    }
}

// 파일 이름 비교를 위한 8.3 변환 및 비교 (대소문자 무시)
int fat_filename_match(char* name, char* ext, char* input)
{
    char temp[12];
    int  idx = 0;
    
    // Name
    for (int i = 0; i < 8; i++)
    {
        if (name[i] == ' ') break;
        temp[idx++] = name[i];
    }
    
    // Ext
    if (ext[0] != ' ')
    {
        temp[idx++] = '.';
        for (int i = 0; i < 3; i++)
        {
            if (ext[i] == ' ') break;
            temp[idx++] = ext[i];
        }
    }
    temp[idx] = 0;

    // 대소문자 무시 비교
    int i = 0;
    while (temp[i] != 0 && input[i] != 0)
    {
        char c1 = temp[i];
        char c2 = input[i];
        
        // 대문자로 통일
        if (c1 >= 'a' && c1 <= 'z') c1 -= 32;
        if (c2 >= 'a' && c2 <= 'z') c2 -= 32;
        
        if (c1 != c2) return 0;
        i++;
    }
    
    // 길이가 같은지 확인
    return (temp[i] == 0 && input[i] == 0);
}

void    fat_list()
{
    if (!is_fat_initialized)
    {
        kprint("File system not mounted. Run 'mount' first.\n");
        return;
    }

    // 1섹터 단위 버퍼 할당 (512바이트)
    uint8_t* buffer = (uint8_t*)kmalloc(512);
    if (!buffer)
    {
        kprint("ls: Memory allocation failed\n");
        return;
    }

    kprint("Filename        Size\n");
    kprint("--------------------\n");

    for (int i = 0; ; i++)
    {
        if (!fat_read_dir_sector(current_dir_cluster, i, buffer)) break;

        fat_dir_entry_t* entry = (fat_dir_entry_t*)buffer;

        // 1섹터 당 16개 엔트리 (512 / 32 = 16)
        for (int j = 0; j < 16; j++)
        {
            // 0x00: End of Directory
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
            
            if (entry[j].attributes & ATTR_DIRECTORY)
            {
                kprint("/"); // 디렉토리 표시
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

int fat_find_file(char* filename, fat_dir_entry_t* entry_out)
{
    if (!is_fat_initialized)
    {
        kprint("File system not mounted. Run 'mount' first.\n");
        return 0;
    }

    uint8_t* buffer = (uint8_t*)kmalloc(512);
    if (!buffer) return 0;

    for (int i = 0; ; i++)
    {
        if (!fat_read_dir_sector(current_dir_cluster, i, buffer)) break;

        fat_dir_entry_t* entry = (fat_dir_entry_t*)buffer;

        for (int j = 0; j < 16; j++)
        {
            if (entry[j].name[0] == 0x00) { kfree(buffer); return 0; }
            if (entry[j].name[0] == 0xE5) continue;
            if (entry[j].attributes == 0x0F || (entry[j].attributes & ATTR_VOLUME_ID)) continue;

            if (fat_filename_match((char*)entry[j].name, (char*)entry[j].ext, filename))
            {
                memcpy((char*)entry_out, (char*)&entry[j], sizeof(fat_dir_entry_t));
                kfree(buffer);
                return 1;
            }
        }
    }

    kfree(buffer);
    return 0;
}

void fat_change_dir(char* dirname)
{
    if (!is_fat_initialized)
    {
        kprint("File system not mounted. Run 'mount' first.\n");
        return;
    }

    fat_dir_entry_t entry;
    if (fat_find_file(dirname, &entry))
    {
        if (entry.attributes & ATTR_DIRECTORY)
        {
            current_dir_cluster = entry.first_cluster_low;
            kprint("Changed directory.\n");

            // 경로 문자열 업데이트
            if (strcmp(dirname, "..") == 0)
            {
                int len = strlen(current_path);
                if (len > 1) 
                {
                    for (int i = len - 1; i >= 0; i--)
                    {
                        if (current_path[i] == '/')
                        {
                            if (i == 0) current_path[1] = 0;
                            else current_path[i] = 0;
                            break;
                        }
                    }
                }
            }
            else if (strcmp(dirname, ".") == 0) { } // No-op for current directory
            else
            {
                // 실제 저장된 이름으로 경로 업데이트
                char real_name[9];
                memcpy(real_name, (char*)entry.name, 8);
                real_name[8] = 0;
                // 공백 제거
                for(int k=7; k>=0; k--) { 
                    if(real_name[k] == ' ') real_name[k]=0; 
                    else break; 
                }

                int len = strlen(current_path);
                int dlen = strlen(real_name);
                
                if (len + dlen + 2 < 256)
                {
                    if (len > 1 && current_path[len-1] != '/') 
                    {
                        current_path[len] = '/';
                        len++;
                    }
                    for(int i=0; i<dlen; i++) current_path[len+i] = real_name[i];
                    current_path[len+dlen] = 0;
                }
            }
        }
        else
        {
            kprint("Not a directory.\n");
        }
    }
    else
    {
        kprint("Directory not found.\n");
    }
}

int fat_create_entry(char* filename, uint8_t attr, uint16_t cluster, uint32_t size)
{
    if (!is_fat_initialized) return 0;

    // 8.3 포맷 변환
    char name[8];
    char ext[3];
    memset(name, ' ', 8);
    memset(ext, ' ', 3);

    int i = 0;
    int j = 0;
    // Name parsing
    while (filename[i] != 0 && filename[i] != '.' && j < 8)
    {
        // 대문자 변환 (간단하게)
        char c = filename[i];
        if (c >= 'a' && c <= 'z') c -= 32;
        name[j++] = c;
        i++;
    }
    // Skip to extension
    while (filename[i] != 0 && filename[i] != '.') i++;
    if (filename[i] == '.')
    {
        i++;
        j = 0;
        while (filename[i] != 0 && j < 3)
        {
            char c = filename[i];
            if (c >= 'a' && c <= 'z') c -= 32;
            ext[j++] = c;
            i++;
        }
    }

    // 빈 슬롯 찾기
    uint8_t* buffer = (uint8_t*)kmalloc(512);
    
    // 디렉토리 순회 (fat_find_file과 유사하지만 쓰기 위해 섹터 번호가 필요)
    int sector_idx = 0;
    while (1)
    {
        if (!fat_read_dir_sector(current_dir_cluster, sector_idx, buffer)) break;

        fat_dir_entry_t* entries = (fat_dir_entry_t*)buffer;
        int found = -1;

        for (int k = 0; k < 16; k++)
        {
            if (entries[k].name[0] == 0x00 || entries[k].name[0] == 0xE5)
            {
                found = k;
                break;
            }
        }

        if (found != -1)
        {
            // Found free slot
            fat_dir_entry_t* target = &entries[found];
            memcpy((char*)target->name, name, 8);
            memcpy((char*)target->ext, ext, 3);
            target->attributes = attr;
            target->reserved = 0;
            target->create_time = 0;
            target->create_date = 0;
            target->first_cluster_high = 0;
            target->first_cluster_low = cluster;
            target->file_size = size;

            // Write back
            // 섹터 번호 계산 다시 필요
            uint32_t lba = 0;
            if (current_dir_cluster == 0)
            {
                lba = root_dir_sector + sector_idx;
            }
            else
            {
                // Subdir LBA calc (Simplified, assuming 1 sector/cluster again or linear scan)
                // fat_read_dir_sector에서 계산했던 로직을 다시 써야함.
                // 여기서는 간단히 fat_read_dir_sector가 1을 반환했으므로
                // 그 LBA를 알아내야 함.
                // fat_read_dir_sector를 수정해서 LBA를 반환하거나, 여기서 다시 계산.
                
                int spc = fat_info.sectors_per_cluster;
                int cluster_steps = sector_idx / spc;
                int sector_offset = sector_idx % spc;
                uint16_t curr = current_dir_cluster;
                for (int s = 0; s < cluster_steps; s++) curr = fat_next_cluster(curr);
                lba = fat_lba_of_cluster(curr) + sector_offset;
            }

            ata_write_sector(lba, buffer);
            kfree(buffer);

            // 생성된 이름 출력
            char final_name[13];
            int n = 0;
            for(int k=0; k<8; k++) if(target->name[k] != ' ') final_name[n++] = target->name[k];
            if (target->ext[0] != ' ') {
                final_name[n++] = '.';
                for(int k=0; k<3; k++) if(target->ext[k] != ' ') final_name[n++] = target->ext[k];
            }
            final_name[n] = 0;
            
            kprint("Entry created: ");
            kprint(final_name);
            kprint("\n");

            return 1;
        }

        sector_idx++;
    }

    kfree(buffer);
    kprint("Directory full or error.\n");
    return 0;
}

void fat_free_cluster_chain(uint16_t cluster)
{
    if (cluster < 2) return;

    uint16_t curr = cluster;
    while (curr < 0xFFF8)
    {
        uint16_t next = fat_next_cluster(curr);
        fat_write_fat_entry(curr, 0x0000); // Free
        curr = next;
    }
}

// Check if directory is empty (only . and .. allowed)
int fat_is_dir_empty(uint16_t cluster)
{
    uint8_t* buffer = (uint8_t*)kmalloc(512);
    if (!buffer) return 0;

    int is_empty = 1;
    for (int i = 0; ; i++)
    {
        if (!fat_read_dir_sector(cluster, i, buffer)) break;

        fat_dir_entry_t* entries = (fat_dir_entry_t*)buffer;
        for (int j = 0; j < 16; j++)
        {
            if (entries[j].name[0] == 0x00) 
            {
                kfree(buffer);
                return is_empty; // End of dir
            }
            if (entries[j].name[0] == 0xE5) continue; // Deleted
            
            // Check for . and ..
            if (entries[j].name[0] == '.')
            {
                if (entries[j].name[1] == ' ' || (entries[j].name[1] == '.' && entries[j].name[2] == ' '))
                {
                    continue; // Skip . and ..
                }
            }

            // Found actual file/dir
            is_empty = 0;
            kfree(buffer);
            return 0;
        }
    }
    kfree(buffer);
    return is_empty;
}

int fat_delete_file(char* filename)
{
    if (!is_fat_initialized) return 0;

    uint8_t* buffer = (uint8_t*)kmalloc(512);
    int sector_idx = 0;

    while (1)
    {
        if (!fat_read_dir_sector(current_dir_cluster, sector_idx, buffer)) break;

        fat_dir_entry_t* entries = (fat_dir_entry_t*)buffer;
        int found = -1;

        for (int k = 0; k < 16; k++)
        {
            if (entries[k].name[0] == 0x00) break; // End
            if (entries[k].name[0] == 0xE5) continue;
            if (entries[k].attributes & ATTR_VOLUME_ID) continue;

            if (fat_filename_match((char*)entries[k].name, (char*)entries[k].ext, filename))
            {
                if (entries[k].attributes & ATTR_DIRECTORY)
                {
                    kprint("Error: Is a directory.\n");
                    kfree(buffer);
                    return 0;
                }
                found = k;
                break;
            }
        }

        if (found != -1)
        {
            // Calculate LBA to write back
            uint32_t lba = 0;
            if (current_dir_cluster == 0)
            {
                lba = root_dir_sector + sector_idx;
            }
            else
            {
                int spc = fat_info.sectors_per_cluster;
                int cluster_steps = sector_idx / spc;
                int sector_offset = sector_idx % spc;
                uint16_t curr = current_dir_cluster;
                for (int s = 0; s < cluster_steps; s++) curr = fat_next_cluster(curr);
                lba = fat_lba_of_cluster(curr) + sector_offset;
            }

            // Free Clusters
            fat_free_cluster_chain(entries[found].first_cluster_low);

            // Mark deleted
            entries[found].name[0] = 0xE5;

            // Write back
            ata_write_sector(lba, buffer);
            kfree(buffer);
            kprint("File deleted.\n");
            return 1;
        }
        sector_idx++;
    }

    kfree(buffer);
    kprint("File not found.\n");
    return 0;
}

int fat_delete_dir(char* dirname)
{
    if (!is_fat_initialized) return 0;

    uint8_t* buffer = (uint8_t*)kmalloc(512);
    int sector_idx = 0;

    while (1)
    {
        if (!fat_read_dir_sector(current_dir_cluster, sector_idx, buffer)) break;

        fat_dir_entry_t* entries = (fat_dir_entry_t*)buffer;
        int found = -1;

        for (int k = 0; k < 16; k++)
        {
            if (entries[k].name[0] == 0x00) break;
            if (entries[k].name[0] == 0xE5) continue;

            if (fat_filename_match((char*)entries[k].name, (char*)entries[k].ext, dirname))
            {
                if (!(entries[k].attributes & ATTR_DIRECTORY))
                {
                    kprint("Error: Not a directory.\n");
                    kfree(buffer);
                    return 0;
                }
                found = k;
                break;
            }
        }

        if (found != -1)
        {
            // Check if empty
            if (!fat_is_dir_empty(entries[found].first_cluster_low))
            {
                kprint("Error: Directory not empty.\n");
                kfree(buffer);
                return 0;
            }

            // Calculate LBA
            uint32_t lba = 0;
            if (current_dir_cluster == 0)
            {
                lba = root_dir_sector + sector_idx;
            }
            else
            {
                int spc = fat_info.sectors_per_cluster;
                int cluster_steps = sector_idx / spc;
                int sector_offset = sector_idx % spc;
                uint16_t curr = current_dir_cluster;
                for (int s = 0; s < cluster_steps; s++) curr = fat_next_cluster(curr);
                lba = fat_lba_of_cluster(curr) + sector_offset;
            }

            // Free Clusters
            fat_free_cluster_chain(entries[found].first_cluster_low);

            // Mark deleted
            entries[found].name[0] = 0xE5;

            ata_write_sector(lba, buffer);
            kfree(buffer);
            kprint("Directory deleted.\n");
            return 1;
        }
        sector_idx++;
    }

    kfree(buffer);
    kprint("Directory not found.\n");
    return 0;
}
