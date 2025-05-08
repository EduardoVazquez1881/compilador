section .data
    b dd 5
    a dd 10
    c dd 10
    x dd 10

section .bss
    buffer resb 20

section .text
    global _start

_start:

    ; Escribir variable a
    mov eax, [a]
    call print_int

    ; Escribir variable b
    mov eax, [b]
    call print_int

while_start_0:
    mov eax, [a]
    mov ebx, [b]
    cmp eax, ebx
    jle while_end_0

    ; Escribir variable x
    mov eax, [x]
    call print_int
    jmp while_start_0
while_end_0:

    ; Terminar programa
    mov eax, 1
    xor ebx, ebx
    int 0x80
