section .data
    base dq 0xAAAAAAAAAAAA       ; Базовое число (12 раз 'A')
    digits db 0x3, 0x5, 0x7, 0xF   ; Специальные цифры
    num_str dd 1                    ; Счетчик чисел
    str_num db "%02d: ", 0
    buffer times 13 db 0           ; Буфер для строки (12 символов + 0)
    newline db 10

section .text
    global main
    extern printf

; --- Функция вывода числа с номером ---
outputnum:
    push rdi
    push rsi
    push rax
    push rdx
    mov rdi, str_num              ; формат строки
    mov esi, [num_str]            ; номер числа
    xor rax, rax
    call printf
    pop rdx
    pop rax
    pop rsi
    pop rdi
    ret

; --- Функция преобразования числа в строку ---
output:
    push rdi
    push rsi
    push rdx
    push rcx
    push rbx
    push rax

    ; Преобразуем число в строку из 12 шестнадцатеричных символов
    mov rbx, rax                  ; Сохраняем число в rbx
    lea rdi, [buffer]             ; Адрес буфера
    mov rcx, 12                   ; 12 цифр

.convert_loop:
    ; Получаем текущую цифру (от старшей к младшей)
    mov rax, rbx
    shr rax, 44                   ; Сдвигаем, чтобы получить старшую цифру
    and rax, 0xF                  ; Изолируем 4 бита
    
    cmp al, 10
    jl .digit
    add al, 'A' - 10              ; Буквы 'A'-'F'
    jmp .store
.digit:
    add al, '0'                   ; Цифры '0'-'9'
.store:
    mov [rdi], al                 ; Сохраняем символ в буфер
    inc rdi
    
    ; Сдвигаем число для получения следующей цифры
    shl rbx, 4
    and rbx, 0xFFFFFFFFFFFF       ; Маскируем до 48 бит
    
    loop .convert_loop

    mov byte [rdi], 0             ; Завершающий ноль

    ; Выводим строку
    push rdi
    push rsi
    push rax
    mov rdi, buffer               ; Адрес буфера
    call print_string
    pop rax
    pop rsi
    pop rdi

    pop rax
    pop rbx
    pop rcx
    pop rdx
    pop rsi
    pop rdi
    ret

; --- Функция вывода строки ---
print_string:
    push rdi
    push rsi
    push rdx
    push rax
    push rcx
    
    ; Находим длину строки
    mov rdi, buffer
    xor rcx, rcx
.length_loop:
    cmp byte [rdi + rcx], 0
    je .length_done
    inc rcx
    jmp .length_loop
.length_done:
    
    ; Выводим строку
    mov rax, 1                    ; sys_write
    mov rdi, 1                    ; stdout
    mov rsi, buffer               ; строка
    mov rdx, rcx                  ; длина
    syscall
    
    ; Выводим символ новой строки
    mov rax, 1                    ; sys_write
    mov rdi, 1                    ; stdout
    mov rsi, newline              ; символ новой строки
    mov rdx, 1                    ; длина 1
    syscall
    
    pop rcx
    pop rax
    pop rdx
    pop rsi
    pop rdi
    ret

; --- Основная функция ---
main:
    push r12
    push r13
    push r14
    push r15
    push rbx
    
    mov r12, [base]               ; Загружаем базовое число
    mov r13, 11                   ; Счетчик позиций (11..0) - от старшей к младшей

outer_loop:
    cmp r13, -1
    jle exit_program
    
    mov r14, 0                    ; Индекс специальной цифры

inner_loop:
    cmp r14, 4
    jge next_position
    
    ; Выводим номер числа
    call outputnum
    mov eax, [num_str]
    inc eax
    mov [num_str], eax
    
    ; Создаем число с замененной цифрой
    mov rax, r12                  ; Копируем базовое число
    
    ; Вычисляем позицию для замены (от 11 до 0)
    ; 11 - самая левая цифра, 0 - самая правая
    mov rcx, r13
    imul rcx, 4                   ; Умножаем позицию на 4 (биты на цифру)
    
    ; Создаем маску для очистки позиции
    mov rbx, 0xF
    shl rbx, cl                   ; Сдвигаем маску на нужную позицию
    not rbx
    and rax, rbx                  ; Очищаем позицию
    
    ; Устанавливаем новую цифру
    movzx rdx, byte [digits + r14] ; Загружаем специальную цифру
    shl rdx, cl                   ; Сдвигаем цифру на нужную позицию
    or rax, rdx                   ; Устанавливаем новую цифру
    
    ; Маскируем до 48 бит
    and rax, 0xFFFFFFFFFFFF
    
    ; Выводим число
    call output
    
    inc r14
    jmp inner_loop

next_position:
    dec r13
    jmp outer_loop

exit_program:
    pop rbx
    pop r15
    pop r14
    pop r13
    pop r12
    mov rax, 60                   ; sys_exit
    xor rdi, rdi
    syscall