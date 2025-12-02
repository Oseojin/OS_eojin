# 04. GDT (Global Descriptor Table)

이 문서는 리얼 모드에서 32비트 보호 모드(Protected Mode)로 전환하기 위해 반드시 필요한 **GDT (Global Descriptor Table)**의 개념과 구조를 설명합니다.

## 1. GDT란 무엇인가?

**GDT (Global Descriptor Table)**는 메모리 영역(세그먼트)들의 정보를 담고 있는 **표(Table)**입니다.

-   **리얼 모드**: 주소 = `Segment * 16 + Offset` (물리 주소 직접 접근)
-   **보호 모드**: 주소 = `Segment Selector(인덱스)` -> **GDT** -> `Base Address + Offset`

보호 모드에서는 세그먼트 레지스터(CS, DS 등)에 더 이상 실제 주소값(`0x1000` 등)을 넣지 않습니다. 대신 **GDT 테이블의 몇 번째 항목을 사용할지**를 나타내는 **인덱스(Selector)**를 넣습니다. CPU는 GDT를 참조하여 해당 세그먼트의 실제 시작 주소, 크기(Limit), 권한(Access) 등을 확인합니다.

## 2. 왜 필요한가? (메모리 보호)

GDT를 사용하면 각 메모리 영역에 **권한**과 **속성**을 부여할 수 있습니다.

-   **커널 영역 보호**: 유저 프로그램이 커널 코드나 데이터에 접근하지 못하게 막을 수 있습니다 (Privilege Level).
-   **실행/읽기/쓰기 제어**: 코드 영역은 "읽기/실행"만 가능하고 "쓰기"는 불가능하게 설정하여, 코드가 변조되는 것을 막을 수 있습니다.

## 3. GDT 엔트리 구조 (8 Bytes)

GDT의 각 항목(Entry)은 8바이트(64비트) 크기이며, 매우 복잡하게 꼬여 있습니다. (하위 호환성과 역사적인 이유 때문입니다.)

| 비트 위치 | 내용 | 설명 |
| :--- | :--- | :--- |
| **Base (32bit)** | 시작 주소 | 세그먼트가 메모리 어디에서 시작하는지 (보통 0으로 설정하여 Flat Model 사용). 3조각으로 쪼개져 있음. |
| **Limit (20bit)** | 크기 | 세그먼트의 크기. 2조각으로 쪼개져 있음. |
| **Access Byte** | 접근 권한 | 읽기/쓰기, 실행 가능 여부, 특권 레벨(Ring 0~3), 시스템 세그먼트 여부 등. |
| **Flags** | 플래그 | 32비트/16비트 모드 설정, Granularity(단위) 설정 등. |

### Access Byte 상세 (1 Byte)
`Pr | Privl (2) | S | Ex | DC | RW | Ac`

-   `Pr` (Present): 세그먼트가 유효한지 (1=Yes).
-   `Privl` (Privilege): 0=커널(Ring 0), 3=유저(Ring 3).
-   `S` (Descriptor Type): 1=코드/데이터, 0=시스템(TSS 등).
-   `Ex` (Executable): 1=코드(실행 가능), 0=데이터.
-   `RW` (Readable/Writable): 코드면 읽기 가능 여부, 데이터면 쓰기 가능 여부.

## 4. Flat Memory Model

현대 운영체제(Windows, Linux 등)와 우리는 **Flat Memory Model**을 사용합니다.

-   **Code Segment**: Base=0, Limit=4GB (전체 메모리)
-   **Data Segment**: Base=0, Limit=4GB (전체 메모리)

즉, 세그먼트로 영역을 나누지 않고 **모든 세그먼트가 전체 메모리(0 ~ 4GB)를 가리키도록 설정**합니다. 이렇게 하면 페이징(Paging) 기법을 사용할 때 관리가 훨씬 편해집니다. GDT는 형식적으로 존재하며 권한 체크 용도로만 쓰입니다.

## 5. GDT Descriptor (GDTR)

CPU에게 "GDT가 여기 있다"라고 알려주기 위해 사용하는 특별한 구조체입니다. `lgdt` 명령어로 로드합니다.

-   **Size (16bit)**: GDT 전체 크기 - 1 (바이트 단위).
-   **Offset (32bit)**: GDT 테이블의 시작 주소.

## 6. 목표 설정

우리는 총 3개의 엔트리를 만들 것입니다.
1.  **Null Descriptor**: 필수 (0번 엔트리는 무조건 비워둬야 함).
2.  **Kernel Code**: Base=0, Limit=4GB, 실행/읽기 가능, Ring 0.
3.  **Kernel Data**: Base=0, Limit=4GB, 읽기/쓰기 가능, Ring 0.
