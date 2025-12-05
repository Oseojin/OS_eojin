#include "../../../includes/process.h"
#include "../../../includes/kheap.h"
#include "../../../includes/utils.h"

// 외부 함수
extern void kprint(char* msg);

process_t   processes[MAX_PROCESSES];
int         current_pid = -1;
int         process_count = 0;

void init_multitasking()
{
    for (int i = 0; i < MAX_PROCESSES; i++)
    {
        processes[i].state = PROCESS_WAITING;
    }
    // PID 0 is the initial kernel task (will be captured on first schedule)
    processes[0].pid = 0;
    processes[0].state = PROCESS_RUNNING;
    process_count = 1;
    current_pid = 0;
    
    kprint("Multitasking Initialized.\n");
}

void create_kernel_process(void (*entry)())
{
    if (process_count >= MAX_PROCESSES)
    {
        kprint("Max processes reached!\n");
        return;
    }

    process_t* p = &processes[process_count];
    p->pid = process_count;
    p->state = PROCESS_READY;

    // 스택 할당
    uint64_t* stack = (uint64_t*)kmalloc(STACK_SIZE);
    p->stack_base = (uint64_t)stack;
    
    // 스택 포인터를 스택 끝으로 이동
    uint64_t rsp = (uint64_t)stack + STACK_SIZE;

    // ISR 스택 프레임 시뮬레이션 (iretq가 복구할 내용)
    // Stack grows DOWN. Pushing values.
    
    // Ring 0 thread setup
    rsp -= 8; *((uint64_t*)rsp) = 0x10; // SS
    rsp -= 8; *((uint64_t*)rsp) = rsp + 16; // RSP (approx)
    rsp -= 8; *((uint64_t*)rsp) = 0x202; // RFLAGS (IF=1)
    rsp -= 8; *((uint64_t*)rsp) = 0x08;  // CS
    rsp -= 8; *((uint64_t*)rsp) = (uint64_t)entry; // RIP

    // Error Code & Int No
    rsp -= 8; *((uint64_t*)rsp) = 0;
    rsp -= 8; *((uint64_t*)rsp) = 0;

    // Registers (R15..R8, RDI..RAX, DS)
    for (int i = 0; i < 16; i++)
    {
        rsp -= 8; *((uint64_t*)rsp) = 0;
    }

    p->rsp = rsp;
    process_count++;
    
    kprint("Created Process PID ");
    char buf[16];
    hex_to_ascii(p->pid, buf);
    kprint(buf);
    kprint("\n");
}

uint64_t schedule(uint64_t current_rsp)
{
    if (process_count <= 1) return current_rsp; // Only kernel running

    // Save current context
    processes[current_pid].rsp = current_rsp;
    if (processes[current_pid].state == PROCESS_RUNNING)
        processes[current_pid].state = PROCESS_READY;

    // Round Robin: Find next READY process
    int next = current_pid;
    do {
        next++;
        if (next >= process_count) next = 0;
    } while (processes[next].state != PROCESS_READY && next != current_pid);

    if (next == current_pid) return current_rsp; // No other ready process

    current_pid = next;
    processes[current_pid].state = PROCESS_RUNNING;
    
    return processes[current_pid].rsp;
}
