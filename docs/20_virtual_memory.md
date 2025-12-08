# 20. 가상 메모리 관리 (Virtual Memory Management)

이 문서는 OS_eojin에서 가상 메모리를 동적으로 관리하기 위한 VMM(Virtual Memory Manager)의 설계와 구현 방법을 다룹니다. 현재의 정적 Identity Mapping을 넘어, 프로세스별 독립적인 주소 공간을 제공하기 위한 기반이 됩니다.

## 1. x86-64 페이징 구조 (4-Level Paging)

x86-64 아키텍처(Long Mode)는 4단계 페이징을 사용하여 48비트 가상 주소를 52비트(최대) 물리 주소로 변환합니다.

### 가상 주소 구조 (48-bit)
| Sign Ext | Index 4 | Index 3 | Index 2 | Index 1 | Offset |
| :---: | :---: | :---: | :---: | :---: | :---: |
| 16 bits | 9 bits | 9 bits | 9 bits | 9 bits | 12 bits |
| - | **PML4** | **PDPT** | **PD** | **PT** | **Physical** |

*   **PML4 (Page Map Level 4):** 최상위 테이블. CR3 레지스터가 가리킴.
*   **PDPT (Page Directory Pointer Table):** PML4 엔트리가 가리킴.
*   **PD (Page Directory):** PDPT 엔트리가 가리킴. (2MB Huge Page 매핑 가능).
*   **PT (Page Table):** PD 엔트리가 가리킴. (4KB Page 매핑).
*   **Offset:** 페이지 내 오프셋 (4KB = 12bits).

### 엔트리 구조 (64-bit)
모든 테이블 엔트리는 8바이트(64비트)이며, 하위 비트에 속성 플래그가 있습니다.

*   **Bit 0 (P):** Present. 1이면 메모리에 존재.
*   **Bit 1 (R/W):** Read/Write. 1이면 쓰기 가능.
*   **Bit 2 (U/S):** User/Supervisor. 1이면 유저 모드(Ring 3) 접근 가능.
*   **Bit 12-51:** 다음 테이블의 물리 주소 (4KB 정렬).

---

## 2. VMM 설계

### 기능 요구사항
1.  **페이지 매핑 (Map):** `vmm_map(pml4, vaddr, paddr, flags)`
    *   가상 주소 `vaddr`을 물리 주소 `paddr`에 매핑합니다.
    *   필요한 중간 테이블(PDPT, PD, PT)이 없으면 `PMM`을 통해 할당하고 연결합니다.
2.  **페이지 해제 (Unmap):** `vmm_unmap(pml4, vaddr)`
    *   매핑을 해제하고 `P` 비트를 끕니다.
    *   물리 메모리 해제 여부는 옵션입니다.
3.  **주소 변환 (Virt to Phys):** `vmm_get_phys(pml4, vaddr)`
    *   디버깅 및 DMA 설정 시 필요합니다.

### 메모리 레이아웃 전략
*   **커널 공간 (Higher Half):** `0xFFFF800000000000` 이상. (추후 적용).
*   **유저 공간 (Lower Half):** `0x0000000000000000` ~ `0x00007FFFFFFFFFFF`.
*   **Identity Mapping:** 커널 초기화 및 하드웨어 제어(MMIO)를 위해 일부 영역(예: 첫 1GB)은 1:1 매핑 유지.

---

## 3. 구현 계획

1.  **`vmm.h`:** 페이징 플래그 상수 및 함수 선언.
2.  **`vmm.c`:**
    *   `vmm_init()`: 커널용 PML4 생성 및 초기 매핑.
    *   `map_page()`: 4단계 테이블 워킹 및 엔트리 설정.
    *   `get_next_table()`: 테이블이 없으면 생성하는 헬퍼 함수.

### 주의사항
*   페이지 테이블 자체도 물리 메모리에 있어야 하므로 `pmm_alloc_block`을 사용합니다.
*   새로 할당된 페이지 테이블은 반드시 **0으로 초기화**해야 합니다. (Garbage 값 방지).
*   TLB(Translation Lookaside Buffer) 캐시를 갱신하기 위해 `invlpg` 명령어나 `CR3` 리로드가 필요합니다.

---

## 4. 한국어 요약

VMM은 가상 주소를 물리 주소로 연결해주는 관리자입니다.
4단계 페이징 테이블(PML4 -> PDPT -> PD -> PT)을 따라가며 주소를 변환합니다.
새로운 가상 주소를 사용하려면, 중간 단계의 테이블이 없을 경우 새로 만들어서 연결해줘야 합니다.
이 기능을 통해 각 프로세스마다 서로 다른 메모리 맵을 가질 수 있게 됩니다.
