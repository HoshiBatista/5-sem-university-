#include <stdio.h>
#include <math.h>

extern double my_pow(double x, double y);

#define COLOR_RESET   "\033[0m"
#define COLOR_RED     "\033[31m"
#define COLOR_GREEN   "\033[32m"
#define COLOR_YELLOW  "\033[33m"
#define COLOR_BLUE    "\033[34m"
#define COLOR_MAGENTA "\033[35m"
#define COLOR_CYAN    "\033[36m"

void print_colored(const char* color, const char* text) {
    printf("%s%s%s", color, text, COLOR_RESET);
}

void test_pow(double x, double y, const char* description) {
    double my_result = my_pow(x, y);
    double std_result = pow(x, y);
    
    printf("x=%8.3f, y=%8.3f: ", x, y);
    printf("my_pow=%12.6f, std_pow=%12.6f ", my_result, std_result);
    
    int is_nan_my = isnan(my_result);
    int is_nan_std = isnan(std_result);
    int is_inf_my = isinf(my_result);
    int is_inf_std = isinf(std_result);
    
    int passed = 0;
    
    if (is_nan_my && is_nan_std) {
        passed = 1;
    } else if (is_inf_my && is_inf_std && (my_result * std_result > 0)) {
        passed = 1;
    } else if (!is_nan_my && !is_nan_std && !is_inf_my && !is_inf_std &&
               fabs(my_result - std_result) < 1e-6) {
        passed = 1;
    }
    
    if (passed) {
        print_colored(COLOR_GREEN, "✓ PASS");
    } else {
        print_colored(COLOR_RED, "✗ FAIL");
        printf(" %s", description);
    }
    printf("\n");
}

int main() {
    print_colored(COLOR_CYAN, "Testing my_pow function:\n\n");
    
    test_pow(2.0, 3.0, "2^3 = 8");
    test_pow(2.0, -3.0, "2^(-3) = 0.125");
    test_pow(-2.0, 3.0, "(-2)^3 = -8");
    test_pow(-2.0, 4.0, "(-2)^4 = 16");
    test_pow(-2.0, -3.0, "(-2)^(-3) = -0.125");
    test_pow(0.0, 5.0, "0^5 = 0");
    test_pow(5.0, 0.0, "5^0 = 1");
    test_pow(0.0, 0.0, "0^0 = 1");
    test_pow(0.0, -2.0, "0^(-2) = inf");
    test_pow(-4.0, 0.5, "(-4)^0.5 = NaN");
    test_pow(4.0, 0.5, "4^0.5 = 2");
    test_pow(1.5, 2.5, "1.5^2.5 ≈ 2.75568");
    test_pow(10.0, -2.0, "10^(-2) = 0.01");
    
    print_colored(COLOR_CYAN, "\nAdditional tests:\n");
    test_pow(1.0, 100.0, "1^100 = 1");
    test_pow(1.0, -100.0, "1^(-100) = 1");
    test_pow(-1.0, 2.0, "(-1)^2 = 1");
    test_pow(-1.0, 3.0, "(-1)^3 = -1");
    test_pow(8.0, 1.0/3.0, "8^(1/3) = 2");
    test_pow(27.0, 1.0/3.0, "27^(1/3) = 3");
    test_pow(16.0, 0.25, "16^0.25 = 2");
    test_pow(9.0, 0.5, "9^0.5 = 3");
    test_pow(25.0, 0.5, "25^0.5 = 5");
    
    print_colored(COLOR_CYAN, "\nTesting completed!\n");
    
    return 0;
}