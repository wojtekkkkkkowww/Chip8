#include "MonoScreen.h"
#include <ostream>
#include <stdexcept>

MonoScreen::MonoScreen() {
    for(auto &pixel : screen) {
        pixel = 0;
    }
}

void MonoScreen::set(std::size_t x, std::size_t y) {
    if(x >= WIDTH or y >= HEIGHT) {
        throw std::out_of_range("screen coordinates out of bounds");
    }
    screen[x + y * WIDTH] = 1;
}

void MonoScreen::reset(std::size_t x, std::size_t y) {
    if(x >= WIDTH or y >= HEIGHT) {
        throw std::out_of_range("screen coordinates out of bounds");
    }
    screen[x + y * WIDTH] = 0;
}

std::uint8_t MonoScreen::get(std::size_t x, std::size_t y) const {
    if(x >= WIDTH or y >= HEIGHT) {
        throw std::out_of_range("screen coordinates out of bounds");
    }
    return screen[x + y * WIDTH];
}

const std::array<std::uint8_t, MonoScreen::WIDTH * MonoScreen::HEIGHT> &MonoScreen::getPixels() const {
    return screen;
}

std::ostream &operator<<(std::ostream &output, const MonoScreen &screen) {
    // Write the screen to the stream.
    for(std::size_t y = 0; y < MonoScreen::HEIGHT; y++) {
        for(std::size_t x = 0; x < MonoScreen::WIDTH; x++) {
            output << ((screen.get(x, y) == 0) ? "O" : "#");
        }
        output << '\n';
    }
    return output;
}
