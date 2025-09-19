#include <iostream>
#include <vector>
#include <string>

// Функция для вывода произвольного вывода v с аннотациями
void print_arbitrary_derivation() {
    std::cout << "=========================================================\n";
    
    std::cout << "   S"
              << " => S1bSa"
              << " => S1bSabSa"
              << " => SbA3abSa"
              << " => Sbb6abSa"
              << " => A3bbabSa"
              << " => aS4bbabSa"
              << " => aA3bbabSa"
              << " => ab6bbabSa"
              << " => abbabS2a"
              << " => abbabS2aa"
              << " => abbabA3aaa"
              << " => abbbab6aaa"
              << " => abbababbaaa\n";
}

// Функция для вывода анализа и левого вывода с аннотациями
void print_analysis_and_left_derivation() {
    std::cout << "\n=========================================================\n";

    std::cout << "-> Линейная скобочная форма дерева вывода:\n";
    std::cout << "   S1(S1(A3(aS4(A3(b6)))bA3(b6)a)bS2(S2(A3(b6))a)a)\n";

    std::cout << "\n-> Последовательность правил для левого вывода:\n";
    std::cout << "   113436362236\n";

    std::cout << "\n-> Левый вывод:\n";
    std::cout << "   S"
              << " => S1bSa"
              << " => S1bSabSa"
              << " => A3bSabSa"
              << " => aS4bSabSa"
              << " => aA3bSabSa"
              << " => ab6bSabSa"
              << " => abA3abSa"
              << " => abb6abSa"
              << " => abbabS2a"
              << " => abbabS2aa"
              << " => abbabA3aaa"
              << " => abbbab6aaa"
              << " => abbababbaaa\n"; 

    std::cout << "=========================================================\n";
}


int main() {
    std::cout << "113634362236\n\n";
    
    print_arbitrary_derivation();
    print_analysis_and_left_derivation();

    return 0;
}