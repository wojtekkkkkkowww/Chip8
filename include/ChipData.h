#pragma once

#include "MonoScreen.h"
#include <array>
#include <cstdint>
#include <optional>
#include <random>

struct KeyboardState {
    std::array<uint8_t, 16> keys{};
    std::optional<uint8_t> releasedKey;
    std::optional<uint8_t> waitingRegister;
};

struct StackState {
    std::array<uint16_t, 16> addresses{};
    uint8_t pointer{}; // First free stack entry.
};

struct TimerState {
    uint8_t delay{};
    uint8_t sound{};
};

struct ChipData {
    std::array<uint8_t, 4096> ram{};
    std::array<uint8_t, 16> commonRegisters{};
    KeyboardState keyboard;
    StackState stack;
    uint16_t indexRegister{};
    uint16_t programCounter{};
    TimerState timers;
    std::mt19937 randomGenerator{std::random_device{}()};
    MonoScreen screen;
};
