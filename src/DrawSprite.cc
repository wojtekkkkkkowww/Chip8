#include "DrawSprite.h"
#include "ChipData.h"
#include "Config.h"
#include "Opcode.h"
#include <stdexcept>

template <uint8_t size>
std::optional<uint8_t> DrawSprite::offsetPosition(uint8_t position, uint8_t offset) {

    uint8_t nextPosition = position + offset;
    switch(ConfigStorage::getConfig().collisionType) {
    case SpriteCollisionType::Wrapping:
        return nextPosition % size;
    case SpriteCollisionType::Clipping:
        if(nextPosition >= size)
            return std::nullopt;
        return nextPosition;
    }
    throw std::logic_error("Unknown SpriteCollisionType");
}

DrawSprite::DrawSprite(uint16_t command)
    : xRegisterIndex(Opcode::F00(command)), yRegisterIndex(Opcode::F0(command)),
      rowCount(Opcode::F(command)) {}

void DrawSprite::execute(ChipData &data) {

    uint8_t xPosition = data.commonRegisters[xRegisterIndex] % MonoScreen::WIDTH;
    uint8_t yPosition = data.commonRegisters[yRegisterIndex] % MonoScreen::HEIGHT;

    uint8_t collisionRegisterIndex = 0xF;

    data.commonRegisters[collisionRegisterIndex] = 0;

    for(uint8_t i = 0; i < rowCount; i++) {

        uint8_t spriteByte = data.ram.at(data.indexRegister + i);
        auto currentYPosition = offsetPosition<MonoScreen::HEIGHT>(yPosition, i);

        if(not currentYPosition)
            continue;

        for(uint8_t j = 0; j < 8; j++) {

            auto currentXPosition = offsetPosition<MonoScreen::WIDTH>(xPosition, j);

            if(not currentXPosition)
                continue;

            uint8_t color = spriteByte & (0x1 << (7 - j));

            if(color > 0) {
                if(data.screen.get(*currentXPosition, *currentYPosition) == 1) {
                    data.commonRegisters[collisionRegisterIndex] = 1;
                    data.screen.reset(*currentXPosition, *currentYPosition);
                } else {
                    data.screen.set(*currentXPosition, *currentYPosition);
                }
            }
        }
    }
}
