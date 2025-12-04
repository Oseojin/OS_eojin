#include "../../includes/pmm.h"
#include "../../includes/utils.h"

// 비트맵 배열 포인터 (초기화 시 위치 결정)
uint32_t*   bitmap;
uint64_t    total_blocks;
uint64_t    bitmap_size; // in bytes

// 비트 조작 매크로
#define SET_BIT(idx)    (bitmap[(idx)/32] |= (1 << ((idx)%32)))
#define CLEAR_BIT(idx)  (bitmap[(idx)/32] &= ~(1 << ((idx)%32)))
#define TEST_BIT(idx)   (bitmap[(idx)/32] & (1 << ((idx)%32)))

// 외부 함수 선언
// screen.c
extern void kprint(char* message);
// utils.c
extern void hex_to_ascii(uint64_t n, char* str);

// PMM 초기화
void    init_pmm()
{
    // e820 맵 읽기
    uint16_t            entry_count = *(uint16_t*)0x5000;
    memory_map_entry_t* entries = (memory_map_entry_t*)0x5004;

    uint64_t            max_addr = 0;

    // 가장 큰 주소 찾기 (총 메모리 크기 추정)
    for (int i = 0; i < entry_count; i++)
    {
        if (entries[i].type == 1) // Usable
        {
            uint64_t    end = entries[i].base_addr + entries[i].length;
            if (end > max_addr)
            {
                max_addr = end;
            }
        }
    }

    total_blocks = max_addr / BLOCK_SIZE;
    bitmap_size = total_blocks / 8; // 1 block = 1 bit

    // 비트맵 위치: 커널(0x8600 시작) + 커널크기.
    // 안전하게 2MB(0x200000) 지점에 비트맵 생성 가정
    bitmap = (uint32_t*)0x200000;

    // 비트맵 전체 1(Used)로 초기화 (기본적으로 모든 메모리 사용 불가)
    memset(bitmap, 0xff, bitmap_size);

    // Usable 영역만 0(Free)로 마킹
    for (int i = 0; i < entry_count; i++)
    {
        if (entries[i].type == 1)
        {
            uint64_t    start_block = entries[i].base_addr / BLOCK_SIZE;
            uint64_t    num_blocks = entries[i].length / BLOCK_SIZE;

            for (uint64_t j = 0; j < num_blocks; j++)
            {
                CLEAR_BIT(start_block + j);
            }
        }
    }

    // 커널이 사용하는 영역, 비트맵 자체 영역 등 보호
    // 0 ~ 4MB 구간을 Safe Zone으로 설정
    uint64_t    reserved_blocks = (0x400000) / BLOCK_SIZE; // 4MB
    for (uint64_t i = 0; i < reserved_blocks; i++)
    {
        SET_BIT(i);
    }

    kprint("PMM Initialized. Total Memory: ");
    char    buf[32];
    hex_to_ascii(max_addr, buf);
    kprint(buf);
    kprint("\n");
}

// 블록 할당
void*   pmm_alloc_block()
{
    for (uint64_t i = 0; i < total_blocks; i++)
    {
        if (!TEST_BIT(i)) // Free block found
        {
            SET_BIT(i);
            return (void*)(i * BLOCK_SIZE);
        }
    }
    kprint("PMM: Out of Memory!\n");
    return 0;
}

// 블록 해제
void    pmm_free_block(void* p)
{
    uint64_t    addr = (uint64_t)p;
    uint64_t    idx = addr / BLOCK_SIZE;
    CLEAR_BIT(idx);
}