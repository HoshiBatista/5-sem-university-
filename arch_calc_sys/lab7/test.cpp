#include <iostream>
#include <algorithm>
#include <functional>
#include <cstring>
#include <cmath>

extern "C" {
    int sort_stdcall(float* a, int length, float* pos_res, int* pos_count, float* neg_res, int* neg_count);
    int sort_cdecl(float* a, int length, float* pos_res, int* pos_count, float* neg_res, int* neg_count);
    int sort_fastcall(float* a, int length, float* pos_res, int* pos_count, float* neg_res, int* neg_count);
}

void print_array(const char* name, float* arr, int count) {
    std::cout << name << " (" << count << "): ";
    for (int i = 0; i < count; i++) {
        std::cout << arr[i] << " ";
    }
    std::cout << std::endl;
}

bool compare_arrays(float* a, float* b, int count) {
    for (int i = 0; i < count; i++) {
        if (fabs(a[i] - b[i]) > 1e-6) {
            std::cout << "Mismatch at index " << i << ": " << a[i] << " != " << b[i] << std::endl;
            return false;
        }
    }
    return true;
}

void test_case(const char* test_name, float* input, int length, 
               float* expected_pos, int expected_pos_count,
               float* expected_neg, int expected_neg_count) {
    std::cout << "\n=== " << test_name << " ===" << std::endl;
    
    // Инициализируем массивы нулями чтобы избежать мусора
    float pos_res[100] = {0};
    float neg_res[100] = {0};
    int pos_count = 0, neg_count = 0;
    
    // Запускаем функцию
    int result = sort_stdcall(input, length, pos_res, &pos_count, neg_res, &neg_count);
    
    // Проверяем результаты
    std::cout << "Input: ";
    for (int i = 0; i < length; i++) std::cout << input[i] << " ";
    std::cout << std::endl;
    
    print_array("Expected positive", expected_pos, expected_pos_count);
    print_array("Actual positive", pos_res, pos_count);
    print_array("Expected negative", expected_neg, expected_neg_count);
    print_array("Actual negative", neg_res, neg_count);
    std::cout << "Returned: " << result << ", Expected return: " << expected_neg_count << std::endl;
    
    bool pos_correct = (pos_count == expected_pos_count) && 
                      compare_arrays(pos_res, expected_pos, pos_count);
    bool neg_correct = (neg_count == expected_neg_count) && 
                      compare_arrays(neg_res, expected_neg, neg_count);
    bool return_correct = (result == expected_neg_count);
    
    std::cout << "Positive array: " << (pos_correct ? "PASS" : "FAIL") << std::endl;
    std::cout << "Negative array: " << (neg_correct ? "PASS" : "FAIL") << std::endl;
    std::cout << "Return value: " << (return_correct ? "PASS" : "FAIL") << std::endl;
    
    if (pos_correct && neg_correct && return_correct) {
        std::cout << "✅ TEST PASSED" << std::endl;
    } else {
        std::cout << "❌ TEST FAILED" << std::endl;
    }
}

void test_all_calling_conventions() {
    std::cout << "\n=== Testing All Calling Conventions ===" << std::endl;
    
    float test[] = {1, -2, 3, -4, 5};
    float pos_res[5] = {0};
    float neg_res[5] = {0};
    int pos_count, neg_count;
    
    // Test stdcall
    int result_std = sort_stdcall(test, 5, pos_res, &pos_count, neg_res, &neg_count);
    std::cout << "StdCall: pos_count=" << pos_count << " neg_count=" << neg_count << " return=" << result_std << std::endl;
    print_array("Pos array", pos_res, pos_count);
    print_array("Neg array", neg_res, neg_count);
    
    // Reset arrays
    memset(pos_res, 0, sizeof(pos_res));
    memset(neg_res, 0, sizeof(neg_res));
    
    // Test cdecl
    int result_cdecl = sort_cdecl(test, 5, pos_res, &pos_count, neg_res, &neg_count);
    std::cout << "Cdecl: pos_count=" << pos_count << " neg_count=" << neg_count << " return=" << result_cdecl << std::endl;
    print_array("Pos array", pos_res, pos_count);
    print_array("Neg array", neg_res, neg_count);
    
    // Reset arrays
    memset(pos_res, 0, sizeof(pos_res));
    memset(neg_res, 0, sizeof(neg_res));
    
    // Test fastcall
    int result_fast = sort_fastcall(test, 5, pos_res, &pos_count, neg_res, &neg_count);
    std::cout << "FastCall: pos_count=" << pos_count << " neg_count=" << neg_count << " return=" << result_fast << std::endl;
    print_array("Pos array", pos_res, pos_count);
    print_array("Neg array", neg_res, neg_count);
}

int main() {
    std::cout << "Array Operations Test - Correctness Check" << std::endl;
    std::cout << "=========================================" << std::endl;
    
    // Тест 1: Пример из задания
    float test1[] = {1, 3, 4, -5, 7, -2, -1, 3, 5, -5};
    float expected_pos1[] = {1, 3, 3, 4, 5, 7};  // сортировка по неубыванию
    float expected_neg1[] = {-1, -2, -5, -5};    // сортировка по невозрастанию
    test_case("Test 1 - Example from assignment", test1, 10, 
              expected_pos1, 6, expected_neg1, 4);
    
    // Тест 2: Только положительные числа
    float test2[] = {5, 2, 8, 1, 9};
    float expected_pos2[] = {1, 2, 5, 8, 9};
    float expected_neg2[] = {};
    test_case("Test 2 - Only positive numbers", test2, 5,
              expected_pos2, 5, expected_neg2, 0);
    
    // Тест 3: Только отрицательные числа
    float test3[] = {-3, -1, -4, -2};
    float expected_pos3[] = {};
    float expected_neg3[] = {-1, -2, -3, -4};  // по невозрастанию: -1 > -2 > -3 > -4
    test_case("Test 3 - Only negative numbers", test3, 4,
              expected_pos3, 0, expected_neg3, 4);
    
    // Тест 4: С нулями
    float test4[] = {2, 0, -3, 5, 0, -1};
    float expected_pos4[] = {2, 5};
    float expected_neg4[] = {-1, -3};  // по невозрастанию: -1 > -3
    test_case("Test 4 - With zeros", test4, 6,
              expected_pos4, 2, expected_neg4, 2);
    
    // Тест 5: Один элемент
    float test5[] = {-7};
    float expected_pos5[] = {};
    float expected_neg5[] = {-7};
    test_case("Test 5 - Single element", test5, 1,
              expected_pos5, 0, expected_neg5, 1);
    
    // Тест 6: Пустой массив
    float test6[] = {};
    float expected_pos6[] = {};
    float expected_neg6[] = {};
    test_case("Test 6 - Empty array", test6, 0,
              expected_pos6, 0, expected_neg6, 0);
    
    // Тест 7: Все нули
    float test7[] = {0, 0, 0, 0};
    float expected_pos7[] = {};
    float expected_neg7[] = {};
    test_case("Test 7 - All zeros", test7, 4,
              expected_pos7, 0, expected_neg7, 0);
    
    // Тестирование всех соглашений о вызовах
    test_all_calling_conventions();
    
    std::cout << "\n=== All tests completed ===" << std::endl;
    return 0;
}