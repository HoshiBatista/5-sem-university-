section .data
    ; Тестовые данные - набор 1
    h1 db 2
    x1 db 5, 8, 12, 15, 3
    y1 dw 10, 20, 30, 40, 50
    m1 dw 2
    n1 dw 2
    
    ; Тестовые данные - набор 2
    h2 db -3
    x2 db 7, 9, 11, 13, 6
    y2 dw 5, 15, 25, 35, 45
    m2 dw 2
    n2 dw 2
    
    ; Тестовые данные - набор 3
    h3 db 1
    x3 db 4, 6, 10, 14, 8
    y3 dw 8, 18, 28, 38, 48
    m3 dw 2
    n3 dw 2

    ; Сообщения для вывода
    result_msg db "Result: %d", 10, 0
    test_msg db "Test %d: h=%d, m=%u, n=%u", 10, 0
    newline db 10, 0

section .bss
    result resd 1

section .text
    global main
    extern printf

main:
    ; Инициализация результата
    mov dword [result], 0
    
    ; Тест 1
    movzx eax, word [m1]
    movzx ebx, word [n1]
    movsx ecx, byte [h1]
    
    push ebx
    push eax
    push ecx
    push 1
    push test_msg
    call printf
    add esp, 20
    
    call calculate_test1
    
    push dword [result]
    push result_msg
    call printf
    add esp, 8
    
    push newline
    call printf
    add esp, 4
    
    ; Сброс результата для следующего теста
    mov dword [result], 0
    
    ; Тест 2
    movzx eax, word [m2]
    movzx ebx, word [n2]
    movsx ecx, byte [h2]
    
    push ebx
    push eax
    push ecx
    push 2
    push test_msg
    call printf
    add esp, 20
    
    call calculate_test2
    
    push dword [result]
    push result_msg
    call printf
    add esp, 8
    
    push newline
    call printf
    add esp, 4
    
    ; Сброс результата для следующего теста
    mov dword [result], 0
    
    ; Тест 3
    movzx eax, word [m3]
    movzx ebx, word [n3]
    movsx ecx, byte [h3]
    
    push ebx
    push eax
    push ecx
    push 3
    push test_msg
    call printf
    add esp, 20
    
    call calculate_test3
    
    push dword [result]
    push result_msg
    call printf
    add esp, 8

    ; Завершение программы
    mov eax, 1
    xor ebx, ebx
    int 0x80

calculate_test1:
    pusha
    mov esi, x1  ; адрес массива x
    mov edi, y1  ; адрес массива y
    movsx eax, byte [h1]
    movzx ebx, word [m1]
    movzx ecx, word [n1]
    call calculate_expression
    popa
    ret

calculate_test2:
    pusha
    mov esi, x2
    mov edi, y2
    movsx eax, byte [h2]
    movzx ebx, word [m2]
    movzx ecx, word [n2]
    call calculate_expression
    popa
    ret

calculate_test3:
    pusha
    mov esi, x3
    mov edi, y3
    movsx eax, byte [h3]
    movzx ebx, word [m3]
    movzx ecx, word [n3]
    call calculate_expression
    popa
    ret

calculate_expression:
    ; Вход: EAX = h, EBX = m, ECX = n, ESI = адрес x, EDI = адрес y
    ; Выход: result = Σ_i=0^m Σ_j=0^n [ (x_i^5 - i*h + y_i) * (h + 17*i) * f(x_i, y_j) ]
    
    push ebp
    mov ebp, esp
    sub esp, 16  ; место для локальных переменных
    
    ; Сохраняем параметры в локальные переменные
    mov [ebp-4], eax   ; h
    mov [ebp-8], ebx   ; m
    mov [ebp-12], ecx  ; n
    mov [ebp-16], esi  ; адрес x
    
    ; Обнуляем результат
    mov dword [result], 0
    
    ; Внешний цикл по i (0..m)
    mov esi, 0           ; i = 0
outer_loop:
    cmp esi, [ebp-8]     ; сравниваем i с m
    jg end_outer_loop
    
    ; Получаем x_i (знаковый байт -> знаковый 32-бит)
    mov edx, [ebp-16]    ; адрес x
    movsx eax, byte [edx + esi]  ; x_i
    
    ; Получаем y_i (беззнаковое слово -> беззнаковое 32-бит)
    movzx ebx, word [edi + esi*2] ; y_i
    
    ; Вычисляем term1 = x_i^5 - i*h + y_i
    ; Сначала x_i^5
    mov ecx, eax         ; сохраняем x_i
    imul eax, eax        ; x_i^2
    imul eax, ecx        ; x_i^3
    imul eax, ecx        ; x_i^4
    imul eax, ecx        ; x_i^5
    
    ; Затем i*h
    mov edx, esi         ; i
    imul edx, [ebp-4]    ; i * h
    
    ; x_i^5 - i*h
    sub eax, edx
    
    ; + y_i
    add eax, ebx
    
    ; Сохраняем term1
    push eax
    
    ; Вычисляем term2 = h + 17*i
    mov eax, [ebp-4]     ; h
    mov ecx, esi         ; i
    imul ecx, 17         ; 17*i
    add eax, ecx         ; h + 17*i
    
    ; Сохраняем term2
    push eax
    
    ; Внутренний цикл по j (0..n)
    mov edi, 0           ; j = 0
inner_loop:
    cmp edi, [ebp-12]    ; сравниваем j с n
    jg end_inner_loop
    
    ; Получаем y_j для вызова функции f(x_i, y_j)
    mov edx, [ebp+8]     ; адрес y (передан через стек)
    movzx eax, word [edx + edi*2] ; y_j
    
    ; Получаем x_i для вызова функции
    mov edx, [ebp-16]    ; адрес x
    movsx ecx, byte [edx + esi]  ; x_i
    
    ; Вызываем функцию f(x_i, y_j)
    push eax             ; y_j
    push ecx             ; x_i
    call f_function
    add esp, 8
    
    ; EAX содержит результат f(x_i, y_j)
    
    ; Достаем term2 и term1
    pop ecx              ; term2
    pop edx              ; term1
    
    ; Вычисляем product = term1 * term2 * f(x_i, y_j)
    imul edx, ecx        ; term1 * term2
    imul edx, eax        ; * f(x_i, y_j)
    
    ; Добавляем к общей сумме
    add [result], edx
    
    ; Восстанавливаем term1 и term2 для следующей итерации j
    push edx             ; сохраняем нашу сумму (временно)
    push ecx             ; term2
    push edx             ; term1 (изначальное)
    
    inc edi
    jmp inner_loop

end_inner_loop:
    ; Очищаем стек от term1 и term2
    add esp, 8
    
    ; Восстанавливаем адрес y для следующей итерации i
    mov edi, [ebp+8]
    
    inc esi
    jmp outer_loop

end_outer_loop:
    mov esp, ebp
    pop ebp
    ret

f_function:
    ; Функция f(x, y)
    ; Вход: [esp+4] = x (знаковое 32-бит), [esp+8] = y (беззнаковое 32-бит)
    ; Выход: EAX = результат
    
    push ebp
    mov ebp, esp
    
    mov eax, [ebp+8]     ; x
    mov ebx, [ebp+12]    ; y
    
    ; Проверяем условие x > 10
    cmp eax, 10
    jg .case1
    
    ; Случай 2: x <= 10, f(x,y) = 1000
    mov eax, 1000
    jmp .end
    
.case1:
    ; Случай 1: x > 10, f(x,y) = x + y
    add eax, ebx         ; x + y
    
.end:
    pop ebp
    ret