# ATA PIO 모드 드라이버 (Hard Disk Driver)

## 1. 개요
ATA(Advanced Technology Attachment)는 하드 디스크와 통신하는 표준 인터페이스입니다. 초기 x86 시스템에서는 **PIO(Programmed I/O)** 모드를 사용하여 CPU가 직접 데이터 전송을 제어했습니다. (현대는 DMA를 쓰지만, OS 개발 초기엔 PIO가 구현하기 쉽습니다.)

## 2. I/O 포트 (Primary Bus)
ATA 컨트롤러는 I/O 포트를 통해 제어합니다. Primary Bus의 기본 포트는 `0x1F0` ~ `0x1F7`입니다.

| 포트 | 읽기 (Read) | 쓰기 (Write) |
| :--- | :--- | :--- |
| `0x1F0` | **Data Register** (16-bit) | **Data Register** (16-bit) |
| `0x1F1` | Error Register | Features Register |
| `0x1F2` | Sector Count | Sector Count |
| `0x1F3` | LBA Low (0-7) | LBA Low (0-7) |
| `0x1F4` | LBA Mid (8-15) | LBA Mid (8-15) |
| `0x1F5` | LBA High (16-23) | LBA High (16-23) |
| `0x1F6` | Drive / Head | Drive / Head |
| `0x1F7` | **Status Register** | **Command Register** |

- **Data Register (0x1F0):** 실제 데이터를 2바이트(Word) 단위로 읽고 씁니다.
- **Command Register (0x1F7):** 디스크에 명령(읽기, 쓰기 등)을 내립니다.
- **Status Register (0x1F7):** 디스크의 현재 상태(Busy, Ready, Error 등)를 확인합니다.

## 3. LBA (Logical Block Addressing) 28-bit
예전에는 CHS(Cylinder-Head-Sector) 방식을 썼지만, 지금은 섹터 번호를 0부터 순차적으로 매기는 LBA 방식을 씁니다.
28비트 모드에서는 최대 128GB까지 접근 가능합니다.

### 3.1. LBA 전송 순서
1. **Drive Select (0x1F6):** 마스터(0xA0) / 슬레이브(0xB0) 선택 및 LBA 상위 4비트(24-27) 설정.
2. **Sector Count (0x1F2):** 읽거나 쓸 섹터 수 설정 (보통 1).
3. **LBA Address (0x1F3~1F5):** 하위 24비트 주소 설정.
4. **Command (0x1F7):** 
   - 읽기: `0x20` (Read Sectors with Retry)
   - 쓰기: `0x30` (Write Sectors with Retry)

## 4. 읽기/쓰기 절차

### 읽기 (Read)
1. `BSY` (Busy) 비트가 꺼질 때까지 대기.
2. 레지스터(Drive, LBA, Count) 설정.
3. `0x20` 명령 전송.
4. `DRQ` (Data Request) 비트가 켜질 때까지 대기.
5. `0x1F0` 포트에서 256번(256 words = 512 bytes) 데이터를 읽음 (`insw`).

### 쓰기 (Write)
1. `BSY` 비트 대기.
2. 레지스터 설정.
3. `0x30` 명령 전송.
4. `DRQ` 비트 대기.
5. `0x1F0` 포트에 256번 데이터 쓰기 (`outsw`).
6. (선택) `Cache Flush` 명령(`0xE7`) 전송 후 대기.

## 5. 주의사항
- **Floating Bus:** 드라이브가 없거나 연결이 안 된 경우 `Status`가 `0xFF`로 읽혀 무한 루프에 빠질 수 있습니다.
- **400ns Delay:** 명령 전송 후 상태 레지스터를 읽기 전에 약간의 딜레이가 필요합니다. (보통 포트 `0x3F6` 등을 4번 읽어서 해결)
