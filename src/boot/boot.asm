[BITS 16]
[ORG 0x7c00]

start:
    ; 세그먼트 초기화
    xor ax, ax          ; AX = 0
    mov ds, ax          ; DS = 0, Data Segment: 데이터 위치
    mov es, ax          ; ES = 0, Extra Segment: 추가 데이터 위치
    mov ss, ax          ; SS = 0, Stack Segment: 스택 위치
    mov sp, 0x7c00      ; SP = 0x7c00, Stack Pointer: 스택 포인터, 부트로더 아래쪽

    mov [BOOT_DRIVE], dl    ; BIOS가 넘겨준 드라이브 번호 저장

    mov si, msg
    call print_string

    ; 커널 로드 (섹터 2부터 읽어서 0x1000에 저장)
    mov ax, 0x1000
    mov es, ax
    xor bs, bs          ; ES:BX = 0x1000:0x0000 = 0x10000 (저장할 주소)
    mov dh, 20              ; 읽을 섹터 수
    mov dl, [BOOT_DRIVE]
    call disk_load
    
    ; 보호모드 전환
    cli                     ; 16비트(리얼모드) 인터럽트 비우기(끄기)
    lgdt [gdt_descriptor]   ; GDT 로드

    mov eax, cr0
    or eax, 0x1             ; CR0 레지스터의 PE 비트(첫 비트 켜기)
    mov cr0, eax

    jmp CODE_SEG:init_pm    ; Far Jump (CS 업데이트 & 파이프라인 플러시)


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

disk_load:
    push dx         ; 개수 저장

    mov ah, 0x02    ; BIOS 섹터 읽기 함수
    mov al, dh      ; DH 섹터 읽기
    mov ch, 0x00    ; 실린더 0
    mov dh, 0x00    ; 헤드 0
    mov cl, 0x02    ; 섹터 2부터 시작

    ; DL은 BIOS가 부팅 시 자동 설정함
    ; BX는 호출하는 쪽에서 설정
    int 0x13        ; BIOS 인터럽트

    jc disk_error   ; Carry Flag가 켜지면 에러

    pop dx          ; 원래 요청 개수 복구
    cmp dh, al      ; 실제로 읽은 개수(AL)과 요청 개수(DH) 비교
    jne disk_error  ; 다르면 에러
    ret

disk_error:
    mov si, DISK_ERROR_MSG
    call print_string
    jmp $

BOOT_DRIVE db 0     ; 드라이브 번호 저장 변수
DISK_ERROR_MSG db 'Disk read error!', 0
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

; 64-bit GDT
gdt64_start:
    dq 0    ; Null 디스크립터
gdt64_code:
    ; Code 세그먼트: 디스크립터 권한 Level(0), Present(1), Read/Write(1), Executable(1), 64-bit(1)
    ; Access:   10011010b (0x9a)
    ; Flags:    10101111b (0xaf) - L(bit 5)=1 (Long Mode)
    dd 0x00000000
    dd 0x00209a00
gdt64_data:
    ; Data 세그먼트
    dd 0x00000000
    dd 0x00009200
gdt64_end:

gdt64_descriptor:
    dw gdt64_end - gdt64_start - 1
    dq gdt64_start  ; 64비트 주소

; 상수 정의
CODE_SEG equ gdt_code - gdt_start
DATA_SEG equ gdt_data - gdt_start

[BITS 32]
init_pm:
    ; 세그먼트 레지스터 DATA_SEG로 초기화
    mov ax, DATA_SEG
    mov ds, ax
    mov ss, ax
    mov es, ax
    mov fs, ax                      ; 32bit CPU에서 추가된 사용처가 정해지지 않은 영역
    mov gs, AX                      ; 32bit CPU에서 추가된 사용처가 정해지지 않은 영역

    call check_cpuid
    call check_long_mode
    call setup_paging
    call enable_paging

    lgdt [gdt64_descriptor]         ; 64비트 GDT 로드
    jmp 0x08:init_lm                ; 64-bit Far Jump

[BITS 64]
init_lm:
    ; 세그먼트 레지스터 초기화
    mov ax, 0
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    ; 스택 재설정
    mov rsp, 0x90000

    ; 커널 점프 (커널이 0x10000에 있음)
    ; 64비트에서는 call 명령어에 32비트 절대주소 사용 불가 -> 레지스터 이용
    mov rax, 0x10000
    call rax
    jmp $

; 모든 메모리는 이 위에서 할당하기!
; 부트로더 크기 512 바이트로 설정 (0으로 초기화)
times 510 - ($ - $$) db 0

; 매직 넘버
dw 0xaa55

; Include 파일
%include "src/boot/cpuid.asm"
%include "src/boot/long_mode_init.asm"