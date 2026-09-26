#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <iosfwd>

class MonoScreen {
  public:
    static const std::uint8_t WIDTH = 64;
    static const std::uint8_t HEIGHT = 32;

  private:
    std::array<std::uint8_t, WIDTH * HEIGHT> screen;

  public:
    MonoScreen();

    void set(std::size_t x, std::size_t y);

    void reset(std::size_t x, std::size_t y);

    std::uint8_t get(std::size_t x, std::size_t y) const;
};

std::ostream &operator<<(std::ostream &output, const MonoScreen &screen);
