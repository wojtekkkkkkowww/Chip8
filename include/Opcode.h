#pragma once

#include <cstdint>

namespace Opcode {
static constexpr uint16_t FFF(uint16_t opcode) { return opcode & 0x0FFF; }
static constexpr uint8_t FF(uint16_t opcode) {
    return static_cast<uint8_t>(opcode & 0x00FF);
}
static constexpr uint8_t F(uint16_t opcode) {
    return static_cast<uint8_t>(opcode & 0x000F);
}
// >> has higher precedence than &
// static constexpr uint8_t F000(uint16_t opcode) {
//    return static_cast<uint8_t>((opcode & 0xF000) >> 12);
//}
static constexpr uint8_t F00(uint16_t opcode) {
    return static_cast<uint8_t>((opcode & 0x0F00) >> 8);
}
static constexpr uint8_t F0(uint16_t opcode) {
    return static_cast<uint8_t>((opcode & 0x00F0) >> 4);
}
} // namespace Opcode
