[BITS 64]
global gdt_flush
global tss_flush

gdt_flush:
    lgdt [rdi]      ; Load GDT
    
    ; 세그먼트 레지스터 갱신
    mov ax, 0x10    ; Kernel Data (0x10)
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    
    ; CS 갱신 (Far Return 이용)
    pop rdi         ; 리턴 주소 꺼냄
    
    mov rax, 0x08   ; Kernel Code
    push rax        ; CS
    push rdi        ; RIP
    retfq           ; Far Return (Pops RIP, CS)

tss_flush:
    mov ax, 0x28    ; TSS Segment Index (5 * 8 = 40 = 0x28)
    ltr ax
    ret
