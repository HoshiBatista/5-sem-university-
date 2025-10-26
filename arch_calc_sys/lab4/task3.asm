section .data
    ; Таблица переходов 7x10 для основания 10 и делителя 7
    ; Формат: table[old_remainder][digit] = new_remainder
    transition_table db 0, 1, 2, 3, 4, 5, 6, 0, 1, 2   ; r=0
                    db 3, 4, 5, 6, 0, 1, 2, 3, 4, 5   ; r=1
                    db 6, 0, 1, 2, 3, 4, 5, 6, 0, 1   ; r=2
                    db 2, 3, 4, 5, 6, 0, 1, 2, 3, 4   ; r=3
                    db 5, 6, 0, 1, 2, 3, 4, 5, 6, 0   ; r=4
                    db 1, 2, 3, 4, 5, 6, 0, 1, 2, 3   ; r=5
                    db 4, 5, 6, 0, 1, 2, 3, 4, 5, 6   ; r=6

    ; Тестовые числа (строки цифр)
    number1 db "123456789", 0
    number2 db "100", 0
    number3 db "999", 0
    number4 db "7", 0

    ; Сообщения для вывода
    result_msg db "Number: %s, Remainder mod 7: %d", 10, 0
    test_msg db "Test %d:", 10, 0
    newline db 10, 0

section .bss
    remainder resb 1

section .text
    global main
    extern printf

main:
    ; Тест 1: 123456789 mod 7
    push 1
    push test_msg
    call printf
    add esp, 8

    mov esi, number1
    call calculate_remainder
    
    push dword [remainder]
    push number1
    push result_msg
    call printf
    add esp, 12

    push newline
    call printf
    add esp, 4

    ; Тест 2: 100 mod 7
    push 2
    push test_msg
    call printf
    add esp, 8

    mov esi, number2
    call calculate_remainder
    
    push dword [remainder]
    push number2
    push result_msg
    call printf
    add esp, 12

    push newline
    call printf
    add esp, 4

    ; Тест 3: 999 mod 7
    push 3
    push test_msg
    call printf
    add esp, 8

    mov esi, number3
    call calculate_remainder
    
    push dword [remainder]
    push number3
    push result_msg
    call printf
    add esp, 12

    push newline
    call printf
    add esp, 4

    ; Тест 4: 7 mod 7
    push 4
    push test_msg
    call printf
    add esp, 8

    mov esi, number4
    call calculate_remainder
    
    push dword [remainder]
    push number4
    push result_msg
    call printf
    add esp, 12

    ; Завершение программы
    mov eax, 1
    xor ebx, ebx
    int 0x80

calculate_remainder:
    ; Вход: ESI = адрес строки с числом
    ; Выход: remainder = остаток от деления на 7
    
    pusha
    
    ; Инициализация: начинаем с остатка 0
    mov byte [remainder], 0
    
.process_digit:
    ; Читаем следующий символ
    mov al, [esi]
    
    ; Проверяем конец строки
    cmp al, 0
    je .done
    
    ; Преобразуем символ в цифру (вычитаем '0')
    sub al, '0'
    
    ; Получаем текущий остаток
    mov bl, [remainder]
    
    ; Вычисляем индекс в таблице переходов: index = remainder * 10 + digit
    movzx ebx, bl          ; расширяем remainder до 32 бит
    imul ebx, ebx, 10      ; remainder * 10
    movzx eax, al          ; расширяем digit до 32 бит
    add ebx, eax           ; remainder * 10 + digit
    
    ; Получаем новый остаток из таблицы
    mov al, [transition_table + ebx]
    mov [remainder], al
    
    ; Переходим к следующему символу
    inc esi
    jmp .process_digit

.done:
    popa
    ret