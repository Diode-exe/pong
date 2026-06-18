#include <iostream>
#include "font8x8_basic.h"

int main() {
    // Test rendering characters
    for (char c = 32; c < 127; c++) {
        std::cout << "Character: " << c << std::endl;
        for (int row = 0; row < 8; row++) {
            for (int col = 0; col < 8; col++) {
                if (font8x8_basic[c][row] & (1 << col)) {
                    std::cout << "#";
                } else {
                    std::cout << " ";
                }
            }
            std::cout << std::endl;
        }
        std::cout << std::endl;
    }
    return 0;
}