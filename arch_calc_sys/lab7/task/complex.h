#ifndef COMPLEX_H
#define COMPLEX_H

#include <iostream>

class Complex {
public:
    int re, im;
    
    Complex(int r = 0, int i = 0) : re(r), im(i) {}
    
    Complex sum(const Complex& other) const;
    
    void print() const {
        std::cout << re << " + " << im << "i";
    }
};

#endif