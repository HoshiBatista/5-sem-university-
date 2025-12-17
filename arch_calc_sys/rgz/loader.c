#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

// Обновленный шелл-код с NULL-терминатором
unsigned char code[] = \
"\x48\x31\xf6\x48\x31\xd2\x48\x31\xc0\x50\x48\xbb\x2f\x2f\x62\x69\x6e\x2f\x73\x68\x53\x48\x89\xe7\xb0\x3b\x0f\x05";

int main() {
    printf("Shellcode Length: %ld\n", sizeof(code) - 1);

    // Выделяем исполняемую память
    void *executable_mem = mmap(0, sizeof(code), 
                                PROT_READ | PROT_WRITE | PROT_EXEC, 
                                MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

    if (executable_mem == MAP_FAILED) {
        perror("mmap");
        return 1;
    }

    // Копируем код
    memcpy(executable_mem, code, sizeof(code));

    // Запускаем
    void (*ret)() = (void(*)())executable_mem;
    ret();

    return 0;
}