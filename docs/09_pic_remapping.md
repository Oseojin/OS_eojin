# 09. PIC 리매핑 (Programmable Interrupt Controller Remapping)

이 문서는 하드웨어 인터럽트를 처리하기 위해 **8259A PIC** 칩을 제어하고 인터럽트 벡터 번호를 재설정(Remapping)하는 과정을 설명합니다.

## 1. PIC란 무엇인가?

**PIC (Programmable Interrupt Controller)**는 키보드, 타이머, 디스크 등 외부 장치에서 보내는 신호(IRQ)를 받아서, 우선순위를 판별한 뒤 CPU에게 "인터럽트가 왔다"고 알려주는 칩입니다.

전통적인 x86 시스템은 두 개의 PIC 칩을 사용합니다.
-   **Master PIC**: CPU와 직접 연결됨. (IRQ 0~7 담당)
-   **Slave PIC**: Master PIC와 연결됨. (IRQ 8~15 담당)

## 2. 왜 리매핑(Remapping)을 해야 하는가?

BIOS는 부팅 시 PIC를 초기화하면서 IRQ 0~7번을 인터럽트 벡터 **0x08~0x0F**에 할당합니다.
그런데 인텔 CPU의 보호 모드에서는 **0x00~0x1F**가 **CPU 예외(Exception)** (0으로 나누기, Page Fault 등) 용도로 예약되어 있습니다.

즉, **"IRQ 0 (타이머)"**가 발생했는데 CPU는 이를 **"Double Fault (예외 8번)"**로 착각하게 됩니다. 이 충돌을 피하기 위해 PIC가 보내는 인터럽트 번호를 **0x20 (32번)** 이후로 밀어버려야 합니다.

### 목표 매핑
-   **Master PIC (IRQ 0~7)** -> **0x20 ~ 0x27** (32 ~ 39)
-   **Slave PIC (IRQ 8~15)** -> **0x28 ~ 0x2F** (40 ~ 47)

## 3. PIC 제어 방법 (I/O Port)

PIC는 I/O 포트를 통해 명령을 주고받습니다.

| 장치 | Command Port | Data Port |
| :--- | :--- | :--- |
| **Master PIC** | `0x20` | `0x21` |
| **Slave PIC** | `0xA0` | `0xA1` |

## 4. 초기화 시퀀스 (ICW)

PIC를 초기화하려면 **ICW (Initialization Command Word)**라는 4개의 명령어를 순서대로 보내야 합니다.

1.  **ICW1 (초기화 시작)**: `0x11`을 Command Port에 보냅니다. (초기화 모드 진입, ICW4 필요함 알림)
2.  **ICW2 (벡터 오프셋 설정)**: **가장 중요!** Data Port에 "어디로 매핑할지" 시작 번호를 보냅니다. (Master: `0x20`, Slave: `0x28`)
3.  **ICW3 (마스터-슬레이브 연결)**:
    -   Master: `0x04` (2번 핀에 Slave가 있다)
    -   Slave: `0x02` (나는 2번 핀에 연결되었다)
4.  **ICW4 (환경 설정)**: `0x01` (8086 모드 사용).

## 5. 구현 전략 (C언어 + Assembly)

C언어에서는 직접 I/O 포트를 제어할 수 없으므로, 어셈블리 래퍼 함수(`inb`, `outb`)를 만들어서 사용해야 합니다.

```c
// C 코드 예시
outb(0x20, 0x11); // ICW1
outb(0x21, 0x20); // ICW2 (Master -> 0x20)
// ...
```

```nasm
; Assembly Wrapper
global outb
outb:
    ; 스택에서 인자(port, data) 꺼내서 out 명령어 실행
    mov al, [esp + 8]
    mov dx, [esp + 4]
    out dx, al
    ret
```
