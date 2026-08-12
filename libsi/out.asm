global si_out

section .text

; void out(int value)
; Receives value in edi and writes a signed decimal number followed by '\n'.
si_out:
    push rbp
    mov rbp, rsp
    sub rsp, 48

    mov eax, edi
    xor r8d, r8d

    test eax, eax
    jns .magnitude_ready

    mov r8d, 1
    neg eax

.magnitude_ready:
    lea rsi, [rbp - 1]
    mov byte [rsi], 10
    mov ecx, 10

.convert_digit:
    xor edx, edx
    div ecx
    add dl, '0'
    dec rsi
    mov byte [rsi], dl
    test eax, eax
    jnz .convert_digit

    test r8d, r8d
    jz .write

    dec rsi
    mov byte [rsi], '-'

.write:
    mov rdx, rbp
    sub rdx, rsi

    mov eax, 1
    mov edi, 1
    syscall

    mov rsp, rbp
    pop rbp
    ret