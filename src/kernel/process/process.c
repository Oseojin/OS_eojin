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
    char buf[32];
    hex_to_ascii(p->pid, buf);
    kprint(buf);
    kprint("\n");
}

extern char* fat_get_current_path();

void kill_current_process()
{
    if (current_pid == -1) return;
    
    processes[current_pid].state = PROCESS_DEAD;
    kprint("Process Killed PID: ");
    char buf[32];
    hex_to_ascii(current_pid, buf);
    kprint(buf);
    kprint("\n");
    
    // Reprint prompt
    kprint("OS_eojin:");
    kprint(fat_get_current_path());
    kprint("> ");
    
    // Force schedule will happen on next timer interrupt or we can trigger it.
    // But here we are called from syscall handler, so we just set state.
    // The ISR will call schedule() next.
}

uint64_t schedule(uint64_t current_rsp)
{
    // Save current context (only if not dead)
    if (processes[current_pid].state != PROCESS_DEAD)
    {
        processes[current_pid].rsp = current_rsp;
        if (processes[current_pid].state == PROCESS_RUNNING)
            processes[current_pid].state = PROCESS_READY;
    }

    // Round Robin: Find next READY process
    int next = current_pid;
    int loop_count = 0;
    
    do {
        next++;
        if (next >= process_count) next = 0;
        loop_count++;
        // Infinite loop prevention if all are dead/waiting (should not happen with shell)
        if (loop_count > MAX_PROCESSES + 1) return current_rsp; 
        
    } while (processes[next].state != PROCESS_READY && next != current_pid);

    // If we found the same process and it is DEAD
    if (next == current_pid && processes[next].state != PROCESS_READY)
    {
        // Fallback to Shell (PID 0) if valid
        if (processes[0].state == PROCESS_READY || processes[0].state == PROCESS_RUNNING)
        {
            // kprint("Switching to Shell (PID 0)\n");
            next = 0;
            
            // Debug Stack
            /*
            uint64_t* s = (uint64_t*)processes[0].rsp;
            kprint("PID 0 RSP: ");
            char buf[32];
            hex_to_ascii((uint64_t)s, buf); kprint(buf);
            
            // registers_t structure:
            // ds, rax..rdi, r8..r15, int_no, err_code, rip, cs, ...
            // ds(1) + regs(15) + int_no(1) + err(1) = 18 qwords
            // stack[18] = RIP, stack[19] = CS
            
            kprint(" RIP: ");
            hex_to_ascii(s[18], buf); kprint(buf);
            kprint(" CS: ");
            hex_to_ascii(s[19], buf); kprint(buf);
            kprint("\n");
            */
        }
    }
    
    if (next != current_pid)
    {
        /*
        if (next == 0)
        {
            uint64_t* s = (uint64_t*)processes[0].rsp;
            kprint("To PID 0. RSP: ");
            char buf[32];
            hex_to_ascii((uint64_t)s, buf); kprint(buf);
            kprint(" RIP: ");
            hex_to_ascii(s[18], buf); kprint(buf);
            kprint(" CS: ");
            hex_to_ascii(s[19], buf); kprint(buf);
            kprint(" RFLAGS: ");
            hex_to_ascii(s[20], buf); kprint(buf);
            kprint(" SS: ");
            hex_to_ascii(s[22], buf); kprint(buf);
            kprint("\n");
        }
        */
    }

    current_pid = next;
    processes[current_pid].state = PROCESS_RUNNING;
    
    return processes[current_pid].rsp;
}
