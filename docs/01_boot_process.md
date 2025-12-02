# 01. 부팅 프로세스와 MBR (Boot Process & MBR)

이 문서는 **OS_eojin** 프로젝트의 첫 번째 단계인 시스템 부팅과 MBR(Master Boot Record)의 개념을 설명합니다.

## 1. 부팅 시퀀스 (Boot Sequence)

컴퓨터 전원을 켜면 운영체제가 실행되기 전까지 다음과 같은 과정을 거칩니다.

1.  **Power On**: 전원이 공급됩니다.
2.  **POST (Power-On Self-Test)**: 하드웨어(RAM, 키보드, 디스크 등)가 정상인지 검사합니다.
3.  **BIOS (Basic Input/Output System) 실행**: 메인보드의 ROM에 저장된 펌웨어가 실행됩니다.
4.  **Boot Device 탐색**: BIOS 설정에 따라 부팅 가능한 장치(HDD, USB, CD-ROM 등)를 순서대로 찾습니다.
5.  **Boot Sector 로드**: 부팅 장치의 첫 번째 섹터(Sector 0, 512바이트)를 읽어 메모리의 물리 주소 `0x7C00`에 로드합니다.
6.  **제어권 전달**: BIOS는 `0x7C00`으로 점프(Jump)하여 코드를 실행합니다.

이때 실행되는 512바이트 크기의 코드가 바로 **부트로더(Bootloader)**의 1단계입니다.

## 2. MBR (Master Boot Record)

저장 장치의 가장 첫 번째 섹터(LBA 0)를 MBR이라고 합니다.

### 구조 (512 Bytes)
| 구성 요소 | 크기 | 설명 |
| :--- | :--- | :--- |
| **Boot Code** | 446 bytes | 실제 실행될 부트로더 코드 (기계어) |
| **Partition Table** | 64 bytes | 파티션 정보 (16 bytes * 4개) |
| **Boot Signature** | 2 bytes | 이 섹터가 부팅 가능함을 알리는 매직 넘버 (`0x55`, `0xAA`) |

**중요**: 마지막 2바이트가 반드시 `0x55`, `0xAA`여야 BIOS가 이를 유효한 부트 섹터로 인식하고 실행합니다.

## 3. 16-bit Real Mode (리얼 모드)

부트로더가 처음 실행될 때 CPU는 **16비트 리얼 모드** 상태입니다.

-   **특징**:
    -   BIOS 인터럽트(Service Routine)를 사용하여 화면 출력, 디스크 읽기 등을 쉽게 수행할 수 있습니다.
    -   주소 지정 방식: `Segment * 16 + Offset` (최대 1MB 메모리 접근 가능).
    -   메모리 보호 기능이 없어, 잘못된 코드가 시스템 전체를 멈추게 할 수 있습니다.

### 주요 레지스터 (Registers)
-   `AX`, `BX`, `CX`, `DX`: 범용 레지스터 (16비트).
-   `CS` (Code Segment), `DS` (Data Segment): 세그먼트 레지스터.
-   `IP` (Instruction Pointer): 현재 실행 중인 코드의 위치.
-   `SP` (Stack Pointer): 스택의 최상위 주소.

## 4. 목표 (Goal)

Phase 1의 첫 번째 목표는 다음 기능을 수행하는 최소한의 부트로더를 어셈블리어로 작성하는 것입니다.

1.  메모리 `0x7C00`에서 실행 시작.
2.  화면에 "Hello, OS_eojin!" 문자열 출력 (BIOS 인터럽트 `INT 0x10` 사용).
3.  무한 루프를 돌아 제어권 유지.
4.  510바이트까지 0으로 채우고, 마지막에 `0xAA55` 서명 추가.