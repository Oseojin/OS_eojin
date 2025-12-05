[ORG 0x7c00]
[BITS 16]

jmp short start
nop

; BPB - FAT16 호환
oem_name                db "MSWIN4.1"
bytes_per_sector        dw 512
sectors_per_cluster     db 1
reserved_sectors        dw 1
fat_count               db 2
root_dir_entries        dw 224
total_sector_16         dw 0
media_type              db 0xf8
fat_size_16             dw 9
sectors_per_track       dw 18
head_count              dw 2
hidden_sectors          dd 0
total_sectors_32        dd 0

; Extended BPB
drive_number            db 0x80
reserved                db 0
boot_signature          db 0x29
volume_id               dd 0x12345678
volume_label            db "OS_EOJIN   "
fs_type                 db "FAT16   "

; Bootloader Code
start:
    ; 세그먼트 초기화
    cli
    mov ax, 0
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7c00
    sti

    mov [drive_number], dl  ; BIOS가 넘겨준 드라이브 번호 저장

    ; 루트 디렉토리 위치 계산
    ; RootDirStart = ReservedSectors + (FatCount * FatSize)
    mov ax, [fat_size_16]
    mov bl, [fat_count]
    xor bh, bh
    mul bx                      ; AX = FatSize * FatCount
    add ax, [reserved_sectors]  ; AX = RootDirStart Sector (LBA)

    push ax                     ; 스택에 루트 디렉토리 시작 섹터 저장

    ; 루트 디렉토리 크기 계산 및 로드
    ; RootDirSectors = (RootDirEntries * 32) / 512
    mov ax, [root_dir_entries]
    shl ax, 5                       ; * 32
    xor dx, dx
    mov bx, [bytes_per_sector]
    div bx                          ; AX = 섹터 수

    mov cx, ax                      ; 읽을 섹터 수
    pop ax                          ; 루트 디렉토리 시작 LBA (복원)

    ; 데이터 영역 시작 LBA 계산
    ; DataStart = RootDirStart + RootDirSectors
    mov [data_start], ax
    add [data_start], cx

    mov bx, 0x500                   ; 버퍼 주소 (임시)
    push ax                         ; LBA 저장
    push cx                         ; 개수 저장

    call read_sectors               ; 루트 디렉토리 읽기 (AX = LBA, CX = Count, ES:BX = Buffer)

    ; 파일 찾기 ("LOADER  BIN")
    mov cx, [root_dir_entries]
    mov di, 0x500                   ; 루트 디렉토리 버퍼

.search_loop:
    push cx
    mov cx, 11                      ; 파일명 길이 (8 + 3)
    mov si, filename
    push di
    rep cmpsb                       ; 문자열 비교
    pop di
    je .found

    pop cx
    add di, 32                      ; 다음 엔트리
    loop .search_loop
    jmp .error                      ; 탐색 실패

.found:
    pop cx                          ; 스택 정리

    ; 시작 클러스터 번호 획득
    mov dx, [di + 26]               ; First Cluster Low (Offset 26)
    mov [cluster], dx               ; 저장

    ; FAT 테이블 읽기
    mov ax, 0x1000
    mov es, ax
    mov bx, 0                       ; ES:BX = 0x1000:0000

    mov ax, [reserved_sectors]
    mov cx, [fat_size_16]
    call read_sectors               ; FAT 테이블 로드

    ; 파일 내용 로드 (클러스터 체인 추적)
    mov ax, 0
    mov es, ax
    mov bx, 0x7e00

.load_file:
    mov ax, [cluster]

    ; 클러스터 -> LBA 변환
    ; LBA = DataStart + (Cluster - 2) * SectorPerCluster
    sub ax, 2
    xor cx, cx
    mov cl, [sectors_per_cluster]
    mul cx
    add ax, [data_start]            ; LBA 계산 완료

    mov cx, 1                       ; 클러스터 읽기

    push bx                         ; 현재 버퍼 위치 저장
    call read_sectors
    pop bx

    ; 다음 클러스터 구하기
    ; FAT 테이블 위치: 0x10000 + (Cluster * 2)
    mov ax, 0x1000
    mov fs, ax                      ; FS 세그먼트로 FAT 접근

    mov ax, [cluster]
    shl ax, 1                       ; * 2 (16비트 엔트리)
    mov di, ax
    mov dx, [fs:di]                 ; 다음 클러스터 읽기

    mov [cluster], dx               ; 갱신

    ; 버퍼 포인터 이동
    xor ax, ax
    mov al, [sectors_per_cluster]
    mov cx, 512
    mul cx                          ; 한 클러스터 크기 (Byte)
    add bx, ax                      ; 버퍼 주소 증가
    
    ; End of Chain 확인 (0xfff8 이상)
    cmp dx, 0xfff8
    jb .load_file

    ; 로더 실행
    jmp 0x0000:0x7e00

.error:
    mov si, msg_error
    call print_string
    jmp $

; Helper Funtions
; read_sectors: LBA 모드로 섹터 읽기

; AX = LBA, CX = Count, ES:BX = Buffer
read_sectors:
    pusha
.loop:
    push ax
    push cx

    call lba_to_chs                 ; AX(LBA) -> CHS 변환 (결과는 CX, DH에)

    mov ah, 0x02                    ; Read Sectors
    mov al, 1                       ; 1 sector
    mov dl, [drive_number]
    int 0x13
    jc .read_error

    pop cx
    pop ax
    inc ax                          ; 다음 LBA
    add bx, 512                     ; 버퍼 이동
    loop .read_loop_internal

    popa
    ret

.read_loop_internal:
    jmp .loop

.read_error:
    mov si, msg_read_err
    call print_string
    jmp $

; LBA to CHS 변환
; LBA % SectorsPerTrack + 1 = Sector
; (LBA / SectorsPerTrack) % Heads = Head
; (LBA / SectorsPerTrack) / Heads = Cylinder
lba_to_chs:
    xor dx, dx
    div word [sectors_per_track]        ; AX = LBA / SPT, DX = LBA % SPT
    inc dx
    mov cl, dl                          ; Sector = CL

    xor dx, dx
    div word [head_count]               ; AX = Cylinder, DX = Head
    mov dh, dl                          ; Head = DH
    mov ch, al                          ; Cylinder Low = CH
    ret

print_string:
    mov ah, 0x0e
.ps_loop:
    lodsb
    cmp al, 0
    je .ps_done
    int 0x10
    jmp .ps_loop
.ps_done:
    ret

; Data
filename            db "LOADER  BIN"
cluster             dw 0
data_start          dw 0
msg_error           db "No LOADER.BIN", 0
msg_read_err        db "Read Err", 0

times 510 - ($ - $$) db 0
dw 0xaa55