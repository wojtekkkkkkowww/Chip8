#pragma once
#include <array>

class MonoScreen {
  public:
    static const uint8_t WIDTH = 64;
    static const uint8_t HIGTH = 32;

  private:
    std::array<uint8_t, WIDTH * WIDTH> screen;

  public:
    MonoScreen() {
        for(auto &pixel : screen) {
            pixel = 0;
        }
    }

    void set(size_t i, size_t j) {
        if(i >= WIDTH or j >= WIDTH) {
            throw std::out_of_range("screen coordinates out of bands");
        }
        screen[i + j * WIDTH] = 1;
    }

    void reset(size_t i, size_t j) {
        if(i >= WIDTH or j >= WIDTH) {
            throw std::out_of_range("screen coordinates out of bands");
        }
        screen[i + j * WIDTH] = 0;
    }

    uint8_t get(size_t i, size_t j) {
        if(i >= WIDTH or j >= WIDTH) {
            throw std::out_of_range("screen coordinates out of bands");
        }
        return screen[i + j * WIDTH];
    }

    /* skad wiediec gdzie jest poczatek sprita??
    void drawSprite() {
    }
    */
};
