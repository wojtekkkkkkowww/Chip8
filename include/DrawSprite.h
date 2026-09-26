#pragma once

#include <cstdint>
#include <optional>

struct ChipData;

class DrawSprite {
  private:
    uint8_t xRegisterIndex{};
    uint8_t yRegisterIndex{};
    uint8_t rowCount{};

    template <uint8_t size>
    std::optional<uint8_t> offsetPosition(uint8_t position, uint8_t offset);

  public:
    DrawSprite(uint16_t command);

    void execute(ChipData &data);
};
