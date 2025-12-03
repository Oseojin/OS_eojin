[BITS 64]
[EXTERN isr_handler]    ; C 코드에 작성할 공용 핸들러 함수

global isr0
global irq0
global irq1

%macro ISR_NOERRCODE 1
    global isr%1
    isr%1:
        cli
        push 0          ; 더미 에러 코드
        push %1         ; 인터럽트 번호
        jmp isr_common_stub
%endmacro

%macro IRQ 2
    global irq%1
    irq%1:
        cli
        push 0
        push %2
        jmp isr_common_stub
%endmacro

; 핸들러 정의
ISR_NOERRCODE 0         ; isr0 (Divide by Zero)
IRQ 0, 32               ; irq0 (Timer)
IRQ 1, 33               ; irq1 (Keyboard)

; 공통 처리 루틴
isr_common_stub:
    ; 64비트 레지스터 저장
    push r15
    push r14
    push r13
    push r12
    push r11
    push r10
    push r9
    push r8
    push rdi
    push rsi
    push rbp
    push rdx
    push rcx
    push rbx
    push rax
    
    ; 세그먼트 레지스터는 64비트에서 필수는 아님
    mov ax, ds          ; 데이터 세그먼트 저장
    push rax

    mov ax, 0x10        ; 커널 데이터 세그먼트 디스크립터 (0x10) 로드 (GDT64 기준)
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    ; C 핸들러 호출 (System V AMD64 ABI)
    ; 첫 번째 인자는 RDI 레지스터에 넣어서 전달해야 함
    ; 현재 스택 포인터(RSP)가 registers_t 구조체의 시작점
    mov rdi, rsp
    call isr_handler

    pop rax             ; 원래 데이터 세그먼트 복구
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    pop rax
    pop rbx
    pop rcx
    pop rdx
    pop rbp
    pop rsi
    pop rdi
    pop r8
    pop r9
    pop r10
    pop r11
    pop r12
    pop r13
    pop r14
    pop r15

    add rsp, 16         ; Error Code(8) + Int No(8) 스택 정리
    sti                 ; 인터럽트 다시 허용
    iretq               ; 64비트 인터럽트 복귀