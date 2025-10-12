section .data
    t db 1, 1, 1           ; Массив t: t1=3, t2=5, t3=2
    x dw 12                 ; Переменная x = 10
    a dw 6                  ; Переменная a = 4
    result_msg db "Result = %d", 10, 0 ; Сообщение для вывода результата

section .text
    global main
    extern printf

main:
    ; Вычисление t1 * (x - a)
    movzx eax, word [x]     ; Загрузка x в EAX (знаковое расширение)
    movzx ebx, word [a]     ; Загрузка a в EBX
    sub eax, ebx            ; EAX = x - a = 6
    movsx ebx, byte [t]     ; Загрузка t1 в EBX (знаковое расширение)
    imul eax, ebx           ; EAX = t1 * (x - a) = 18
    push eax                ; Сохраняем результат на стеке

    ; Вычисление t2 * (x - a)^2 / 2
    movzx eax, word [x]     ; Загрузка x
    movzx ebx, word [a]     ; Загрузка a
    sub eax, ebx            ; EAX = x - a = 6
    imul eax, eax           ; EAX = (x - a)^2 = 36
    movsx ebx, byte [t+1]   ; Загрузка t2 в EBX
    imul eax, ebx           ; EAX = t2 * (x - a)^2 = 180
    mov ecx, 2              ; ECX = 2 (делитель)
    cdq                     ; Расширение EAX в EDX:EAX для деления
    idiv ecx                ; EAX = 180 / 2 = 90
    push eax                ; Сохраняем результат на стеке

    ; Вычисление t3 * (x - a)^3 / 6
    movzx eax, word [x]     ; Загрузка x
    movzx ebx, word [a]     ; Загрузка a
    sub eax, ebx            ; EAX = x - a = 6
    mov ecx, eax            ; ECX = x - a = 6
    imul eax, eax           ; EAX = (x - a)^2 = 36
    imul eax, ecx           ; EAX = (x - a)^3 = 216
    movsx ebx, byte [t+2]   ; Загрузка t3 в EBX
    imul eax, ebx           ; EAX = t3 * (x - a)^3 = 432
    mov ecx, 6              ; ECX = 6 (делитель)
    cdq                     ; Расширение EAX в EDX:EAX
    idiv ecx                ; EAX = 432 / 6 = 72
    push eax                ; Сохраняем результат на стеке

    ; Суммирование всех частей
    pop eax                 ; EAX = 72 (третья часть)
    pop ebx                 ; EBX = 90 (вторая часть)
    add eax, ebx            ; EAX = 72 + 90 = 162
    pop ebx                 ; EBX = 18 (первая часть)
    add eax, ebx            ; EAX = 162 + 18 = 180

    ; Вывод результата
    push eax                ; Передаем результат в printf
    push result_msg         ; Передаем форматную строку
    call printf             ; Вызов printf
    add esp, 8              ; Очистка стека

    ; Завершение программы
    mov eax, 1              ; Системный вызов exit
    xor ebx, ebx            ; Код возврата 0
    int 0x80