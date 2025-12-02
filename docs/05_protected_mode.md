# 05. 보호 모드 전환 (Switch to Protected Mode)

이 문서는 16비트 리얼 모드에서 **32비트 보호 모드(Protected Mode)**로 전환하는 구체적인 절차를 설명합니다.

## 1. 전환 절차 요약

보호 모드로 진입하기 위해서는 CPU에게 "이제 32비트 방식으로 작동하라"고 알려주어야 합니다. 그 과정은 다음과 같습니다.

1.  **인터럽트 비활성화 (`cli`)**: 전환 도중에 인터럽트가 발생하면 CPU가 꼬일 수 있으므로 모든 인터럽트를 끕니다.
2.  **GDT 로드 (`lgdt`)**: 앞서 만든 GDT 정보를 CPU에 등록합니다.
3.  **CR0 레지스터 설정**: CPU 제어 레지스터(`CR0`)의 첫 번째 비트(PE bit)를 1로 켭니다.
4.  **Far Jump (`jmp`)**: CPU 파이프라인을 비우고(`Flush`), `CS` 레지스터를 새로운 GDT 코드 세그먼트로 업데이트하기 위해 멀리 점프합니다.

## 2. 상세 설명

### 2.1 인터럽트 비활성화 (CLI)
리얼 모드의 BIOS 인터럽트 테이블(IVT)과 보호 모드의 인터럽트 처리 방식(IDT)은 완전히 다릅니다. 보호 모드로 넘어가자마자 리얼 모드 방식의 인터럽트가 들어오면 시스템은 폭발(Crash)합니다. 따라서 IDT를 새로 만들기 전까지는 **절대 인터럽트를 받으면 안 됩니다**.

```nasm
cli     ; Clear Interrupt Flag
```

### 2.2 GDT 로드 (LGDT)
우리가 만든 GDT 구조체의 주소를 CPU의 **GDTR(Global Descriptor Table Register)**에 저장합니다.

```nasm
lgdt [gdt_descriptor]
```

### 2.3 CR0 제어 레지스터
CR0(Control Register 0)는 CPU의 작동 모드를 제어하는 중요한 레지스터입니다.
**0번 비트(PE - Protection Enable)**를 1로 세팅하면 보호 모드가 활성화됩니다.

```nasm
mov eax, cr0
or eax, 0x1     ; 0번 비트 켜기
mov cr0, eax
```

**주의**: 이 명령이 실행되는 순간부터 CPU는 32비트 모드로 작동하기 시작합니다. 하지만 아직 `CS` 레지스터 등은 16비트 시절의 값을 가지고 있어 불안정한 상태입니다.

### 2.4 파이프라인 플러시 (Far Jump)
CPU는 성능 향상을 위해 명령어를 미리 읽어오는(Prefetch) **파이프라인** 구조를 가지고 있습니다.
`mov cr0, eax` 명령 바로 다음 명령어들도 이미 16비트 방식으로 해석되어 파이프라인에 들어와 있을 수 있습니다.
이를 강제로 비우고(Flush), `CS` 레지스터를 우리가 GDT에 정의한 `CODE_SEG`로 강제 업데이트하기 위해 **Far Jump**를 사용합니다.

```nasm
jmp CODE_SEG:init_pm
```

-   `CODE_SEG`: GDT에서 코드 세그먼트 오프셋.
-   `init_pm`: 32비트 코드가 시작될 라벨.

## 3. 32비트 모드 코드 작성 (`[BITS 32]`)

점프한 도착지(`init_pm`)부터는 32비트 명령어를 사용해야 하므로 `[BITS 32]` 지시어를 사용합니다.
여기서 가장 먼저 해야 할 일은 **세그먼트 레지스터 초기화**입니다. `CS`는 점프하면서 바뀌었지만, `DS`, `SS`, `ES` 등은 아직 쓰레기 값을 가지고 있습니다.

```nasm
[BITS 32]
init_pm:
    mov ax, DATA_SEG    ; 데이터 세그먼트 인덱스
    mov ds, ax
    mov ss, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    
    mov ebp, 0x90000    ; 스택 위치 재설정 (더 넓은 공간으로)
    mov esp, ebp
    
    call BEGIN_PM       ; 32비트 메인 함수 호출
```
