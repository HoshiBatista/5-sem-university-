CXX = g++
ASM = nasm
CXX_FLAGS = -O2 -std=c++11
ASM_FLAGS = -f elf64
EXE = complex_test
OBJS = main.o complex.o complex_asm.o

all: $(EXE)

$(EXE): $(OBJS)
	$(CXX) -o $(EXE) $(OBJS)

main.o: main.cpp complex.h
	$(CXX) $(CXX_FLAGS) -c main.cpp

complex.o: complex.cpp complex.h
	$(CXX) $(CXX_FLAGS) -c complex.cpp

complex_asm.o: complex_asm.asm
	$(ASM) $(ASM_FLAGS) -o complex_asm.o complex_asm.asm

clean:
	rm -f *.o $(EXE)

run: $(EXE)
	./$(EXE)

.PHONY: all clean run