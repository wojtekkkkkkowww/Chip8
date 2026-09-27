#pragma once

#include "MonoScreen.h"
#include <array>
#include <cstdint>
#include <random>

struct ChipData {
    std::array<uint8_t, 4096> ram{};
    std::array<uint8_t, 16> commonRegisters{};
    std::array<uint8_t, 16> keyboard{};
    std::array<uint16_t, 16> stack{};
    uint8_t stackPointer{}; // First free stack entry.
    uint16_t indexRegister{};
    uint16_t programCounter{};
    uint8_t delayTimer{};
    uint8_t soundTimer{};
    std::mt19937 randomGenerator{std::random_device{}()};
    MonoScreen screen;
};
