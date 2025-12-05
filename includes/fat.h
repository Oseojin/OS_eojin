#ifndef FAT_H
#define FAT_H

#include <stdint.h>

// BIOS Parameter Block (Standard + FAT16 Extended)
typedef struct
{
    uint8_t     jump_boot[3];
    uint8_t     oem_name[8];
    uint16_t    bytes_per_sector;
    uint8_t     sectors_per_cluster;
    uint16_t    reserved_sectors;
    uint8_t     fat_count;
    uint16_t    root_dir_entries;
    uint16_t    total_sectors_16;
    uint8_t     media_type;
    uint16_t    fat_size_16;
    uint16_t    sectors_per_track;
    uint16_t    head_count;
    uint32_t    hidden_sectors;
    uint32_t    total_sectors_32;

    // FAT16 Extended Boot Record
    uint8_t     drive_number;
    uint8_t     reserved;
    uint8_t     boot_signature;
    uint32_t    volume_id;
    uint8_t     volume_label[11];
    uint8_t     fs_type[8];
} __attribute__((packed))   fat_bpb_t;

// Directory Entry
typedef struct
{
    uint8_t     name[8];
    uint8_t     ext[3];
    uint8_t     attributes;
    uint8_t     reserved;
    uint8_t     create_time_tenth;
    uint16_t    create_time;
    uint16_t    create_date;
    uint16_t    last_access_date;
    uint16_t    first_cluster_high;
    uint16_t    write_time;
    uint16_t    write_date;
    uint16_t    first_cluster_low;
    uint32_t    file_size;
} __attribute__((packed))   fat_dir_entry_t;

// 속성 비트 마스크
#define ATTR_READ_ONLY  0x01
#define ATTR_HIDDEN     0x02
#define ATTR_SYSTEM     0x04
#define ATTR_VOLUME_ID  0x08
#define ATTR_DIRECTORY  0x10
#define ATTR_ARCHIVE    0x20

void    fat_init();
void    fat_list();
uint16_t fat_next_cluster(uint16_t cluster);
int     fat_find_file(char* filename, fat_dir_entry_t* entry_out);
uint32_t fat_lba_of_cluster(uint16_t cluster);

#endif