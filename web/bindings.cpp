#include "Chip8.h"
#include <emscripten/bind.h>
#include <emscripten/val.h>
#include <span>
#include <cstdint>

namespace {
void loadRom(Chip8 &chip, const emscripten::val &bytes) {
    const auto rom = emscripten::convertJSArrayToNumberVector<uint8_t>(bytes);
    chip.loadProgram(std::span<const uint8_t>{rom});
}

void runCycles(Chip8 &chip, unsigned int count) {
    for(unsigned int cycle = 0; cycle < count; ++cycle) chip.makeStep();
}

emscripten::val screenPixels(const Chip8 &chip) {
    const auto &pixels = chip.getData().screen.getPixels();
    return emscripten::val(emscripten::typed_memory_view(pixels.size(), pixels.data()));
}
} // namespace

EMSCRIPTEN_BINDINGS(chip8_module) {
    emscripten::class_<Chip8>("Chip8")
        .constructor<>()
        .function("loadRom", &loadRom)
        .function("makeStep", &Chip8::makeStep)
        .function("runCycles", &runCycles)
        .function("tickTimers", &Chip8::tickTimers)
        .function("setKey", &Chip8::setKey)
        .function("screenPixels", &screenPixels);
}
