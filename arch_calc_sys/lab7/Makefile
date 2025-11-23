CC = gcc
CXX = g++
ASM = nasm
ASM_FLAGS = -f elf64
CC_FLAGS = -shared -fPIC
CXX_FLAGS = -O2
LIB_NAME = libarrayops.so

all: $(LIB_NAME) test

$(LIB_NAME): array_ops.o
	$(CC) $(CC_FLAGS) -o $(LIB_NAME) array_ops.o

array_ops.o: array_ops.asm
	$(ASM) $(ASM_FLAGS) -o array_ops.o array_ops.asm

test: test.cpp $(LIB_NAME)
	$(CXX) $(CXX_FLAGS) -o test test.cpp -L. -larrayops -Wl,-rpath,.

clean:
	rm -f *.o $(LIB_NAME) test performance_data.csv

.PHONY: all clean