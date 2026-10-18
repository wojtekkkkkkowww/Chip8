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
    Chip8();
    void loadProgram(std::span<const uint8_t> rom);
    void makeStep();
    void setKey(uint8_t keyHex, bool pressed);
    void tickTimers(); // Call at 60 Hz, independently of CPU instruction steps.
    const ChipData &getData() const;

  private:
    void reset();
};
