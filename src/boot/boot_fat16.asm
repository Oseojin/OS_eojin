[ORG 0x7c00]
[BITS 16]

jmp short start
nop

; BPB (BIOS Parameter Block) - FAT16
oem_name                db "MSWIN4.1"
bytes_per_sector        dw 512
sectors_per_cluster     db 1
reserved_sectors        dw 1
fat_count               db 2
root_dir_entries        dw 512
total_sector_16         dw 0
media_type              db 0xf8
fat_size_16             dw 80
sectors_per_track       dw 32
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

start:
    cli
    cld
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7c00
    sti

    mov [drive_number], dl

    ; BIOS Extended Read (LBA) Check
    mov ah, 0x41
    mov bx, 0x55aa
    mov dl, [drive_number]
    int 0x13
    jc .no_lba
    cmp bx, 0xaa55
    jne .no_lba

    ; [DEBUG] L
    mov ah, 0x0e
    mov al, 'L'
    int 0x10

    ; Calculate Root Dir Start LBA
    mov ax, [fat_size_16]
    xor cx, cx
    mov cl, [fat_count]
    mul cx
    add ax, [reserved_sectors]
    push ax                 ; Save RootDirStart

    ; Calculate Root Dir Size (sectors)
    mov ax, [root_dir_entries]
    shl ax, 5
    xor dx, dx
    mov bx, [bytes_per_sector]
    div bx
    
    mov cx, ax              ; CX = RootSectors
    pop ax                  ; AX = RootDirStart LBA
    push ax                 ; Save RootDirStart
    add ax, cx              ; AX = DataStart LBA
    mov [data_start], ax

    ; Read Root Directory
    pop ax                  ; AX = RootDirStart
    mov bx, 0x500           ; Buffer 0x500
    call read_sectors       ; (CX = RootSectors)

    ; Search for File
    mov cx, [root_dir_entries]
    mov di, 0x500
.search:
    push cx
    mov cx, 11
    mov si, filename
    push di
    rep cmpsb
    pop di
    je .found
    pop cx
    add di, 32
    loop .search
    jmp disk_error              ; File not found

.found:
    pop cx
    mov dx, [di + 26]       ; First Cluster
    mov [cluster], dx

    ; Load FAT Table
    mov ax, [reserved_sectors]
    mov cx, [fat_size_16]
    mov bx, 0x2000
    mov es, bx
    xor bx, bx
    call read_sectors

    ; Load File (Cluster Chain)
    xor ax, ax
    mov es, ax
    mov bx, 0x7e00

.load_loop:
    ; [DEBUG] .
    mov ah, 0x0e
    mov al, '.'
    int 0x10

    mov ax, [cluster]
    sub ax, 2
    xor cx, cx
    mov cl, [sectors_per_cluster]
    mul cx
    add ax, [data_start]    ; Calculate LBA
    
    xor cx, cx
    mov cl, [sectors_per_cluster]
    push bx
    call read_sectors
    pop bx

    ; Next Cluster Lookup
    mov ax, 0x2000
    mov fs, ax
    mov ax, [cluster]
    shl ax, 1
    mov di, ax
    mov dx, [fs:di]
    mov [cluster], dx   ; Update

    test dx, dx         ; Check if next cluster is 0 (Invalid/Empty)
    jz fat_zero_error

    ; Buffer Advance
    xor ax, ax
    mov al, [sectors_per_cluster]
    mov cx, 512
    mul cx
    add bx, ax
    jnc .no_overflow
    mov ax, es
    add ax, 0x1000
    mov es, ax
.no_overflow:

    ; Reload Next Cluster (DX was clobbered by MUL)
    mov dx, [cluster]

    ; Loop Check (Safe Comparison)
    mov ax, 0xfff8
    cmp dx, ax
    jae .success
    jmp .load_loop

.success:
    ; [DEBUG] J
    mov ah, 0x0e
    mov al, 'J'
    int 0x10
    jmp 0x0000:0x7e00

.no_lba:
    mov ah, 0x0e
    mov al, 'N'
    int 0x10
    jmp $

fat_zero_error:
    mov ah, 0x0e
    mov al, 'Z'
    int 0x10
    jmp $

disk_error:
    mov ah, 0x0e
    mov al, 'E'
    int 0x10
    jmp $

; ------------------------------------------------------------------
; read_sectors: LBA Extended Read (AH=42h) Using Stack DAP
; AX = LBA (Low 16bit), CX = Count, ES:BX = Buffer
; ------------------------------------------------------------------
read_sectors:
    pusha
    and eax, 0x0000ffff
.loop:
    push dword 0        ; LBA High [32bit] = 0
    push eax            ; LBA Low  [32bit] = AX (Zero extended)
    push es             ; Segment  [16bit]
    push bx             ; Offset   [16bit]
    push word 1         ; Count    [16bit] = 1
    push word 0x10      ; Size     [16bit] = 16 bytes

    mov ah, 0x42
    mov dl, [drive_number]
    mov si, sp          ; SI = Stack Pointer (DAP Start)
    int 0x13
    
    add sp, 16          ; Clean Stack

    jc .fail

    inc ax              ; Next LBA
    add bx, 512         ; Buffer Advance
    
    dec cx
    jnz .loop
    popa
    ret
.fail:
    jmp disk_error

filename    db "LOADER  BIN"
cluster     dw 0
data_start  dw 0

times 510 - ($ - $$) db 0
dw 0xaa55