[BITS 16]
[ORG 0x7c00]

start:
    ; 세그먼트 초기화
    xor ax, ax          ; AX = 0
    mov ds, ax          ; DS = 0, Data Segment: 데이터 위치
    mov es, ax          ; ES = 0, Extra Segment: 추가 데이터 위치
    mov ss, ax          ; SS = 0, Stack Segment: 스택 위치
    mov sp, 0x7c00      ; SP = 0x7c00, Stack Pointer: 스택 포인터, 부트로더 아래쪽

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

; 부팅 메시지 저장
msg db 'Hello, OS_eojin!', 0

; GDT(Global Descriptor Table) 시작
gdt_start:

; null 디스크립터
; 8바이트이므로 dd(4바이트 할당)을 통해 전부 0으로 초기화
gdt_null:
    dd 0
    dd 0
; Code Segment 디스크립터
; Base = 0, Limit = 0xfffff, Access = 0x9a, Flags = 0xc
gdt_code:
    dw 0xffff       ; 1. Limit 하위 16비트 (전부 1)
    dw 0x0          ; 2. Base 하위 16비트 (0번지)
    db 0x0          ; 3. Base 중간 8비트 (0번지)
    
    ; 4. Access Byte: 10011010b
    ; 1 (Present): 메모리에 존재함
    ; 00 (DPL): Ring 0 (커널 레벨)
    ; 1 (S): 코드/데이터 세그먼트임 (시스템 세그먼트 아님)
    ; 1 (Type - Ex): 실행 가능 (Executable, 코드 세그먼트)
    ; 0 (Type - DC): 하위 권한에서 실행 불가 (Conforming 아님)
    ; 1 (Type - RW): 읽기 가능 (Readable)
    ; 0 (Type - Ac): 아직 접근 안 됨 (Accessed)
    db 10011010b    
    
    ; 5. Flags + Limit 상위 4비트: 11001111b
    ; 1 (G): Granularity, 4KB 단위 설정 (이것 때문에 4GB가 됨)
    ; 1 (DB): 32비트 모드 설정 (중요! 16비트가 아님을 명시)
    ; 0 (L): 64비트 모드 아님
    ; 0 (Avl): 사용자 정의 (안 씀)
    ; 1111 : Limit 상위 4비트
    db 11001111b
    
    db 0x0          ; 6. Base 상위 8비트 (0번지)
    
; Data Segment 디스크립터
; Base = 0, Limit = 0xfffff, Access = 0x92, Flags = 0xc
gdt_data:
    dw 0xffff
    dw 0x0
    db 0x0
    db 10010010b ; 실행 불가능한 세그먼트 = 데이터 세그먼트
    db 11001111b
    db 0x0
gdt_end:
; GDT 끝

gdt_descriptor:
    dw gdt_end - gdt_start - 1  ; GDT 크기
    dd gdt_start                ; Offset

; 상수 정의
CODE_SEG equ gdt_code - gdt_start
DATA_SEG equ gdt_data - gdt_start

; 모든 메모리는 이 위에서 할당하기!
; 부트로더 크기 512 바이트로 설정 (0으로 초기화)
times 510 - ($ - $$) db 0

; 매직 넘버
dw 0xaa55

