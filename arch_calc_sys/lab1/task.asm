section .data
    ; Переменные из задания
    strd db 5,5,5,0,7,7,7,7
    h    dw -1, -2, -3
    w    dq 0x100000, 100000
    s    dd 1.0, -1.0

    ; Строка формата для вывода
    fmt db "%s",9,"%p",9,"%p",9,"%d",10, "%s",9,"%p",9,"%p",9,"%d",10, "%s",9,"%p",9,"%p",9,"%d",10, "%s",9,"%p",9,"%p",9,"%d",10,0  ; 9 - табуляция, 10 - новая строка, 0 - конец строки

    ; Имена переменных
    strd_name db "strd",0
    h_name    db "h",0
    w_name    db "w",0
    s_name    db "s",0

section .text
    global main
    extern printf

main:

    ; Вывод информации о s
    push 8                 ; Размер s
    push s+7               ; Конечный адрес s
    push s                 ; Начальный адрес s
    push s_name            ; Имя переменной

        ; Вывод информации о w
    push 16                ; Размер w
    push w+15              ; Конечный адрес w
    push w                 ; Начальный адрес w
    push w_name            ; Имя переменной

        ; Вывод информации о h
    push 6                 ; Размер h
    push h+5               ; Конечный адрес h
    push h                 ; Начальный адрес h
    push h_name            ; Имя переменной

    ; Вывод информации о strd
    push 8                 ; Размер strd
    push strd+7            ; Конечный адрес strd
    push strd              ; Начальный адрес strd
    push strd_name         ; Имя переменной


    push fmt               ; Строка формата

    call printf
    add esp, 4*17            ; Очистка стека

    ; Завершение программы
    mov eax, 1
    mov ebx, 0
    int 0x80