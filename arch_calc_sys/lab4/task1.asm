section .data
    ; Тестовые данные - корректные значения для 2-байтовых и 1-байтовых переменных
    x1 dw -1        ; знаковое 2-байтовое (-32768 до 32767)
    y1 db 5          ; беззнаковое однобайтовое (0 до 255)
    z1 db 2          ; беззнаковое однобайтовое (0 до 255)

    x2 dw 3          ; знаковое 2-байтовое
    y2 db 2         ; беззнаковое однобайтовое
    z2 db 0          ; беззнаковое однобайтовое

    x3 dw -4         ; знаковое 2-байтовое
    y3 db 3          ; беззнаковое однобайтовое
    z3 db 1          ; беззнаковое однобайтовое

    x4 dw 7          ; знаковое 2-байтовое
    y4 db 4         ; беззнаковое однобайтовое
    z4 db 3          ; беззнаковое однобайтовое

    ; Сообщения для вывода
    result_msg db "Result: %d", 10, 0
    test_msg db "Test %d: x=%d, y=%u, z=%u", 10, 0
    case1_msg db "Case 1: z not in [0,1] and y<0", 10, 0
    case2_msg db "Case 2: |x|>5 and |y|<=8", 10, 0  
    case3_msg db "Case 3: |x|<=5", 10, 0
    newline db 10, 0

section .bss
    result resd 1

section .text
    global main
    extern printf

main:
    ; Тест 1
    movsx eax, word [x1]   ; корректное чтение 2-байтового x
    movzx ebx, byte [y1]   ; корректное чтение 1-байтового y
    movzx ecx, byte [z1]   ; корректное чтение 1-байтового z
    
    push ecx
    push ebx
    push eax
    push 1
    push test_msg
    call printf
    add esp, 20
    
    ; Определяем случай и выводим сообщение
    movsx eax, word [x1]
    movzx ebx, byte [y1]
    movzx ecx, byte [z1]
    call determine_case
    push eax
    call printf
    add esp, 4
    
    mov ax, [x1]
    mov bl, [y1]
    mov cl, [z1]
    call calculate_function
    
    push dword [result]
    push result_msg
    call printf
    add esp, 8
    
    push newline
    call printf
    add esp, 4
    
    ; Тест 2
    movsx eax, word [x2]
    movzx ebx, byte [y2]
    movzx ecx, byte [z2]
    
    push ecx
    push ebx
    push eax
    push 2
    push test_msg
    call printf
    add esp, 20
    
    movsx eax, word [x2]
    movzx ebx, byte [y2]
    movzx ecx, byte [z2]
    call determine_case
    push eax
    call printf
    add esp, 4
    
    mov ax, [x2]
    mov bl, [y2]
    mov cl, [z2]
    call calculate_function
    
    push dword [result]
    push result_msg
    call printf
    add esp, 8
    
    push newline
    call printf
    add esp, 4
    
    ; Тест 3
    movsx eax, word [x3]
    movzx ebx, byte [y3]
    movzx ecx, byte [z3]
    
    push ecx
    push ebx
    push eax
    push 3
    push test_msg
    call printf
    add esp, 20
    
    movsx eax, word [x3]
    movzx ebx, byte [y3]
    movzx ecx, byte [z3]
    call determine_case
    push eax
    call printf
    add esp, 4
    
    mov ax, [x3]
    mov bl, [y3]
    mov cl, [z3]
    call calculate_function
    
    push dword [result]
    push result_msg
    call printf
    add esp, 8
    
    push newline
    call printf
    add esp, 4
    
    ; Тест 4
    movsx eax, word [x4]
    movzx ebx, byte [y4]
    movzx ecx, byte [z4]
    
    push ecx
    push ebx
    push eax
    push 4
    push test_msg
    call printf
    add esp, 20
    
    movsx eax, word [x4]
    movzx ebx, byte [y4]
    movzx ecx, byte [z4]
    call determine_case
    push eax
    call printf
    add esp, 4
    
    mov ax, [x4]
    mov bl, [y4]
    mov cl, [z4]
    call calculate_function
    
    push dword [result]
    push result_msg
    call printf
    add esp, 8

    ; Завершение программы
    mov eax, 1
    xor ebx, ebx
    int 0x80

determine_case:
    ; Вход: EAX = x, EBX = y, ECX = z
    ; Выход: EAX = адрес строки с описанием случая
    
    ; Сохраняем регистры
    push ebx
    push ecx
    push edx
    
    ; Проверяем случай 1: z не в [0,1] и y < 0
    ; Но y беззнаковое, поэтому y < 0 никогда не выполняется
    ; Поэтому случай 1 никогда не активируется
    
    ; Проверяем случай 2: |x|>5 и |y|<=8
    mov edx, eax
    cmp edx, 0
    jge .check_abs_x2
    neg edx
.check_abs_x2:
    cmp edx, 5
    jle .check_case3
    cmp ebx, 8
    jg .check_case3
    mov eax, case2_msg
    jmp .end
    
.check_case3:
    ; Проверяем случай 3: |x|<=5
    mov edx, eax
    cmp edx, 0
    jge .check_abs_x3
    neg edx
.check_abs_x3:
    cmp edx, 5
    jg .default_case
    mov eax, case3_msg
    jmp .end
    
.default_case:
    ; Если ни одно условие не выполняется
    mov eax, case1_msg  ; Хотя случай 1 не выполняется, используем это для "default"
    
.end:
    pop edx
    pop ecx
    pop ebx
    ret

calculate_function:
    ; Вход: AX = x (знаковое 16-битное), BL = y (беззнаковое 8-битное), CL = z (беззнаковое 8-битное)
    ; Выход: result
    
    ; Сохраняем регистры
    push eax
    push ebx
    push ecx
    push edx
    push esi
    push edi
    
    ; Преобразуем к 32-битным значениям
    movsx eax, ax        ; x (знаковое расширение)
    movzx ebx, bl        ; y (беззнаковое расширение) 
    movzx ecx, cl        ; z (беззнаковое расширение)
    
    ; Сохраняем оригинальные значения
    push eax  ; x
    push ebx  ; y
    push ecx  ; z
    
    ; Проверяем условие 2: |x|>5 и |y|<=8
    mov edx, eax
    cmp edx, 0
    jge .abs_x_done2
    neg edx
.abs_x_done2:
    
    mov esi, ebx  ; |y| = y (так как y беззнаковое)
    
    cmp edx, 5
    jle .check_case3
    cmp esi, 8
    jg .check_case3
    
    ; Случай 2: z + 1 + x*y
    pop ecx  ; z
    pop ebx  ; y
    pop eax  ; x
    
    mov edx, ecx    ; z
    add edx, 1      ; z + 1
    
    mov esi, eax    ; x
    imul esi, ebx   ; x*y (знаковое * беззнаковое - приводим к знаковому)
    
    add edx, esi    ; z + 1 + x*y
    
    mov [result], edx
    jmp .end
    
.check_case3:
    ; Восстанавливаем значения
    pop ecx
    pop ebx
    pop eax
    push eax
    push ebx
    push ecx
    
    ; Проверяем условие 3: |x|<=5
    mov edx, eax
    cmp edx, 0
    jge .abs_x_done3
    neg edx
.abs_x_done3:
    
    cmp edx, 5
    jg .default_case
    
    ; Случай 3: (z+1)^2 / y
    pop ecx  ; z
    pop ebx  ; y
    pop eax  ; x
    
    ; Проверяем деление на ноль
    test ebx, ebx
    jz .division_by_zero
    
    ; Вычисляем (z+1)^2
    mov eax, ecx    ; z
    add eax, 1      ; z+1
    imul eax, eax   ; (z+1)^2
    
    ; Выполняем деление
    cdq             ; расширяем eax до edx:eax
    idiv ebx        ; (z+1)^2 / y (знаковое деление)
    
    mov [result], eax
    jmp .end
    
.division_by_zero:
    mov dword [result], 0
    jmp .end
    
.default_case:
    ; Если ни одно условие не выполняется
    pop ecx
    pop ebx
    pop eax
    mov dword [result], 0
    
.end:
    ; Восстанавливаем регистры
    pop edi
    pop esi
    pop edx
    pop ecx
    pop ebx
    pop eax
    ret