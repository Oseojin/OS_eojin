[BITS 16]
[ORG 0x7e00]

stage2_start:
    cli
    ; 세그먼트 초기화
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7e00

    ; 비디오 메모리에 직접 'S' 찍기
    mov ax, 0xb800
    mov es, ax
    mov byte [es:0], 'S'
    mov byte [es:1], 0x0f

    ; A20 라인 활성화 (Fast A20 방식)
    in al, 0x92
    or al, 2
    out 0x92, al

    ; GDT 주소 계산 및 수정
    xor eax, eax
    mov ax, ds
    shl eax, 4
    add eax, gdt_start
    mov [gdt_descriptor + 2], eax
    
    ; GDT 로드
    lgdt [gdt_descriptor]

    ; CR0 설정
    mov eax, cr0
    or eax, 0x1
    mov cr0, eax

    ; 점프
    jmp dword 0x08:init_pm

align 4

; GDT
gdt_start:
    dd 0, 0
gdt_code:
    dw 0xffff, 0x0000
    db 0x00, 0x9a, 0xcf, 0x00   ; Code (Base = 0, Limit = 4GB)
gdt_data:
    dw 0xffff, 0x0000
    db 0x0, 0x92, 0xcf, 0x00    ; Data
gdt_end:

align 4
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
    dd gdt64_start

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

; 32-bit 보호 모드
[BITS 32]
init_pm:
    mov ax, DATA_SEG
    mov ds, ax
    mov ss, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    ; 디버깅 코드
    mov edi, 0xb8000
    mov byte [edi], 'P'
    mov byte [edi+1], 0x0f

    call check_cpuid
    call check_long_mode
    call setup_paging
    call enable_paging

    lgdt [gdt64_descriptor]
    jmp 0x08:init_lm

; 64-bit Long Mode
[BITS 64]
init_lm:
    mov ax, 0
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    mov rsp, 0x90000

    ; 디버깅 L
    mov rax, 0xb8000
    mov byte [rax], 'L'
    mov byte [rax+1], 0x2f

    ; 커널 위치
    ; Stage 1에서 50섹터를 읽었으므로 0x7e00 바로 뒤쪽에 커널이 붙어있음
    ; 하지만 Loader 크기가 가변적이라 커널 위치 찾기가 힘들다.
    ; -> Loader를 섹터 단위로 고정하고(지금은 2KB), 그 뒤에 커널을 배치한다.
    ; ex)
    ; Loader = 0x7e00 ~ 0x8600 (2KB)
    ; Kernel = 0x8600 부터 시작

    mov rax, 0x8600     ; 커널 예상 위치
    call rax

%include "src/boot/cpuid.asm"
%include "src/boot/long_mode_init.asm"

; Loader 패딩 (2KB = 2048 바이트로 맞춤)
times 2048 - ($ - $$) db 0