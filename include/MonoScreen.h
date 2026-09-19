#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <ostream>
#include <stdexcept>

class MonoScreen {
public:
  static const std::uint8_t WIDTH = 64;
  static const std::uint8_t HIGTH = 32;

private:
  std::array<std::uint8_t, WIDTH * HIGTH> screen;

public:
  MonoScreen() {
    for (auto &pixel : screen) {
      pixel = 0;
    }
  }

  void set(std::size_t i, std::size_t j) {
    if (i >= WIDTH or j >= HIGTH) {
      throw std::out_of_range("screen coordinates out of bands");
    }
    screen[i + j * WIDTH] = 1;
  }

  void reset(std::size_t i, std::size_t j) {
    if (i >= WIDTH or j >= HIGTH) {
      throw std::out_of_range("screen coordinates out of bands");
    }
    screen[i + j * WIDTH] = 0;
  }

  std::uint8_t get(std::size_t i, std::size_t j) const {
    if (i >= WIDTH or j >= HIGTH) {
      throw std::out_of_range("screen coordinates out of bands");
    }
    return screen[i + j * WIDTH];
  }
};

std::ostream &operator<<(std::ostream &os, const MonoScreen &obj) {
  // write obj to stream
  for (std::size_t i = 0; i < MonoScreen::HIGTH; i++) {
    for (std::size_t j = 0; j < MonoScreen::WIDTH; j++) {
      os << ((obj.get(j, i) == 0) ? "O" : "#");
    }
    os << '\n';
  }
  return os;
}
