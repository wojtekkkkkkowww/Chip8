#pragma once

#include "ChipData.h"
#include "Decoder.h"
#include <cstdint>
#include <span>

class Chip8 {
  private:
    Decoder decoder;
    ChipData data;
    static constexpr uint16_t PROGRAM_START = 0x200;

  public:
    void loadToRam(std::span<const uint8_t> rom);
    void makeStep();
    void setKey(uint8_t keyHex, bool pressed);
    const ChipData &getData() const;
};
