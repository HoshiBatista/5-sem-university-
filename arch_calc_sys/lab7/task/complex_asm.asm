section .text
global complex_sum_asm


complex_sum_asm:
    push rbx
    push rdi  
    
    mov eax, [rsi]      
    add eax, [rdx]      
    mov [rdi], eax      
    
    mov ebx, [rsi + 4]  
    add ebx, [rdx + 4]  
    mov [rdi + 4], ebx 
    
    pop rbx
    pop rdi         
    
    ret