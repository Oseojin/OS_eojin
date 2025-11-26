; src/interrupts.asm
[bits 32]

; drivers.c에 정의된 C 함수
[extern keyboard_handler_main]

global keyboard_handler

keyboard_handler:
    pusha                   ; 모든 범용 레지스터 저장
    call keyboard_handler_main  ; C 함수 실행
    popa                    ; 레지스터 복구
    iret                    ; 인터럽트 종료 (Interrupt Return)