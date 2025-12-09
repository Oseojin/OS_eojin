# OS_eojin

**OS_eojin**은 스크래치부터 개발된 64비트 x86 아키텍처용 커스텀 운영체제입니다.
커스텀 부트로더, 보호 모드(Protected Mode)를 거친 롱 모드(Long Mode) 커널 진입, FAT16 파일 시스템, 그리고 링 3(Ring 3) 유저 모드 애플리케이션 실행을 지원합니다.

## 🚀 주요 기능 (Features)

*   **아키텍처:** x86_64 Long Mode (64-bit)
*   **부팅:** 레거시 BIOS 부팅 (MBR -> Protected Mode -> Long Mode)
*   **메모리 관리:**
    *   비트맵 방식의 물리 메모리 할당자 (PMM)
    *   페이징(4-Level Paging) 기반 가상 메모리 관리 (VMM)
    *   동적 힙 할당자 (Kmalloc/Kfree)
*   **파일 시스템:** FAT16 지원 (읽기/쓰기/생성/삭제, 디렉토리 지원)
*   **멀티태스킹:** 라운드 로빈 스케줄링, 커널/유저 스레드 지원
*   **유저 모드:** Ring 3 격리, 시스템 콜(`int 0x80`), ELF64 바이너리 실행 (`exec`)
*   **드라이버:** VGA 텍스트 모드, PS/2 키보드, PIT 타이머, ATA PIO 하드 디스크
*   **셸(Shell):** 파일 시스템 탐색 및 시스템 제어를 위한 내장 명령어 제공

## 🛠️ 개발 환경 설정 (Prerequisites)

이 프로젝트는 **Linux** 환경(Ubuntu/Debian 계열 권장)에서 개발되었습니다. 빌드를 위해 다음 패키지들이 필요합니다.

### 패키지 설치 (Ubuntu/Debian)
터미널을 열고 다음 명령어를 입력하여 필수 도구를 설치하세요.

```bash
sudo apt update
sudo apt install build-essential nasm qemu-system-x86 dosfstools mtools
```

*   `nasm`: 어셈블러
*   `gcc`: C 컴파일러 (64비트 호스트 기준)
*   `qemu-system-x86`: 에뮬레이터 (테스트용)
*   `dosfstools`: `mkfs.fat` (디스크 이미지 포맷용)
*   `mtools`: `mcopy`, `mmd` (FAT 이미지 조작용)

## 🏗️ 빌드 및 실행 (Build & Run)

### 1. 전체 빌드
소스 코드를 컴파일하고 부팅 가능한 디스크 이미지(`build/os-image.bin`)를 생성합니다.

```bash
make
```

### 2. QEMU에서 실행 (에뮬레이터)
빌드된 이미지를 QEMU 에뮬레이터에서 즉시 실행합니다.

```bash
make run
```

### 3. 클린 빌드
빌드된 파일들을 삭제하고 초기화하려면 다음을 실행하세요.

```bash
make clean
```

## 💾 실물 PC에서 실행 (USB 부팅)

생성된 `os-image.bin`은 실제 하드웨어에서도 부팅 가능합니다.
**주의:** 이 운영체제는 **Legacy BIOS** 모드만 지원하며, UEFI 부팅은 지원하지 않습니다.

1.  **USB 드라이브 준비:** 4GB 이상의 USB를 준비합니다. (내용이 삭제되니 주의하세요!)
2.  **이미지 굽기:** `dd` 명령어를 사용합니다. (`/dev/sdX`는 본인의 USB 장치 경로로 변경해야 합니다. `lsblk`로 확인하세요.)

    ```bash
    # 주의: /dev/sdX를 실제 USB 경로로 반드시 변경하세요! (예: /dev/sdb)
    sudo dd if=build/os-image.bin of=/dev/sdX bs=4M status=progress && sync
    ```

3.  **부팅 설정:**
    *   PC를 재부팅하고 BIOS 설정으로 진입합니다.
    *   **Secure Boot**를 비활성화(Disable)합니다.
    *   부팅 모드를 **Legacy (CSM)**로 설정합니다.
    *   USB를 부팅 1순위로 지정하고 부팅합니다.

## 📂 디렉토리 구조

*   `src/boot/`: 부트로더 어셈블리 소스 (MBR, 모드 전환)
*   `src/kernel/`: 커널 핵심 로직 (C, ASM)
    *   `cpu/`: GDT, IDT 등 CPU 제어
    *   `process/`: 스케줄러 및 프로세스 관리
*   `src/drivers/`: 하드웨어 드라이버 (키보드, VGA, ATA 등)
*   `src/fs/`: FAT16 파일 시스템 구현
*   `src/user/`: 유저 모드 애플리케이션 (`hello.c` 등)
*   `includes/`: 공용 헤더 파일
*   `build/`: 빌드 결과물 저장소 (바이너리, 오브젝트 파일)

## 명령어 (Shell)

*   `halt`: 시스템 종료



## 🐳 Docker 환경에서 바로 실행하기



로컬 환경 설정 없이 Docker만으로 OS를 빌드하고, 리눅스 셸에 접속하듯 터미널에서 즉시 실행해볼 수 있습니다.



### 1. Docker 이미지 빌드

이 이미지는 소스 코드를 포함하며, 빌드까지 자동으로 완료된 상태로 생성됩니다.



```bash

docker build -t os-eojin .

```



### 2. 실행 (터미널 모드)

다음 명령어를 입력하면 QEMU가 터미널 모드(Curses)로 실행되어, 별도의 창 없이 **현재 터미널 안에서 OS_eojin을 조작**할 수 있습니다.



```bash

docker run -it --rm os-eojin

```



*   **종료 방법:** `ALT + 2`를 눌러 QEMU 모니터로 전환한 뒤 `quit`을 입력하거나, 단순히 터미널 창을 닫으면 됩니다. (또는 OS 내에서 `shutdown` 명령 사용)







### ⚠️ Docker Desktop 사용자 주의사항



Docker Desktop의 **'Run' (재생) 버튼**을 눌러 실행하면 정상적으로 작동하지 않습니다. (QEMU Curses 모드는 대화형 터미널(TTY) 할당이 필수이기 때문입니다.)



반드시 **터미널(PowerShell, CMD, Git Bash 등)을 열고** 위의 `docker run -it --rm <image_name> .` 명령어를 직접 입력하여 실행하세요.







---







### 🔧 개발용 모드 (소스 코드 수정 시)



코드를 수정하고 다시 빌드하는 과정을 반복한다면, 로컬 소스 코드를 마운트하여 사용하는 것이 좋습니다.



1.  **소스 마운트 및 빌드:**

    ```bash

    # 로컬의 코드를 컨테이너에 연결하여 빌드 (결과물은 로컬 build/ 에 저장됨)

    docker run --rm -v $(pwd):/usr/src/os os-eojin make

    ```



2.  **GUI 모드로 실행 (Linux Only):**

    그래픽 창(QEMU SDL/GTK)을 띄우려면 X11 포워딩이 필요합니다.

    ```bash

    xhost +local:docker

    docker run --rm -it --privileged -e DISPLAY=$DISPLAY -v /tmp/.X11-unix:/tmp/.X11-unix -v $(pwd):/usr/src/os os-eojin make run

    ```
