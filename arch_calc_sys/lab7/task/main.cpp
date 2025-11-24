#include <iostream>
#include "complex.h"

void print_test(const char* test_name, const Complex& a, const Complex& b, const Complex& result, const char* expected) {
    std::cout << test_name << ": (";
    a.print();
    std::cout << ") + (";
    b.print();
    std::cout << ") = ";
    result.print();
    std::cout << " | Expected: " << expected << std::endl;
}

void test_complex() {
    std::cout << "=== Testing Complex class with ASM method ===" << std::endl << std::endl;
    
    // Тест 1: Базовый случай
    Complex a1(3, 4);
    Complex b1(2, 5);
    Complex c1 = a1.sum(b1);
    print_test("Test 1", a1, b1, c1, "5 + 9i");
    
    // Тест 2: Отрицательные числа
    Complex a2(10, -3);
    Complex b2(-5, 7);
    Complex c2 = a2.sum(b2);
    print_test("Test 2", a2, b2, c2, "5 + 4i");
    
    // Тест 3: Нулевые значения
    Complex a3(7, 2);
    Complex b3(0, 0);
    Complex c3 = a3.sum(b3);
    print_test("Test 3", a3, b3, c3, "7 + 2i");
}

int main() {
    test_complex();
    return 0;
}