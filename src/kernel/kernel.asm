[BITS 32]
[ORG 0x1000]        ; 부트로더에서 해당 주소로 로드함

kernel_main:
    mov ebx, 0xb8000
    add ebx, 2          ; 'P' 다음 칸(0xb8002)
    mov byte [ebx], 'K'
    mov byte [ebx+1], 0x0f

    jmp $

; 커널 크기 확보
times 1024 db 0