[BITS 32]

setup_paging:
    cld                     ; 방향을 정방향으로 설정
    ; 메모리 초기화 (0x1000 ~ 0x4000 영역을 0으로 채우기)
    ; 0x1000~0x3fff까지 페이징 테이블로 사용
    mov edi, 0x1000
    xor eax, eax
    mov ecx, 1024 * 3       ; 3개 테이블 (PML4, PDPT, PD) 크기만큼
    rep stosd               ; dword(4byte) 단위로 0 채우기

    ; PML4 테이블 설정 (0x1000)
    ; 첫 번째 엔트리 -> PDPT(0x2000)
    mov eax, 0x2000
    or eax, 0b11            ; Present(1) | Writable(1)
    mov [0x1000], eax

    ; PDPT 테이블 설정 (0x2000)
    ; 첫 번째 엔트리 -> PD(0x3000)
    mov eax, 0x3000
    or eax, 0b11
    mov [0x2000], eax

    ; PD 테이블 설정 (0x3000) - Huge Page 사용
    ; PT 없이 PD에서 바로 2MB 단위로 매핑

    ; 첫 번째 엔트리: 물리주소 0 ~ 2MB 매핑
    mov eax, 0x0            ; 물리주소 0번지
    or eax, 0b10000011      ; Present(1) | Writable(1) | Huge Page(1)
    mov [0x3000], eax

    ret

enable_paging:
    ; CR3에 PML4 주소 로드
    mov eax, 0x1000
    mov cr3, eax

    ; PAE 활성화 (CR4의 5번째 비트)
    mov eax, cr4
    or eax, 1 << 5
    mov cr4, eax

    ; Long Mode 활성화 (EFER MSR)
    mov ecx, 0xc0000080     ; EFER MSR 번호
    rdmsr                   ; MSR 읽기
    or eax, 1 << 8          ; LM 비트(8번) 켜기
    wrmsr                   ; MSR 쓰기

    ; 페이징 활성화 (CR0의 31번째 비트)
    mov eax, cr0
    or eax, 1 << 31
    mov cr0, eax

    ret