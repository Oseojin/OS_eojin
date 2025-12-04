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

    ; 로그
    mov si, msg_real_mode
    call print_string_rm

    ; Memory Map Detection (E820)
    ; 결과 저장 위치: 0x5000 (Count), 0x5004 (Entries)
    mov di, 0x5004      ; 맵 저장 시작 주소
    xor ebx, ebx
    xor bp, bp          ; 엔트리 개수 카운터

    mov edx, 0x534d4150 ; 'SMAP' 매직 넘버
    mov eax, 0xe820

.e820_loop:
    mov eax, 0xe820     ; 함수 코드 (호출 시 마다 리셋 될 수 있음)
    mov ecx, 24         ; 버퍼 크기 (24바이트 요청)

    int 0x15

    jc .e820_done       ; 에러 시 종료

    add di, 24          ; 다음 저장 위치로 이동
    inc bp              ; 개수 증가

    test ebx, ebx       ; ebx가 0이면 리스크 끝
    jz .e820_done

    jmp .e820_loop

.e820_done:
    mov [0x5000], bp    ; 0x5000 번지에 총 개수 저장

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
    dd 0
    dd 0
gdt_code:
    dw 0xffff
    dw 0x0000
    db 0x00
    db 0x9a
    db 0xcf
    db 0x00   ; Code (Base = 0, Limit = 4GB)
gdt_data:
    dw 0xffff
    dw 0x0000
    db 0x0
    db 0x92
    db 0xcf
    db 0x00    ; Data
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
print_string_rm:
    mov ah, 0x0e
.loop:
    lodsb
    cmp al, 0
    je .done
    int 0x10
    jmp .loop
.done:
    ret

msg_real_mode db "Successfully entered 16-bit Real Mode.", 13, 10, 0

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
    mov ebx, msg_prot_mode
    call print_string_pm

    call check_cpuid
    call check_long_mode
    call setup_paging
    call enable_paging

    lgdt [gdt64_descriptor]
    jmp 0x08:init_lm

; 32-bit 유틸리티
print_string_pm:
    pusha
    mov edx, 0xb8000
    add edx, 320
.loop:
    mov al, [ebx]
    mov ah, 0x07
    cmp al, 0
    je .done
    mov [edx], ax
    add ebx, 1
    add edx, 2
    jmp .loop
.done:
    popa
    ret

msg_prot_mode db "Successfully entered 32-bit Protected Mode.", 0

; 64-bit Long Mode
[BITS 64]
init_lm:
    ; 세그먼트 설정
    mov ax, 0
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    mov rsp, 0x90000

    ; 로그
    mov rbx, msg_long_mode
    call print_string_lm

    ; 커널 위치
    ; Stage 1에서 50섹터를 읽었으므로 0x7e00 바로 뒤쪽에 커널이 붙어있음
    ; 하지만 Loader 크기가 가변적이라 커널 위치 찾기가 힘들다.
    ; -> Loader를 섹터 단위로 고정하고(지금은 2KB), 그 뒤에 커널을 배치한다.
    ; ex)
    ; Loader = 0x7e00 ~ 0x8600 (2KB)
    ; Kernel = 0x8600 부터 시작

    mov rax, 0x8600     ; 커널 예상 위치
    call rax

; 64-bit 유틸리티
print_string_lm:
    push rbx
    push rdx
    push rax

    mov rdx, 0xb8000
    add rdx, 480
.loop:
    mov al, [rbx]
    mov ah, 0x07

    cmp al, 0
    je .done

    mov [rdx], ax
    add rbx, 1
    add rdx, 2
    jmp .loop
.done:
    pop rax
    pop rdx
    pop rbx
    ret

msg_long_mode db "Successfully entered 64-bit Long Mode.", 0

%include "src/boot/cpuid.asm"
%include "src/boot/long_mode_init.asm"

; Loader 패딩 (2KB = 2048 바이트로 맞춤)
times 2048 - ($ - $$) db 0