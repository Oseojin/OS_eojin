[BITS 64]
[ORG 0x4000000]

start:
    mov rax, 1          ; Syscall 1 (Print)
    mov rbx, msg
    int 0x80
    ret

msg db "Hello from User Mode! (Syscall)", 0
