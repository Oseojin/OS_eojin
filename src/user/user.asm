[BITS 64]
[ORG 0x4000000]

start:
    mov rax, 1          ; Syscall 1 (Print)
    mov rbx, msg
    int 0x80

    mov rax, 0          ; Syscall 0 (Exit)
    int 0x80            ; Call Kernel to kill this process

msg db "Hello from User Mode! (Syscall)", 0x0A, 0