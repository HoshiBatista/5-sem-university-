section .text
global sort_stdcall
global sort_cdecl
global sort_fastcall

; Соглашение о вызовах System V AMD64 для Linux (64-bit):
; rdi = float* a
; rsi = int length
; rdx = float* pos_res
; rcx = int* pos_count
; r8  = float* neg_res
; r9  = int* neg_count
; Возврат: rax = neg_count

sort_stdcall:
    ; Сохраняем регистры, которые должны сохраняться вызываемой функцией
    push rbp
    mov rbp, rsp
    push rbx
    push r12
    push r13
    push r14
    push r15
    
    ; Сохраняем аргументы в callee-saved регистрах
    mov r12, rdi        ; r12 = a
    mov r13, rsi        ; r13 = length
    mov r14, rdx        ; r14 = pos_res
    mov r15, rcx        ; r15 = pos_count
    mov rbx, r8         ; rbx = neg_res
    ; r9 уже содержит neg_count
    
    ; Инициализация счетчиков
    xor r10, r10        ; pos_count = 0
    xor r11, r11        ; neg_count = 0
    
    ; Проверяем нулевую длину
    test r13, r13
    jz .skip_processing
    
    ; Проход по исходному массиву
    xor rcx, rcx        ; i = 0
    
.process_loop:
    ; Загружаем элемент a[i]
    movss xmm0, [r12 + rcx*4]
    
    ; Сравниваем с нулем
    xorps xmm1, xmm1    ; xmm1 = 0.0
    comiss xmm0, xmm1
    
    ; Если равно нулю, пропускаем
    je .next_element
    
    ; Если меньше нуля - отрицательное число
    jb .negative_num
    
.positive_num:
    ; Сохраняем в pos_res
    mov rax, r10        ; rax = pos_count
    movss [r14 + rax*4], xmm0
    inc r10             ; pos_count++
    jmp .next_element
    
.negative_num:
    ; Сохраняем в neg_res
    mov rax, r11        ; rax = neg_count
    movss [rbx + rax*4], xmm0
    inc r11             ; neg_count++
    
.next_element:
    inc rcx
    cmp rcx, r13
    jl .process_loop
    
.skip_processing:
    ; Записываем результаты в выходные параметры
    mov [r15], r10d     ; *pos_count = pos_count
    mov [r9], r11d      ; *neg_count = neg_count
    
    ; Сортируем положительные числа по неубыванию
    test r10, r10
    jz .skip_pos_sort
    
    ; Подготовка аргументов для selection_sort
    mov rdi, r14        ; array = pos_res
    mov rsi, r10        ; count = pos_count
    xor rdx, rdx        ; ascending = 0 (для sort по неубыванию)
    call selection_sort
    
.skip_pos_sort:
    ; Сортируем отрицательные числа по невозрастанию
    test r11, r11
    jz .skip_neg_sort
    
    ; Подготовка аргументов для selection_sort
    mov rdi, rbx        ; array = neg_res
    mov rsi, r11        ; count = neg_count
    mov rdx, 1          ; descending = 1 (для sort по невозрастанию)
    call selection_sort
    
.skip_neg_sort:
    ; Возвращаем neg_count
    mov rax, r11
    
    ; Восстанавливаем регистры
    pop r15
    pop r14
    pop r13
    pop r12
    pop rbx
    pop rbp
    ret

; void selection_sort(float* array, int count, int sort_order)
; sort_order: 0 = ascending (неубывание), 1 = descending (невозрастание)
selection_sort:
    push rbp
    mov rbp, rsp
    
    ; Сохраняем регистры
    push rbx
    push r12
    push r13
    push r14
    push r15
    
    ; rdi = array, rsi = count, rdx = sort_order
    
    ; Если count <= 1, выходим
    cmp rsi, 1
    jle .done
    
    mov r12, rdi        ; r12 = array
    mov r13, rsi        ; r13 = count
    mov r14, rdx        ; r14 = sort_order
    
    ; Внешний цикл (i от 0 до count-2)
    xor r15, r15        ; i = 0
    mov rcx, r13
    dec rcx             ; rcx = count - 1
    
.outer_loop:
    ; Индекс экстремального элемента
    mov rbx, r15        ; rbx = min_index/max_index
    
    ; Внутренний цикл (j от i+1 до count-1)
    mov rdx, r15
    inc rdx             ; j = i + 1
    
.inner_loop:
    cmp rdx, r13        ; j < count
    jge .end_inner
    
    ; Сравниваем array[j] и array[rbx]
    movss xmm0, [r12 + rdx*4]   ; xmm0 = array[j]
    movss xmm1, [r12 + rbx*4]   ; xmm1 = array[rbx]
    
    ; В зависимости от режима сортировки
    test r14, r14
    jz .ascending
    
    ; Невозрастание: ищем максимальный элемент (для отрицательных чисел это ближайшее к нулю)
    comiss xmm0, xmm1
    jbe .no_change
    mov rbx, rdx        ; обновляем max_index
    jmp .no_change
    
.ascending:
    ; Неубывание: ищем минимальный элемент
    comiss xmm0, xmm1
    jae .no_change
    mov rbx, rdx        ; обновляем min_index
    
.no_change:
    inc rdx
    jmp .inner_loop
    
.end_inner:
    ; Если нашли другой экстремальный элемент, меняем местами
    cmp rbx, r15
    je .no_swap
    
    ; Меняем местами array[i] и array[rbx]
    movss xmm0, [r12 + r15*4]   ; xmm0 = array[i]
    movss xmm1, [r12 + rbx*4]   ; xmm1 = array[rbx]
    
    movss [r12 + r15*4], xmm1   ; array[i] = array[rbx]
    movss [r12 + rbx*4], xmm0   ; array[rbx] = array[i]
    
.no_swap:
    inc r15
    cmp r15, rcx
    jl .outer_loop
    
.done:
    ; Восстанавливаем регистры
    pop r15
    pop r14
    pop r13
    pop r12
    pop rbx
    pop rbp
    ret

; В 64-bit Linux все соглашения о вызовах совместимы
sort_cdecl:
    jmp sort_stdcall

sort_fastcall:
    jmp sort_stdcall