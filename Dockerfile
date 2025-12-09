FROM ubuntu:22.04

ENV DEBIAN_FRONTEND=noninteractive

# 1. 필수 패키지 설치
RUN apt-get update && apt-get install -y \
    build-essential \
    nasm \
    qemu-system-x86 \
    dosfstools \
    mtools \
    && rm -rf /var/lib/apt/lists/*

# 2. 작업 디렉토리 설정
WORKDIR /usr/src/os

# 3. 소스 코드 복사 (Host -> Container)
# .dockerignore에 정의된 파일(build, .git 등)은 제외됩니다.
COPY . .

# 4. OS 이미지 빌드
RUN make clean && make

# 5. 실행 명령 설정 (QEMU Curses 모드)
ENV TERM=xterm
CMD ["qemu-system-x86_64", "-m", "512M", "-drive", "file=build/os-image.bin,format=raw,index=0,media=disk", "-display", "curses", "-vga", "std"]