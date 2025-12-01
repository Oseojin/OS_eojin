[BITS 16]
[ORG 0x7c00]

start:
    ; 세그먼트 초기화
    xor ax, ax ; AX = 0
    mov ds, ax ; DS = 0, Data Segment: 데이터 위치
    mov es, ax ; ES = 0, Extra Segment: 추가 데이터 위치
    mov ss, ax ; SS = 0, Stack Segment: 스택 위치
    mov sp, 0x7c00 ; SP = 0x7c00, Stack Pointer: 스택 포인터, 부트로더 아래쪽

    mov si, msg
    call print_string
    jmp $

print_string:
    mov ah, 0x0e
    ; 페이지 번호 초기화
    xor bx, bx
.loop:
    lodsb
    cmp al, 0
    je .done

    int 0x10
    jmp .loop
.done:
    ret

msg db 'Hello, OS_eojin!', 0

; 모든 메모리는 이 위에서 할당하기!
; 부트로더 크기 512 바이트로 설정 (0으로 초기화)
times 510 - ($ - $$) db 0

; 매직 넘버
dw 0xaa55

