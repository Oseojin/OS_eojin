#include "../../includes/kheap.h"
#include "../../includes/pmm.h"
#include "../../includes/utils.h"

// 외부 함수 선언
// utils.c
extern void kprint(char* message);

block_header_t* head = 0;

void    init_kheap()
{
    // PMM에서 첫 페이지 할당 (4KB)
    void*   heap_start = pmm_alloc_block();
    if (!heap_start)
    {
        kprint("KHEAP: Init failed (OOM)\n");
        return;
    }

    // 첫 번째 헤더 생성
    head = (block_header_t*)heap_start;
    head->next = 0;
    head->size = BLOCK_SIZE - sizeof(block_header_t); // 헤더 크기를 뺀 나머지가 데이터
    head->is_free = 1;

    kprint("HEAP Initialized.\n");
}

void*   kmalloc(size_t size)
{
    if (size == 0)
    {
        return 0;
    }

    block_header_t* curr = head;
    block_header_t* last = head;

    // First Fit
    while (curr)
    {
        // 적합한 블록
        if (curr->is_free && curr->size >= size)
        {
            // Splitting 가능 여부
            if (curr->size > size + sizeof(block_header_t) + 8)
            {
                block_header_t* new_block = (block_header_t*)((uint64_t)curr + sizeof(block_header_t) + size);

                new_block->size = curr->size - size - sizeof(block_header_t);
                new_block->is_free = 1;
                new_block->next = curr->next;

                curr->size = size;
                curr->next = new_block;
            }

            curr->is_free = 0;
            // 헤더 바로 뒤 주소 (데이터 영역) 반환
            return (void*)((uint64_t)curr + sizeof(block_header_t));
        }
        last = curr;
        curr = curr->next;
    }

    // 탐색 실패 -> 힙 확장 (PMM에서 추가 할당)
    // 새 페이지 할당
    void*   new_page = pmm_alloc_block();
    if (!new_page)
    {
        kprint("KHEAP: PMM OOM! Cannot expand heap.\n");
        return 0;
    }

    // 새 블록 헤더 설정
    block_header_t* new_block = (block_header_t*)new_page;
    new_block->size = BLOCK_SIZE - sizeof(block_header_t);
    new_block->is_free = 1;
    new_block->next = 0;

    // 리스트 연결
    if (last)
    {
        last->next = new_block;
    }
    else
    {
        head = new_block;
    }

    // 병합 조건
    // last == Free
    // last->next == new_block
    uint64_t    last_end = (uint64_t)last + sizeof(block_header_t) + last->size;

    if (last && last->is_free && last_end == (uint64_t)new_block)
    {
        last->size += sizeof(block_header_t) + new_block->size;
        last->next = new_block->next;
    }
    else if(last)
    {
        last->next = new_block;
    }
    
    return 0;
}

void    kfree(void* ptr)
{
    if (!ptr)
    {
        return;
    }

    block_header_t* header = (block_header_t*)((uint64_t)ptr - sizeof(block_header_t));

    header->is_free = 1;

    // 병합
    // 현재 블록과 다음 블록이 모두 free라면 병합

    block_header_t* curr = head;
    while (curr && curr->next)
    {
        if (curr->is_free && curr->next->is_free)
        {
            curr->size += sizeof(block_header_t) + curr->next->size;
            curr->next = curr->next->next;
        }
        else
        {
            curr = curr->next;
        }
    }
}