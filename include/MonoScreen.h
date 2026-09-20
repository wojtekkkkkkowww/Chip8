#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <ostream>
#include <stdexcept>

class MonoScreen {
public:
  static const std::uint8_t WIDTH = 64;
  static const std::uint8_t HEIGHT = 32;

private:
  std::array<std::uint8_t, WIDTH * HEIGHT> screen;

public:
  MonoScreen() {
    for (auto &pixel : screen) {
      pixel = 0;
    }
  }

  void set(std::size_t x, std::size_t y) {
    if (x >= WIDTH or y >= HEIGHT) {
      throw std::out_of_range("screen coordinates out of bounds");
    }
    screen[x + y * WIDTH] = 1;
  }

  void reset(std::size_t x, std::size_t y) {
    if (x >= WIDTH or y >= HEIGHT) {
      throw std::out_of_range("screen coordinates out of bounds");
    }
    screen[x + y * WIDTH] = 0;
  }

  std::uint8_t get(std::size_t x, std::size_t y) const {
    if (x >= WIDTH or y >= HEIGHT) {
      throw std::out_of_range("screen coordinates out of bounds");
    }
    return screen[x + y * WIDTH];
  }
};

std::ostream &operator<<(std::ostream &output, const MonoScreen &screen) {
  // Write the screen to the stream.
  for (std::size_t y = 0; y < MonoScreen::HEIGHT; y++) {
    for (std::size_t x = 0; x < MonoScreen::WIDTH; x++) {
      output << ((screen.get(x, y) == 0) ? "O" : "#");
    }
    output << '\n';
  }
  return output;
}
