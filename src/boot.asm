; -----------------------------------------------------------------------------
;  OS_eojin Bootloader for x86 (boot.asm)
;  기능:
;   1. 16비트 리얼 모드에서 시작 (BIOS에 의해 0x7c00 로드됨)
;   2. 디스크에서 커널(C 코드)을 읽어서 메모리 0x1000에 적재
;   3. GDT(Global Descriptor Table) 설정
;   4. 32비트 보호 모드(Protected Mode)로 전환
;   5. 커널 실행 (0x1000으로 점프)
; -----------------------------------------------------------------------------

[org 0x7c00]            ; BIOS가 부트로더를 로드하는 메모리 주소
[bits 16]               ; 16비트 모드로 시작

KERNEL_OFFSET equ 0x1000 ; 커널을 로드할 메모리 주소

start:
    ; 1. 세그먼트 레지스터 초기화
    xor ax, ax          ; AX = 0
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x9000      ; 스택 포인터를 안전한 곳으로 이동 (0x9000)

    mov [BOOT_DRIVE], dl ; BIOS가 부팅 드라이브 번호를 DL에 저장해줌. 이를 변수에 백업.

    ; 2. 화면에 메시지 출력 (Real Mode)
    mov bx, MSG_REAL_MODE
    call print_string_rm

    ; 3. 커널 로드 (Disk Read)
    call load_kernel

    ; 4. 32비트 보호 모드 전환 준비
    call switch_to_pm   ; 돌아오지 않는 함수 (여기서 32비트 세계로 감)

    jmp $               ; 만약 돌아온다면 무한 루프 (에러)

; -----------------------------------------------------------------------------
;  16-bit Real Mode 함수들
; -----------------------------------------------------------------------------

print_string_rm:        ; BX에 문자열 주소를 넣고 호출
    pusha
    mov ah, 0x0e        ; BIOS 텔레타입 출력 함수 번호
.loop:
    mov al, [bx]        ; 문자 하나 읽기
    cmp al, 0
    je .done            ; NULL 문자(0)면 종료
    int 0x10            ; 화면 출력 인터럽트 발생
    inc bx
    jmp .loop
.done:
    popa
    ret

load_kernel:
    mov bx, MSG_LOAD_KERNEL
    call print_string_rm

    mov bx, KERNEL_OFFSET ; 데이터를 저장할 메모리 주소 (ES:BX = 0x0000:0x1000)
    mov dh, 15            ; 읽을 섹터 수 (커널 크기에 따라 조절 필요, 여기선 15섹터)
    mov dl, [BOOT_DRIVE]  ; 부팅 드라이브 번호

    mov ah, 0x02          ; BIOS 읽기 섹터 함수
    mov al, dh            ; 읽을 섹터 개수
    mov ch, 0x00          ; 실린더 0
    mov dh, 0x00          ; 헤드 0
    mov cl, 0x02          ; 섹터 2부터 읽기 (섹터 1은 부트로더이므로)

    int 0x13              ; 디스크 읽기 인터럽트!

    jc disk_error         ; Carry Flag가 1이면 에러 발생
    ret

disk_error:
    mov bx, MSG_DISK_ERROR
    call print_string_rm
    jmp $

; -----------------------------------------------------------------------------
;  GDT (Global Descriptor Table) 정의
;  32비트 모드에서는 세그먼트 레지스터가 GDT의 인덱스를 가리켜야 함.
; -----------------------------------------------------------------------------
gdt_start:

gdt_null:                ; 필수: 첫 번째 항목은 0 (Null Descriptor)
    dd 0x0
    dd 0x0

gdt_code:                ; 코드 세그먼트 디스크립터
    ; base=0x0, limit=0xfffff
    ; 1st flags: (present)1 (privilege)00 (descriptor type)1 -> 1001b
    ; type flags: (code)1 (conforming)0 (readable)1 (accessed)0 -> 1010b
    ; 2nd flags: (granularity)1 (32-bit default)1 (64-bit seg)0 (AVL)0 -> 1100b
    dw 0xffff           ; Limit (bits 0-15)
    dw 0x0000           ; Base (bits 0-15)
    db 0x00             ; Base (bits 16-23)
    db 10011010b        ; 1st flags, type flags
    db 11001111b        ; 2nd flags, Limit (bits 16-19)
    db 0x00             ; Base (bits 24-31)

gdt_data:                ; 데이터 세그먼트 디스크립터
    ; Code와 거의 같으나 type flags만 다름
    ; type flags: (code)0 (expand-down)0 (writable)1 (accessed)0 -> 0010b
    dw 0xffff
    dw 0x0000
    db 0x00
    db 10010010b        ; Type flags 변경됨
    db 11001111b
    db 0x00

gdt_end:

gdt_descriptor:
    dw gdt_end - gdt_start - 1 ; 크기 (Size)
    dd gdt_start               ; 시작 주소 (Start Address)

; 상수로 사용할 GDT 세그먼트 오프셋 정의
CODE_SEG equ gdt_code - gdt_start
DATA_SEG equ gdt_data - gdt_start

; -----------------------------------------------------------------------------
;  32비트 전환 루틴
; -----------------------------------------------------------------------------
switch_to_pm:
    cli                     ; 1. 인터럽트 비활성화 (매우 중요)
    lgdt [gdt_descriptor]   ; 2. GDT 로드

    mov eax, cr0
    or eax, 0x1             ; 3. CR0 레지스터의 첫 번째 비트를 1로 설정
    mov cr0, eax            ; --> 실제 보호 모드 전환 시점!

    jmp CODE_SEG:init_pm    ; 4. Far Jump로 파이프라인 플러시 (32비트 코드로 점프)

[bits 32]                   ; 이제부터 32비트 명령어로 어셈블
init_pm:
    ; 5. 세그먼트 레지스터를 데이터 세그먼트로 갱신
    mov ax, DATA_SEG
    mov ds, ax
    mov ss, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    mov ebp, 0x90000        ; 스택 포인터 갱신 (32비트 공간의 안전한 곳)
    mov esp, ebp

    call BEGIN_PM           ; 32비트 메인 루틴 호출

BEGIN_PM:
    ; 여기서부터는 32비트 보호 모드입니다!
    ; 비디오 메모리에 직접 써서 "P" (Protected Mode)를 출력해봅시다.
    mov ebx, 0xB8000        ; VGA 텍스트 모드 메모리 주소
    mov byte [ebx], 'P'     ; 화면 좌상단에 'P' 출력
    mov byte [ebx+1], 0x0F  ; 흰색 글자, 검은 배경

    ; 커널로 제어권 넘기기
    call KERNEL_OFFSET      ; 0x1000에 로드된 C 커널의 시작점으로 점프

    jmp $                   ; 무한 루프

; -----------------------------------------------------------------------------
;  데이터 영역
; -----------------------------------------------------------------------------
BOOT_DRIVE      db 0
MSG_REAL_MODE   db "Started in 16-bit Real Mode", 0
MSG_LOAD_KERNEL db "Loading kernel into memory...", 0
MSG_DISK_ERROR  db "Disk read error!", 0

; -----------------------------------------------------------------------------
;  부트 섹터 패딩
; -----------------------------------------------------------------------------
times 510-($-$$) db 0   ; 510바이트까지 0으로 채움
dw 0xaa55               ; 매직 넘버 (부트 섹터임을 표시)