[BITS 16]
[ORG 0x7c00]

start:
    ; 세그먼트 초기화
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7c00

    mov [BOOT_DRIVE], dl    ; 드라이브 번호 저장

    ; 화면 지우기
    mov ax, 0x0003
    int 0x10

    mov si, msg_stage1
    call print_string

    ; Stage 2 (loader.bin) 로딩
    ; 위치: 0x7e00 (boot 섹터 바로 뒤)
    ; 섹터: 2번부터
    ; 크기: 50섹터 (약 25KB)ㄴ

    mov ax, 0x0
    mov es, ax
    mov bx, 0x7e00           ; ES:BX = 0x0000:0x7e00

    mov dh, 50              ; 읽을 섹터 수
    mov dl, [BOOT_DRIVE]
    call disk_load

    ; Stage 2로 점프
    jmp 0x0000:0x7e00

; 유틸리티
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

disk_load:
    push dx
    mov ah, 0x02
    mov al, dh
    mov ch, 0x00
    mov dh, 0x00
    mov cl, 0x02
    int 0x13
    jc disk_error
    pop dx
    cmp dh, al
    jne disk_error
    ret

disk_error:
    mov si, msg_disk_error
    call print_string
    jmp $

; 데이터
BOOT_DRIVE db 0
msg_stage1 db "Boot Stage 1...", 13, 10, 0
msg_disk_error db "Disk Error!", 0

; 패딩
times 510 - ($ - $$) db 0
dw 0xaa55