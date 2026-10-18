#include "Chip8.h"
#include <emscripten/bind.h>
#include <emscripten/val.h>
#include <span>

namespace {
void loadProgram(Chip8 &chip, const emscripten::val &bytes) {
    const auto rom = emscripten::convertJSArrayToNumberVector<uint8_t>(bytes);
    chip.loadProgram(std::span<const uint8_t>{rom});
}

unsigned int programCounter(const Chip8 &chip) {
    return chip.getData().programCounter;
}

unsigned int registerV0(const Chip8 &chip) {
    return chip.getData().commonRegisters[0];
}
} // namespace

EMSCRIPTEN_BINDINGS(chip8_module) {
    emscripten::class_<Chip8>("Chip8")
        .constructor<>()
        .function("loadProgram", &loadProgram)
        .function("makeStep", &Chip8::makeStep)
        .function("tickTimers", &Chip8::tickTimers)
        .function("setKey", &Chip8::setKey)
        .function("getProgramCounter", &programCounter)
        .function("getV0", &registerV0);
}
