# 08. 인터럽트와 IDT (Interrupts & IDT)

이 문서는 **Phase 3: 하드웨어 제어**의 시작점인 **인터럽트(Interrupt)**의 개념과 32비트 보호 모드에서의 처리 방식인 **IDT(Interrupt Descriptor Table)**를 설명합니다.

## 1. 인터럽트란?

CPU가 프로그램을 실행하는 도중에, **"긴급하게 처리해야 할 사건"**이 발생했을 때 하던 일을 멈추고 정해진 처리 루틴(ISR)으로 점프하는 메커니즘입니다.

### 종류
1.  **예외 (Exceptions)**: CPU 내부 오류. (예: 0으로 나누기, 잘못된 메모리 접근, Page Fault)
2.  **하드웨어 인터럽트 (Hardware Interrupts)**: 외부 장치 신호. (예: 키보드 누름, 타이머 울림, 하드디스크 읽기 완료)
3.  **소프트웨어 인터럽트 (Software Interrupts)**: 프로그램이 의도적으로 호출. (예: 시스템 콜 `INT 0x80`)

## 2. IDT (Interrupt Descriptor Table)

리얼 모드에서는 **IVT(Interrupt Vector Table)**가 메모리 `0x0000`에 고정되어 있었지만, 보호 모드에서는 **IDT**를 사용합니다.
IDT는 GDT와 비슷하게 **디스크립터(Descriptor)**들의 배열입니다. 총 256개의 인터럽트(0~255)를 정의할 수 있습니다.

### 구조 (8 Bytes)
IDT의 각 엔트리는 "인터럽트가 발생하면 어디(ISR 주소)로 점프할지"를 정의합니다.

| 비트 위치 | 내용 | 설명 |
| :--- | :--- | :--- |
| **Offset (Low)** | 16 bits | 핸들러 함수의 하위 16비트 주소. |
| **Selector** | 16 bits | 커널 코드 세그먼트 셀렉터 (GDT의 오프셋, 보통 `0x08`). |
| **Zero** | 8 bits | 항상 0. |
| **Type Attributes** | 8 bits | 게이트 타입 및 권한 설정. (아래 상세) |
| **Offset (High)** | 16 bits | 핸들러 함수의 상위 16비트 주소. |

### Type Attributes (1 Byte)
`P | DPL(2) | 0 | GateType(4)`

-   **P (Present)**: 1=유효함.
-   **DPL (Privilege)**: 0=커널만 호출 가능, 3=유저도 호출 가능 (`INT` 명령).
-   **Gate Type**: `0xE` = 32-bit Interrupt Gate (인터럽트 발생 시 자동으로 다른 인터럽트를 막음 `CLI`).

## 3. ISR (Interrupt Service Routine)

실제 인터럽트를 처리하는 함수입니다.
**중요한 점**: CPU는 인터럽트 발생 시 현재 상태(EIP, CS, EFLAGS 등)를 스택에 자동으로 저장합니다. 핸들러가 끝날 때는 반드시 `IRET` (Interrupt Return) 명령어로 복귀해야 이 스택이 정리되고 원래 코드로 돌아갑니다.

C언어 함수는 `RET`으로 끝나므로, ISR은 반드시 **어셈블리어로 작성된 래퍼(Wrapper)**가 먼저 받아줘야 합니다.

```nasm
; ISR 래퍼 예시
global isr_handler
isr_handler:
    pusha           ; 모든 범용 레지스터 저장
    call c_handler  ; C 함수 호출
    popa            ; 레지스터 복구
    iret            ; 인터럽트 종료 및 복귀
```

## 4. 구현 로드맵

1.  **IDT 구조체 정의 (C언어)**: IDT 엔트리와 IDTR 구조체를 만듭니다.
2.  **ISR 작성 (Assembly)**: 각 인터럽트 번호별로 점프할 어셈블리 루틴을 만듭니다.
3.  **로드 (LIDT)**: `lidt` 명령어로 CPU에게 IDT 위치를 알립니다.
4.  **테스트**: 강제로 인터럽트(`int $n`)를 발생시켜 핸들러가 실행되는지 봅니다.
