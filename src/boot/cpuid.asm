; CPUID 지원 여부 확인
check_cpuid:
    pushfd
    pop eax
    mov ecx, EAX
    xor eax, 1 << 21    ; ID 비트 반전
    push eax
    popfd
    pushfd
    pop eax
    push ecx
    popfd
    cmp eax, ecx        ; 비트가 반전된 채로 유지되면 CPUID 지원함
    je .no_cpuid
    ret

.no_cpuid:
    mov si, msg_no_cpuid
    call print_string
    jmp $

; Long Mode 지원 여부 확인
check_long_mode:
    mov eax, 0x80000000
    cpuid
    cmp eax, 0x80000001
    jb .no_long_mode

    mov eax, 0x80000001
    cpuid
    test edx, 1 << 29   ; LM 비트 확인
    jz .no_long_mode
    ret

.no_long_mode:
    mov si, msg_no_long_mode
    call print_string
    jmp $

msg_no_cpuid db "Error: CPUID not supported.", 0
msg_no_long_mode db "Error: Long Mode not supported.", 0