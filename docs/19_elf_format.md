# 19. ELF 파일 포맷 (Executable and Linkable Format)

이 문서는 OS_eojin에서 표준 실행 파일 포맷인 ELF64를 지원하기 위한 기술적 배경과 구현 명세를 다룹니다. 현재의 Flat Binary 실행 방식을 넘어, C 언어로 작성되고 컴파일된 표준 프로그램을 실행하기 위해 필수적입니다.

## 1. ELF 개요

ELF(Executable and Linkable Format)는 유닉스 계열 시스템(Linux, BSD 등)에서 실행 파일, 목적 파일(.o), 공유 라이브러리(.so), 코어 덤프 등에 사용되는 표준 바이너리 파일 형식입니다.

OS_eojin은 64비트 운영체제이므로 **ELF64** 포맷을 사용합니다.

### 파일 구조
ELF 파일은 크게 다음 세 부분으로 구성됩니다.

1.  **ELF Header:** 파일의 맨 앞에 위치하며, 파일의 형식(32/64bit), 엔디안, 진입점(Entry Point), 헤더 테이블의 위치 등을 담고 있습니다.
2.  **Program Header Table:** 실행 파일을 메모리에 로드할 때 필요한 세그먼트(Segment) 정보를 담고 있습니다. (커널이 로더 역할을 할 때 주로 참조).
3.  **Section Header Table:** 링킹 및 디버깅에 필요한 섹션(Section) 정보를 담고 있습니다. (실행 시에는 필수 아님).

---

## 2. 자료형 (Data Types) - ELF64

C 언어 구조체 정의를 위해 ELF64 표준 자료형을 사용합니다.

| 타입 | 크기 | 설명 |
| :--- | :--- | :--- |
| `Elf64_Addr` | 8 bytes | 가상 주소 |
| `Elf64_Off` | 8 bytes | 파일 오프셋 |
| `Elf64_Half` | 2 bytes | Medium integer |
| `Elf64_Word` | 4 bytes | Integer |
| `Elf64_Sword` | 4 bytes | Signed Integer |
| `Elf64_Xword` | 8 bytes | Long Integer |
| `Elf64_Sxword` | 8 bytes | Signed Long Integer |

---

## 3. ELF 헤더 (ELF Header)

파일의 가장 첫 부분(Offset 0)에 위치합니다.

```c
typedef struct {
    unsigned char e_ident[16];  // 매직 넘버 및 파일 클래스
    Elf64_Half    e_type;       // 파일 타입 (실행 파일, 목적 파일 등)
    Elf64_Half    e_machine;    // 아키텍처 (EM_X86_64 = 62)
    Elf64_Word    e_version;    // 버전
    Elf64_Addr    e_entry;      // 프로그램 진입점 (Entry Point) 주소
    Elf64_Off     e_phoff;      // Program Header Table의 파일 오프셋
    Elf64_Off     e_shoff;      // Section Header Table의 파일 오프셋
    Elf64_Word    e_flags;      // 프로세서별 플래그
    Elf64_Half    e_ehsize;     // ELF 헤더 크기 (64 bytes)
    Elf64_Half    e_phentsize;  // Program Header 항목 하나의 크기
    Elf64_Half    e_phnum;      // Program Header 항목 개수
    Elf64_Half    e_shentsize;  // Section Header 항목 하나의 크기
    Elf64_Half    e_shnum;      // Section Header 항목 개수
    Elf64_Half    e_shstrndx;   // 섹션 이름 문자열 테이블의 인덱스
} Elf64_Ehdr;
```

### 주요 필드
*   `e_ident`: `0x7F`, 'E', 'L', 'F' 로 시작해야 합니다.
*   `e_entry`: 프로그램이 시작될 가상 메모리 주소입니다. `exec` 시스템 콜은 로딩 후 이 주소로 점프해야 합니다.
*   `e_phoff`: 프로그램 헤더 테이블이 파일의 어디에 있는지 알려줍니다.

---

## 4. 프로그램 헤더 (Program Header)

실행 파일이 메모리에 어떻게 로드되어야 하는지를 정의합니다. `e_phoff` 위치에서 시작하며, `e_phnum` 개수만큼 존재합니다.

```c
typedef struct {
    Elf64_Word  p_type;     // 세그먼트 타입 (LOAD, DYNAMIC 등)
    Elf64_Word  p_flags;    // 플래그 (Read, Write, Execute)
    Elf64_Off   p_offset;   // 파일 내에서의 오프셋
    Elf64_Addr  p_vaddr;    // 로드될 가상 메모리 주소
    Elf64_Addr  p_paddr;    // (무시됨) 물리 주소
    Elf64_Xword p_filesz;   // 파일에서의 세그먼트 크기
    Elf64_Xword p_memsz;    // 메모리에서의 세그먼트 크기 (BSS 영역 포함)
    Elf64_Xword p_align;    // 정렬 요구사항
} Elf64_Phdr;
```

### 로딩 알고리즘
커널(로더)은 프로그램 헤더 테이블을 순회하며 `p_type`이 `PT_LOAD` (1) 인 세그먼트를 찾습니다.

1.  파일의 `p_offset` 위치에서 `p_filesz`만큼 데이터를 읽습니다.
2.  메모리의 `p_vaddr` 주소에 데이터를 복사합니다.
3.  만약 `p_memsz`가 `p_filesz`보다 크다면, 남은 공간(`p_memsz - p_filesz`)은 0으로 채웁니다. (BSS 영역 초기화).
4.  `p_flags`에 따라 메모리 권한(R/W/X)을 설정할 수 있습니다. (현재 OS_eojin은 모든 페이지가 R/W/X이므로 생략 가능).

---

## 5. 구현 계획

1.  **헤더 정의 (`includes/elf.h`):** 위 구조체들을 정의합니다.
2.  **ELF 검증 (`elf_check_file`):** 파일 버퍼를 읽어 매직 넘버와 아키텍처(x86-64)를 확인합니다.
3.  **세그먼트 로드 (`elf_load_file`):**
    *   `PHDR` 테이블을 순회합니다.
    *   `PT_LOAD` 타입인 경우, `ata_read`를 이용해 디스크에서 메모리로 복사합니다.
    *   주의: `p_vaddr`가 현재 커널 메모리 맵과 충돌하지 않는지 확인해야 합니다. (유저 영역 `0x400000` 이상 권장).
4.  **실행 (`exec`):**
    *   기존 바이너리 로딩 방식을 ELF 파싱 방식으로 변경하거나, 파일 시그니처를 보고 분기 처리합니다.
    *   `create_user_process` 호출 시 진입점(`e_entry`)을 전달합니다.

## 6. 한국어 요약

OS_eojin에서 리눅스 호환 실행 파일을 실행하기 위해 ELF64 포맷을 지원합니다.
ELF 파일 맨 앞의 **ELF 헤더**를 읽어 파일이 맞는지 확인하고 진입점 주소를 얻습니다.
그 다음 **프로그램 헤더**를 읽어 파일의 내용(코드, 데이터)을 메모리의 올바른 위치에 복사합니다.
특히 `.bss` 섹션(초기화되지 않은 전역 변수)은 파일에 저장되지 않으므로, 메모리 상에서 0으로 초기화해 주는 작업이 필요합니다.
