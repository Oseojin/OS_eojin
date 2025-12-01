[BITS 32]
[EXTERN main]   ; C 코드에 있는 main 함수 참조 선언

global _start   ; 링커가 찾을 수 있도록 진입점 심볼 노출

_start:
    call main   ; C 함수의 main() 호출
    jmp $       ; 리턴 (무한 루프)