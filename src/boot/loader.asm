[BITS 16]
[ORG 0x7e00]

stage2_start:
    mov si, msg_stage2
    call print_string

    ; 커널 로딩
    mov ax, 0x1000
    mov es, ax
    xor bx, bx                  ; 0x10000에 커널 로딩

    mov dh, 20                  ; 커널 크기만큼
    mov dl, [BOOT_DRIVE_2]      ; Stage 1에서 드라이브 번호를 넘겨줘야 함.
    ; Stage 1에서 DL 레지스터 보존해서 넘겨주거나 변수에 저장해야함.

    ; 32비트 보호 모드 전환
    cli
    lgdt [gdt_descriptor]
    mov eax, cr0
    or eax, 0x1
    mov cr0, eax
    jmp CODE_SEG:init_pm

; 유틸리티 (16비트)
print_string:
    mov ah, 0x0e
.loop:
    lodsb
    cmp al, 0
    je .done
    int 0x10
    jmp .loop
.done:
    ret

msg_stage2 db "Stage 2 Loaded.", 13, 10, 0
BOOT_DRIVE_2 db 0

; GDT
gdt_start:
gdt_null:
    dd 0
    dd 0
gdt_code:
    dw 0xffff
    dw 0x0
    db 0x0
    db 10011010b
    db 11001111b
    db 0x0
gdt_data:
    dw 0xffff
    dw 0x0
    db 0x0
    db 10010010b
    db 11001111b
    db 0x0
gdt_end:

gdt_descriptor:
    dw gdt_end - gdt_start - 1
    dd gdt_start

CODE_SEG equ gdt_code - gdt_start
DATA_SEG equ gdt_data - gdt_start

; 64-bit GDT
gdt64_start:
    dq 0
gdt64_code:
    dd 0x00000000
    dd 0x00209a00
gdt64_data:
    dd 0x00000000
    dd 0x00009200
gdt64_end:

gdt64_descriptor:
    dw gdt64_end - gdt64_start - 1
    dq gdt64_start

; 32-bit 보호 모드
[BITS 32]
init_pm:
    mov ax, DATA_SEG
    mov ds, ax
    mov ss, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    call check_cpuid
    call check_long_mode
    call setup_paging
    call enable_paging

    lgdt [gdt64_descriptor]
    jmp 0x08:init_lm

; 64-bit Long Mode
[BITS]
init_lm:
    mov ax, 0
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    mov rsp, 0x90000

    ; 커널 위치
    ; Stage 1에서 50섹터를 읽었으므로 0x7e00 바로 뒤쪽에 커널이 붙어있음
    ; 하지만 Loader 크기가 가변적이라 커널 위치 찾기가 힘들다.
    ; -> Loader를 섹터 단위로 고정하고(지금은 2KB), 그 뒤에 커널을 배치한다.
    ; ex)
    ; Loader = 0x7e00 ~ 0x8600 (2KB)
    ; Kernel = 0x8600 부터 시작

    mov rax, 0x8600     ; 커널 예상 위치
    call rax
    jmp $

%include "src/boot/cpuid.asm"
%include "src/boot/long_mode_init.asm"

; Loader 패딩 (2KB = 2048 바이트로 맞춤)
times 2048 - ($ - $$) db 0