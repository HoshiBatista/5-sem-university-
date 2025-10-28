section .data
    
    matrix1 dd 1, 2, 3, 4      
            dd 2, 1, 4, 5       
            dd 3, 4, 1, 5       
            dd 4, 5, 5, 5

    matrix2 dd 1, 2, 3      
            dd 4, 5, 6    
            dd 7, 8, 9     
    
    matrix3 dd 5, 7          
            dd 7, 3          

    size1 dd 4
    size2 dd 3  
    size3 dd 2

    yes_msg db "Да", 10, 0   
    no_msg db "Нет", 10, 0    
    test_msg db "Матрица %d: ", 0 

section .text
    global main
    extern printf

main:

    push dword [size1]     
    push matrix1           
    push 1               
    call test_matrix     
    add esp, 12            


    push dword [size2]     
    push matrix2           
    push 2                 
    call test_matrix       
    add esp, 12            

    push dword [size3]     
    push matrix3           
    push 3                 
    call test_matrix       
    add esp, 12            

    mov eax, 1            
    xor ebx, ebx          
    int 0x80              


test_matrix:
    push ebp
    mov ebp, esp
    
    push dword [ebp+8]    
    push test_msg         
    call printf
    add esp, 8            
    
    push dword [ebp+16]   
    push dword [ebp+12]   
    call check_symmetric
    add esp, 8            
    
    test eax, eax         
    jz .print_no          

    push yes_msg
    call printf
    add esp, 4
    jmp .end
    
.print_no:
    push no_msg
    call printf
    add esp, 4
    
.end:
    pop ebp
    ret

check_symmetric:
    push ebp
    mov ebp, esp
    
    push ebx
    push ecx
    push edx
    push esi
    push edi
    
    mov esi, [ebp+8]      
    mov ecx, [ebp+12]     
    
    
    mov ebx, 0          
    
.outer_loop:             
    cmp ebx, ecx        
    jge .symmetric        
    
    mov edx, ebx         
    inc edx             
    
.inner_loop:
    cmp edx, ecx         
    jge .next_outer       
    
    mov eax, ebx         
    imul eax, ecx       
    add eax, edx        
    shl eax, 2           
    mov eax, [esi + eax] 
    
    mov edi, edx          
    imul edi, ecx        
    add edi, ebx          
    shl edi, 2            
    mov edi, [esi + edi]  
    
    
    cmp eax, edi
    jne .not_symmetric    
    
    inc edx               
    jmp .inner_loop

.next_outer:
    inc ebx               
    jmp .outer_loop

.not_symmetric:
    mov eax, 0            
    jmp .end_check

.symmetric:
    mov eax, 1            

.end_check:

    pop edi
    pop esi
    pop edx
    pop ecx
    pop ebx
    
    pop ebp
    ret