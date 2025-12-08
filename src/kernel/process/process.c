#include "../../../includes/process.h"
#include "../../../includes/kheap.h"
#include "../../../includes/utils.h"
#include "../../../includes/gdt.h"
#include "../../../includes/vmm.h"

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
    // PID 0는 초기 부팅 스택을 사용하므로 kernel_stack 정보를 알기 어렵거나 0x90000임.
    // 하지만 PID 0로 돌아올 때는 TSS 업데이트가 필요 없을 수도 있음 (Ring 0 -> Ring 0).
    process_count = 1;
    current_pid = 0;
    
    kprint("Multitasking Initialized.\n");
}

extern void* pmm_alloc_block();

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

    // 스택 할당 (PMM 직접 사용)
    uint64_t* stack = (uint64_t*)pmm_alloc_block();
    p->stack_base = (uint64_t)stack;
    
    p->kernel_stack_base = (uint64_t)stack;
    p->kernel_stack_top = (uint64_t)stack + STACK_SIZE;
    
    // Kernel Process uses Kernel PML4
    p->pml4 = (uint64_t)kernel_pml4;

    // 스택 포인터를 스택 끝으로 이동
    uint64_t rsp = p->kernel_stack_top;

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
    
    kprint("Created Kernel Process PID ");
    char buf[32];
    hex_to_ascii(p->pid, buf);
    kprint(buf);
    kprint("\n");
}

void create_user_process(void (*entry)(), uint64_t pml4_phys_addr)
{
    if (process_count >= MAX_PROCESSES)
    {
        kprint("Max processes reached!\n");
        return;
    }

    process_t* p = &processes[process_count];
    p->pid = process_count;
    p->state = PROCESS_READY;

    // 1. 유저 스택 할당
    uint64_t* user_stack = (uint64_t*)pmm_alloc_block();
    if (!user_stack) { kprint("Proc Alloc Fail: User Stack\n"); return; }
    
    p->stack_base = (uint64_t)user_stack;
    uint64_t user_rsp = (uint64_t)user_stack + STACK_SIZE;

    // 2. 커널 스택 할당 (TSS용)
    uint64_t* kernel_stack = (uint64_t*)pmm_alloc_block();
    if (!kernel_stack) { kprint("Proc Alloc Fail: Kernel Stack\n"); return; }
    
    p->kernel_stack_base = (uint64_t)kernel_stack;
    p->kernel_stack_top = (uint64_t)kernel_stack + STACK_SIZE;
    
    // User Process gets its own PML4 (Passed argument)
    p->pml4 = pml4_phys_addr;

    // 3. 트랩 프레임 생성 (커널 스택에 생성)
    uint64_t k_rsp = p->kernel_stack_top;
    
    // User Mode IRETQ Frame
    // SS (User Data 0x18 | 3 = 0x1B)
    k_rsp -= 8; *((uint64_t*)k_rsp) = 0x1B; 
    // RSP (User Stack)
    k_rsp -= 8; *((uint64_t*)k_rsp) = user_rsp; 
    
    /*
    kprint("User RSP: ");
    char buf2[32];
    hex_to_ascii(user_rsp, buf2);
    kprint(buf2);
    kprint("\n");
    */

    // RFLAGS (IF=1, IOPL=0)
    k_rsp -= 8; *((uint64_t*)k_rsp) = 0x202; 
    // CS (User Code 0x20 | 3 = 0x23)
    k_rsp -= 8; *((uint64_t*)k_rsp) = 0x23;  
    // RIP (Entry)
    k_rsp -= 8; *((uint64_t*)k_rsp) = (uint64_t)entry; 

    // Context (Registers)
    k_rsp -= 8; *((uint64_t*)k_rsp) = 0; // Error Code
    k_rsp -= 8; *((uint64_t*)k_rsp) = 0; // Int No

    // GP Registers
    for (int i = 0; i < 16; i++)
    {
        k_rsp -= 8; *((uint64_t*)k_rsp) = 0;
    }

    p->rsp = k_rsp; 
    
    process_count++;
    
    kprint("Created User Process PID ");
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
    
    //Reprint prompt
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
            next = 0;
        }
    }
    
    current_pid = next;
    processes[current_pid].state = PROCESS_RUNNING;
    
    // Switch PML4 (CR3)
    if (processes[current_pid].pml4 != 0)
    {
        vmm_switch_pml4((page_table_t*)processes[current_pid].pml4);
    }
    
    // TSS Update (Ring 3 -> Ring 0 전환 시 사용할 스택)
    if (processes[current_pid].kernel_stack_top != 0)
    {
        set_tss_rsp0(processes[current_pid].kernel_stack_top);
    }
    
    return processes[current_pid].rsp;
}
