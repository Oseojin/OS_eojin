# 04-1. GDT 엔트리 상세 구조 (GDT Entry Structure)

이 문서는 8바이트(64비트)로 구성된 GDT 엔트리의 비트맵 구조를 아주 상세하게 분해하여 설명합니다. 16진수 코드를 직접 작성할 때 이 문서를 참조표(Reference)로 사용하세요.

## 1. 전체 구조 (Little Endian 주의)

메모리에 저장될 때는 리틀 엔디안 방식이므로 하위 바이트가 먼저 저장됩니다. 하지만 논리적으로 비트를 따질 때는 아래 그림을 참조하세요.

**(MSB: 63번 비트 ... LSB: 0번 비트)**

| 63...56 | 55...52 | 51...48 | 47...40 | 39...32 | 31...16 | 15...0 |
| :---: | :---: | :---: | :---: | :---: | :---: | :---: |
| Base (High) | Flags | Limit (High) | Access Byte | Base (Mid) | Base (Low) | Limit (Low) |

-   **Base Address (32bit)**: 세그먼트 시작 주소.
    -   Base Low (16bit): 비트 16~31
    -   Base Mid (8bit): 비트 32~39
    -   Base High (8bit): 비트 56~63
-   **Segment Limit (20bit)**: 세그먼트 크기.
    -   Limit Low (16bit): 비트 0~15
    -   Limit High (4bit): 비트 48~51

## 2. Access Byte (비트 40~47)

이 1바이트가 세그먼트의 성격을 결정합니다.

| 비트 | 이름 | 설명 | Code (0x9A) | Data (0x92) |
| :--- | :--- | :--- | :--- | :--- |
| **7** | **P** (Present) | 1: 유효한 세그먼트 (메모리에 있음). 0: 무효. | **1** | **1** |
| **6-5** | **DPL** (Privilege) | 00: Ring 0 (커널), 11: Ring 3 (유저). | **00** | **00** |
| **4** | **S** (Type) | 1: 코드/데이터 세그먼트. 0: 시스템 (TSS 등). | **1** | **1** |
| **3** | **E** (Executable) | 1: 실행 가능 (코드). 0: 실행 불가 (데이터). | **1** | **0** |
| **2** | **DC** (Dir/Conf) | 코드: 0=높은 권한만 호출 가능. 데이터: 0=위쪽으로 자람. | **0** | **0** |
| **1** | **RW** (Read/Write) | 코드: 1=읽기 가능. 데이터: 1=쓰기 가능. | **1** | **1** |
| **0** | **A** (Accessed) | CPU가 사용하면 1로 세팅함. 초기값은 0. | **0** | **0** |

-   **Code Segment**: `1 00 1 1 0 1 0` = `0x9A`
-   **Data Segment**: `1 00 1 0 0 1 0` = `0x92`

## 3. Flags (비트 52~55) + Limit High (비트 48~51)

이 1바이트(8비트)는 상위 4비트가 **Flags**, 하위 4비트가 **Limit High**입니다.

| 비트 | 이름 | 설명 | 설정값 |
| :--- | :--- | :--- | :--- |
| **7 (55)** | **G** (Granularity) | 1: Limit 단위가 4KB (페이지). 0: 1Byte. | **1** (4GB 쓰려면 필수) |
| **6 (54)** | **DB** (Size) | 1: 32비트 모드. 0: 16비트 모드. | **1** (32비트니까) |
| **5 (53)** | **L** (Long Mode) | 1: 64비트 모드. 0: 아니오. | **0** (일단 32비트로 감) |
| **4 (52)** | **R** (Reserved) | 예약됨. 무조건 0. | **0** |
| **3-0** | **Limit High** | Limit의 상위 4비트 (16~19). | **1111** (0xF) |

-   **Flags 값**: `1100` = `0xC`
-   **Limit High 값**: `1111` = `0xF`
-   **합쳐서**: `0xCF`

## 4. 최종 어셈블리 코드 매핑 (Flat Model 기준)

Base = `0x00000000`, Limit = `0xFFFFF` (G=1이므로 4GB)

### 코드 세그먼트 (Code Segment)
```nasm
dw 0xFFFF    ; Limit Low (0~15)
dw 0x0000    ; Base Low (0~15)
db 0x00      ; Base Mid (16~23)
db 10011010b ; Access Byte (0x9A)
db 11001111b ; Flags(4bit) + Limit High(4bit) (0xCF)
db 0x00      ; Base High (24~31)
```

### 데이터 세그먼트 (Data Segment)
```nasm
dw 0xFFFF    ; Limit Low
dw 0x0000    ; Base Low
db 0x00      ; Base Mid
db 10010010b ; Access Byte (0x92) - 여기만 다름!
db 11001111b ; Flags + Limit High (0xCF)
db 0x00      ; Base High
```
