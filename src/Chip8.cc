#include "Chip8.h"
#include "Font.h"
#include <algorithm>
#include <stdexcept>

Chip8::Chip8() {
    reset();
}

void Chip8::loadProgram(std::span<const uint8_t> rom) {
    if(rom.size() > data.ram.size() - PROGRAM_START)
        throw std::length_error("ROM exceeds available RAM");
    reset();

    std::copy(rom.begin(), rom.end(), data.ram.begin() + PROGRAM_START);
    data.programCounter = PROGRAM_START;
}

void Chip8::reset() {
    data = ChipData{};
    Font::loadInto(data.ram);
    data.programCounter = PROGRAM_START;
}

void Chip8::tickTimers() {
    if(data.timers.delay > 0)
        --data.timers.delay;
    if(data.timers.sound > 0)
        --data.timers.sound;
}

void Chip8::makeStep() {
    const auto opcode = static_cast<uint16_t>(
        (data.ram.at(data.programCounter) << 8) |
        data.ram.at(data.programCounter + 1));
    data.programCounter += 2; // Advance PC before executing the instruction.

    auto command = decoder.decode(opcode);
    command.execute(data);
}

void Chip8::setKey(uint8_t keyHex, bool pressed) {
    auto &keyState = data.keyboard.keys.at(keyHex);
    if(keyState == 1 and not pressed) {
        if(data.keyboard.waitingRegister.has_value() and not data.keyboard.releasedKey.has_value())
            data.keyboard.releasedKey = keyHex;
    }
    keyState = static_cast<uint8_t>(pressed);
}

const ChipData &Chip8::getData() const { return data; }
