#include "complex.h"

extern "C" void complex_sum_asm(Complex* result, const Complex* a, const Complex* b);

Complex Complex::sum(const Complex& other) const {
    Complex result;
    complex_sum_asm(&result, this, &other);
    return result;
}