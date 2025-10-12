section .data
    ; Тестовые данные 1
    a1 db 0x2A, 0x03, 0x12, 0x0D, 0x43, 0xE2, 0x34, 0x02, 0x0B, 0x04, 0x23, 0x09, 0x7A, 0x1C, 0x5E, 0x6A, 0x11
    b1 db 0x15, 0xDD, 0x34, 0x4B, 0x57, 0x7F, 0xCD, 0x05, 0x16, 0x09, 0x3D, 0x08, 0x72, 0x2B, 0x6C, 0x4E, 0x0F
    
    ; Тестовые данные 2
    a2 db 0x7C, 0x1A, 0x3B, 0x55, 0x2F, 0x89, 0x40, 0x9D, 0x1E, 0x04, 0x3A, 0x7F, 0x92, 0x6D, 0x11, 0x33, 0x45
    b2 db 0x22, 0x8B, 0x47, 0x9E, 0x0D, 0x71, 0x58, 0x2A, 0x77, 0x5C, 0x14, 0x3C, 0xB0, 0x8E, 0x21, 0x67, 0x52
    
    ; Тестовые данные 3
    a3 db 0x58, 0x0E, 0x6D, 0x39, 0x83, 0x94, 0x22, 0x16, 0x4F, 0x7B, 0x2C, 0x9A, 0x01, 0x3F, 0x66, 0x9C, 0x5B
    b3 db 0xA4, 0x19, 0x23, 0x50, 0x67, 0x7A, 0x33, 0x48, 0x3A, 0x11, 0x8D, 0x5E, 0x77, 0x22, 0x4B, 0x10, 0x27

    ; Буферы для результатов
    r1 times 17 db 0
    r2 times 17 db 0
    r3 times 17 db 0

    ; Строки для вывода
    msg db "Test %d:", 0x0A, "a = ", 0
    msg_b db "b = ", 0
    msg_r db "Result = ", 0
    hex_fmt db "%02X ", 0
    newline db 0x0A, 0

section .text
    global main
    extern printf

main:
    ; ===== ТЕСТ 1 =====
    push 1
    push msg
    call printf
    add esp, 8
    
    ; Выводим массив a1
    mov ecx, 17
    mov esi, a1
print_a1:
    movzx eax, byte [esi]
    push ecx
    push esi
    push eax
    push hex_fmt
    call printf
    add esp, 8
    pop esi
    pop ecx
    inc esi
    loop print_a1
    
    push newline
    call printf
    add esp, 4
    
    ; Выводим массив b1
    push msg_b
    call printf
    add esp, 4
    
    mov ecx, 17
    mov esi, b1
print_b1:
    movzx eax, byte [esi]
    push ecx
    push esi
    push eax
    push hex_fmt
    call printf
    add esp, 8
    pop esi
    pop ecx
    inc esi
    loop print_b1
    
    push newline
    call printf
    add esp, 4
    
    ; Сложение a1 + b1
    clc
    mov ecx, 16
add_loop1:
    mov al, [a1 + ecx]
    adc al, [b1 + ecx]
    mov [r1 + ecx], al
    dec ecx
    jns add_loop1
    
    ; Выводим результат r1
    push msg_r
    call printf
    add esp, 4
    
    mov ecx, 17
    mov esi, r1
print_r1:
    movzx eax, byte [esi]
    push ecx
    push esi
    push eax
    push hex_fmt
    call printf
    add esp, 8
    pop esi
    pop ecx
    inc esi
    loop print_r1
    
    push newline
    call printf
    add esp, 4
    
    ; ===== ТЕСТ 2 =====
    push 2
    push msg
    call printf
    add esp, 8
    
    ; Выводим массив a2
    mov ecx, 17
    mov esi, a2
print_a2:
    movzx eax, byte [esi]
    push ecx
    push esi
    push eax
    push hex_fmt
    call printf
    add esp, 8
    pop esi
    pop ecx
    inc esi
    loop print_a2
    
    push newline
    call printf
    add esp, 4
    
    ; Выводим массив b2
    push msg_b
    call printf
    add esp, 4
    
    mov ecx, 17
    mov esi, b2
print_b2:
    movzx eax, byte [esi]
    push ecx
    push esi
    push eax
    push hex_fmt
    call printf
    add esp, 8
    pop esi
    pop ecx
    inc esi
    loop print_b2
    
    push newline
    call printf
    add esp, 4
    
    ; Сложение a2 + b2
    clc
    mov ecx, 16
add_loop2:
    mov al, [a2 + ecx]
    adc al, [b2 + ecx]
    mov [r2 + ecx], al
    dec ecx
    jns add_loop2
    
    ; Выводим результат r2
    push msg_r
    call printf
    add esp, 4
    
    mov ecx, 17
    mov esi, r2
print_r2:
    movzx eax, byte [esi]
    push ecx
    push esi
    push eax
    push hex_fmt
    call printf
    add esp, 8
    pop esi
    pop ecx
    inc esi
    loop print_r2
    
    push newline
    call printf
    add esp, 4
    
    ; ===== ТЕСТ 3 =====
    push 3
    push msg
    call printf
    add esp, 8
    
    ; Выводим массив a3
    mov ecx, 17
    mov esi, a3
print_a3:
    movzx eax, byte [esi]
    push ecx
    push esi
    push eax
    push hex_fmt
    call printf
    add esp, 8
    pop esi
    pop ecx
    inc esi
    loop print_a3
    
    push newline
    call printf
    add esp, 4
    
    ; Выводим массив b3
    push msg_b
    call printf
    add esp, 4
    
    mov ecx, 17
    mov esi, b3
print_b3:
    movzx eax, byte [esi]
    push ecx
    push esi
    push eax
    push hex_fmt
    call printf
    add esp, 8
    pop esi
    pop ecx
    inc esi
    loop print_b3
    
    push newline
    call printf
    add esp, 4
    
    ; Сложение a3 + b3
    clc
    mov ecx, 16
add_loop3:
    mov al, [a3 + ecx]
    adc al, [b3 + ecx]
    mov [r3 + ecx], al
    dec ecx
    jns add_loop3
    
    ; Выводим результат r3
    push msg_r
    call printf
    add esp, 4
    
    mov ecx, 17
    mov esi, r3
print_r3:
    movzx eax, byte [esi]
    push ecx
    push esi
    push eax
    push hex_fmt
    call printf
    add esp, 8
    pop esi
    pop ecx
    inc esi
    loop print_r3
    
    push newline
    call printf
    add esp, 4

    ; Завершение программы
    mov eax, 1
    mov ebx, 0
    int 0x80