#pragma once

#include "MonoScreen.h"
#include <array>
#include <cstdint>

struct ChipData {
    std::array<uint8_t, 4096> ram{};
    std::array<uint8_t, 16> commonRegisters{};
    std::array<uint8_t, 16> keyboard{};
    uint16_t indexRegister{};
    uint16_t programCounter{};
    uint8_t delayTimer{};
    uint8_t soundTimer{};
    MonoScreen screen;
};
