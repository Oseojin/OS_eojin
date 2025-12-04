#include "../../includes/utils.h"

#define ATA_DATA            0x1f0
#define ATA_ERROR           0x1f1
#define ATA_SECTOR_CNT      0x1f2
#define ATA_LBA_LOW         0x1f3
#define ATA_LBA_MID         0x1f4
#define ATA_LBA_HIGH        0x1f5
#define ATA_DRIVE_HEAD      0x1f6
#define ATA_STATUS          0x1f7
#define ATA_COMMAND         0x1f7

#define ATA_CONTROL         0x3f6

#define STATUS_BSY          0x80
#define STATUS_DRQ          0x08

#define STATUS_ERR          0x01
#define STATUS_DF           0x20

// 외부 함수 선언
extern void kprint(char* message);

void    ata_disable_irq()
{
    outb(ATA_CONTROL, 0x02);
}

// 잠시 대기 (400ns)
void    ata_io_wait()
{
    inb(ATA_STATUS);
    inb(ATA_STATUS);
    inb(ATA_STATUS);
    inb(ATA_STATUS);
}

void    ata_wait_bsy()
{
    while(inb(ATA_STATUS) & STATUS_BSY);
}

int     ata_wait_drq()
{
    int limit = 100000;
    while (limit-- > 0)
    {
        uint8_t status = inb(ATA_STATUS);
        if (status & STATUS_ERR)
        {
            kprint("ATA Error: ERR bit set!\n");
            return 0;
        }
        if (status & STATUS_DF)
        {
            kprint("ATA Error: Drive Fault!\n");
            return 0;
        }
        if (status & STATUS_DRQ)
        {
            return 1;
        }
    }
    return 0;
}

void    ata_read_sector(uint32_t lba, uint8_t* buffer)
{
    ata_disable_irq();
    ata_wait_bsy();

    outb(ATA_DRIVE_HEAD, 0xe0 | ((lba >> 24) & 0x0f)); // Master, LBA Mode
    outb(ATA_SECTOR_CNT, 1);
    outb(ATA_LBA_LOW, (uint8_t)lba);
    outb(ATA_LBA_MID, (uint8_t)(lba >> 8));
    outb(ATA_LBA_HIGH, (uint8_t)(lba >> 16));

    outb(ATA_COMMAND, 0x20); // Read Command

    if (!ata_wait_drq())
    {
        kprint("ATA Error: DRQ Timeout\n");
        return;
    }

    // 256 워드 (512 바이트) 읽기
    for (int i = 0; i < 256; i++)
    {
        uint16_t    data = inw(ATA_DATA);
        buffer[i * 2] = (uint8_t)data;
        buffer[i * 2 + 1] = (uint8_t)(data >> 8);
    }
}

void    ata_write_sector(uint32_t lba, uint8_t* data)
{
    ata_disable_irq();
    ata_wait_bsy();

    outb(ATA_DRIVE_HEAD, 0xe0 | ((lba >> 24) & 0x0f));
    outb(ATA_SECTOR_CNT, 1);
    outb(ATA_LBA_LOW, (uint8_t)lba);
    outb(ATA_LBA_MID, (uint8_t)(lba >> 8));
    outb(ATA_LBA_HIGH, (uint8_t)(lba >> 16));

    outb(ATA_COMMAND, 0x30); // Write Command

    ata_wait_bsy();
    ata_wait_drq();

    for (int i = 0; i < 256; i++)
    {
        uint16_t    word = data[i * 2] | (data[i * 2 + 1] << 8);
        outw(ATA_DATA, word);
    }

    // Cache Flush
    outb(ATA_COMMAND, 0xe7);
    ata_wait_bsy();
}