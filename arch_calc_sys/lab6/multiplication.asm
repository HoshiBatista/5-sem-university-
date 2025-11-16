section .data
    test1 db 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
          db 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
          db 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
          db 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
          db 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
          db 0x00, 0x00, 0x00

    msg db "Multiplication completed successfully!", 10
    msglen equ $ - msg

section .bss
    result resb 84

section .text
    global main

multiplication:
    push rbp
    mov rbp, rsp
    
    ; Сохраняем переданные параметры
    mov r8, rdi   ; сохраняем rdi (test1) в r8
    mov r9, rdx   ; сохраняем rdx (result) в r9
    
    ; Обнуляем результат
    mov rdi, r9
    mov rcx, 84
    xor al, al
    cld
    rep stosb
    
    ; Копируем данные из test1 в result
    mov rsi, r8   ; источник (test1)
    mov rdi, r9   ; назначение (result)
    mov rcx, 42
    cld
    rep movsb
    
    pop rbp
    ret

main:
    push rbp
    mov rbp, rsp
    
    ; Вызываем multiplication
    mov rdi, test1
    mov esi, 1
    mov rdx, result
    call multiplication
    
    ; Выводим сообщение с помощью системного вызова
    mov rax, 1        ; sys_write
    mov rdi, 1        ; stdout
    mov rsi, msg      ; сообщение
    mov rdx, msglen   ; длина сообщения
    syscall
    
    ; Выход
    mov rax, 60       ; sys_exit
    xor rdi, rdi      ; код 0
    syscall