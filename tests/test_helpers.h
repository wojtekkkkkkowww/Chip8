#pragma once

#include "MonoScreen.h"
#include <cstddef>
#include <string>

namespace screen_test_helpers {

// Read eight pixels per row. The rectangle must fit within the screen.
inline std::string readSprite(const MonoScreen &screen, std::size_t x,
                              std::size_t y, std::size_t rows) {
    std::string result;
    for(std::size_t row = 0; row < rows; ++row) {
        for(std::size_t column = 0; column < 8; ++column) {
            result += screen.get(x + column, y + row) ? '#' : 'O';
        }
        result += '\n';
    }
    return result;
}

inline bool isScreenEmpty(const MonoScreen &screen) {
    for(std::size_t y = 0; y < MonoScreen::HEIGHT; ++y) {
        for(std::size_t x = 0; x < MonoScreen::WIDTH; ++x) {
            if(screen.get(x, y) != 0)
                return false;
        }
    }
    return true;
}

} // namespace screen_test_helpers
