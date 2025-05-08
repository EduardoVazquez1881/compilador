section .data
    x dq 0              ; variable x inicializada en 0

section .bss
    buffer resb 20      ; espacio para convertir número a texto

section .text
    global _start

_start:
    ; Bucle: while (x < 5)
while_start:
    mov rax, [x]
    cmp rax, 5
    jge while_end       ; si x >= 5, salir del while

    ; --- Convertir x a string e imprimir ---
    mov rbx, 10
    lea rdi, [buffer+19]
    mov byte [rdi], 0xA     ; salto de línea

.to_string:
    dec rdi
    xor rdx, rdx
    div rbx
    add dl, '0'
    mov [rdi], dl
    test rax, rax
    jnz .to_string

    ; escribir en pantalla
    mov rsi, rdi
    mov rdi, 1              ; stdout
    mov rax, 1              ; syscall: write
    lea rcx, [buffer+20]
    sub rcx, rsi
    mov rdx, rcx
    syscall

    ; x = x + 1
    mov rax, [x]
    add rax, 1
    mov [x], rax

    jmp while_start

while_end:
    ; salir
    mov rax, 60         ; syscall: exit
    xor rdi, rdi
    syscall
