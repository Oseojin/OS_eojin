[BITS 32]
[EXTERN isr_handler]    ; C 코드에 작성할 공용 핸들러 함수

global isr0
global irq0
global irq1

isr0:
    cli                 ; 인터럽트 중복 발생 방지
    push byte 0         ; 에러 코드가 없는 인터럽트를 위한 더미 데이터
    push byte 0         ; 인터럽트 번호 (0번)
    jmp isr_common_stub

; IRQ 0 (Timer) Handler
irq0:
    cli
    push byte 0
    push byte 32
    jmp isr_common_stub

; IRQ 1 (Keyboard) Handler
irq1:
    cli
    push byte 0         ; 더미 에러 코드
    push byte 33        ; 인터럽트 번호 (33번)
    jmp isr_common_stub

; 공통 처리 루틴
isr_common_stub:
    pusha               ; 모든 범용 레지스터 저장 (EAX, ECX, EDX, EBX, ESP, EBP, ESI, EDI)

    mov ax, ds          ; 데이터 세그먼트 저장
    push eax

    mov ax, 0x10        ; 커널 데이터 세그먼트 디스크립터 (0x10) 로드
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    call isr_handler    ; C 핸들러 호출

    pop eax             ; 원래 데이터 세그먼트 복구
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    popa                ; 레지스터 복구
    add esp, 8          ; 에러 코드와 인터럽트 번호 스택 정리 (push byte를 2번 했으므로 8 이동)
    sti                 ; 인터럽트 다시 허용
    iret                ; 인터럽트 복귀