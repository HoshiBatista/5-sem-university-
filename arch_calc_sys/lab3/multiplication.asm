section .data
    ; Тестовые данные
    test1_a dd 0FFFFFFFFh, 0FFFFFFFFh  ; 0xFFFFFFFFFFFFFFFF
    test1_b dd 0FFFFFFFFh, 0FFFFFFFFh  ; 0xFFFFFFFFFFFFFFFF
    
    test2_a dd 0FFFFFFFFh, 0FFFFFFFFh  ; 0xFFFFFFFFFFFFFFFF  
    test2_b dd 5, 0                    ; 5
    
    test3_a dd 5, 0                    ; 5
    test3_b dd 0FFFFFFFFh, 0FFFFFFFFh  ; 0xFFFFFFFFFFFFFFFF
    
    test4_a dd 00001024h, 0            ; 0x1024
    test4_b dd 00000000h, 0FFFFFFFFh   ; 0xFFFFFFFF00000000
    
    test5_a dd 09ABCDEF0h, 012345678h  ; 0x123456789ABCDEF0
    test5_b dd 076543210h, 0FEDCBA98h  ; 0xFEDCBA9876543210
    
    test6_a dd 00000000h, 00000001h    ; 0x100000000 (2^32)
    test6_b dd 00000000h, 00000001h    ; 0x100000000 (2^32)
    
    ; Буфер для результата (16 байт)
    result dd 0, 0, 0, 0
    
    ; Форматные строки
    str_fmt db "%08x %08x %08x %08x", 10, 0
    test_msg db "Test %d: ", 0
    expected_msg db "Expected: ", 0
    asm_msg db "ASM:      ", 0

section .text
    global main
    extern printf

; =============================================
; ПОДПРОГРАММА: Умножение 64-битных чисел
; Вход: ESI = указатель на первое число (2 dword)
;       EDI = указатель на второе число (2 dword)  
;       EBX = указатель на результат (4 dword)
; =============================================
multiply_64bit:
    push eax
    push ecx
    push edx
    push esi
    push edi
    push ebx
    
    ; Обнуляем результат
    mov dword [ebx], 0
    mov dword [ebx+4], 0
    mov dword [ebx+8], 0
    mov dword [ebx+12], 0
    
    ; Умножение младших частей: A0 * B0
    mov eax, [esi]          ; EAX = младшие 32 бита A
    mov ecx, [edi]          ; ECX = младшие 32 бита B
    mul ecx                 ; EDX:EAX = A0 * B0
    mov [ebx], eax          ; Сохраняем младшую часть
    mov [ebx+4], edx        ; Сохраняем перенос
    
    ; Умножение A0 * B1
    mov eax, [esi]          ; EAX = младшие 32 бита A  
    mov ecx, [edi+4]        ; ECX = старшие 32 бита B
    mul ecx                 ; EDX:EAX = A0 * B1
    add [ebx+4], eax        ; Добавляем к средней части
    adc [ebx+8], edx        ; Добавляем с переносом
    adc dword [ebx+12], 0   ; Учитываем перенос
    
    ; Умножение A1 * B0  
    mov eax, [esi+4]        ; EAX = старшие 32 бита A
    mov ecx, [edi]          ; ECX = младшие 32 бита B
    mul ecx                 ; EDX:EAX = A1 * B0
    add [ebx+4], eax        ; Добавляем к средней части
    adc [ebx+8], edx        ; Добавляем с переносом
    adc dword [ebx+12], 0   ; Учитываем перенос
    
    ; Умножение A1 * B1
    mov eax, [esi+4]        ; EAX = старшие 32 бита A
    mov ecx, [edi+4]        ; ECX = старшие 32 бита B
    mul ecx                 ; EDX:EAX = A1 * B1
    add [ebx+8], eax        ; Добавляем к старшей части
    adc [ebx+12], edx       ; Добавляем с переносом
    
    pop ebx
    pop edi
    pop esi
    pop edx
    pop ecx
    pop eax
    ret

; =============================================
; ПОДПРОГРАММА: Вывод результата
; Вход: EBX = указатель на результат
; =============================================
print_result:
    push eax
    push ebx
    push ecx
    push edx
    
    push dword [ebx+12]     ; Самые старшие 32 бита (part3)
    push dword [ebx+8]      ; Старшие 32 бита (part2)
    push dword [ebx+4]      ; Младшие 32 бита (part1)
    push dword [ebx]        ; Самые младшие 32 бита (part0)
    push str_fmt
    call printf
    add esp, 20
    
    pop edx
    pop ecx
    pop ebx
    pop eax
    ret

; =============================================
; ОСНОВНАЯ ПРОГРАММА
; =============================================
main:
    ; Тест 1: FFFFFFFFFFFFFFFF * FFFFFFFFFFFFFFFF
    push 1
    push test_msg
    call printf
    add esp, 8
    
    mov esi, test1_a
    mov edi, test1_b  
    mov ebx, result
    call multiply_64bit
    call print_result
    
    ; Тест 2: FFFFFFFFFFFFFFFF * 5
    push 2
    push test_msg
    call printf
    add esp, 8
    
    mov esi, test2_a
    mov edi, test2_b
    mov ebx, result
    call multiply_64bit
    call print_result
    
    ; Тест 3: 5 * FFFFFFFFFFFFFFFF
    push 3
    push test_msg
    call printf
    add esp, 8
    
    mov esi, test3_a
    mov edi, test3_b
    mov ebx, result
    call multiply_64bit
    call print_result
    
    ; Тест 4: 0x1024 * 0xFFFFFFFF00000000
    push 4
    push test_msg
    call printf
    add esp, 8
    
    mov esi, test4_a
    mov edi, test4_b
    mov ebx, result
    call multiply_64bit
    call print_result
    
    ; Тест 5: 0x123456789ABCDEF0 * 0xFEDCBA9876543210
    push 5
    push test_msg
    call printf
    add esp, 8
    
    mov esi, test5_a
    mov edi, test5_b
    mov ebx, result
    call multiply_64bit
    call print_result
    
    ; Тест 6: 0x100000000 * 0x100000000
    push 6
    push test_msg
    call printf
    add esp, 8
    
    mov esi, test6_a
    mov edi, test6_b
    mov ebx, result
    call multiply_64bit
    call print_result
    
    ; Завершение
    mov eax, 1
    xor ebx, ebx
    int 0x80