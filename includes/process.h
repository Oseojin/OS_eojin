#ifndef PROCESS_H
#define PROCESS_H

#include <stdint.h>

#define MAX_PROCESSES 16
#define STACK_SIZE 4096

typedef enum {
    PROCESS_READY,
    PROCESS_RUNNING,
    PROCESS_WAITING,
    PROCESS_DEAD
} process_state_t;

typedef struct {
    uint64_t    rsp;        // Context Switch 시 저장할 RSP
    uint64_t    pid;
    process_state_t state;
    uint64_t    stack_base; // 유저 스택 (메모리 해제용)
    
    uint64_t    kernel_stack_base; // 커널 스택 (Ring 0 진입용)
    uint64_t    kernel_stack_top;
} process_t;

void    init_multitasking();
void    create_kernel_process(void (*entry)());
void    create_user_process(void (*entry)());
void    kill_current_process();
uint64_t schedule(uint64_t current_rsp); // 컨텍스트 스위칭 요청

#endif