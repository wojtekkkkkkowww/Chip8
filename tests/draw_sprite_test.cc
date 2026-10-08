#include "ChipData.h"
#include "Decoder.h"
#include "Opcode.h"
#include "test_helpers.h"
#include <array>
#include <cstdint>
#include <gtest/gtest.h>
#include <string>
#include <string_view>
#include <vector>

namespace {

struct DrawSpriteTestCase {
    std::string_view name;
    uint16_t opcode;
    std::vector<uint8_t> sprite;
    std::string_view expectedSpriteString;
    uint8_t startingXPosition{};
    uint8_t startingYPosition{};
};
class DrawSpriteTest : public testing::TestWithParam<DrawSpriteTestCase> {};

TEST_P(DrawSpriteTest, DrawSprite) {
    Decoder decoder;
    ChipData data;
    const auto &testCase = GetParam();

    // loading sprite to ram
    data.indexRegister = 0;
    for(size_t index = 0; uint8_t byte : testCase.sprite) {
        data.ram[index++] = byte;
    }

    // setup X and Y
    uint8_t xPosition = testCase.startingXPosition;
    uint8_t yPosition = testCase.startingYPosition;
    data.commonRegisters[Opcode::F00(testCase.opcode)] = xPosition;
    data.commonRegisters[Opcode::F0(testCase.opcode)] = yPosition;

    const auto rowCount = static_cast<uint8_t>(testCase.sprite.size());
    auto drawSprite = decoder.decode(testCase.opcode);
    drawSprite.execute(data);

    const auto fetchedSprite = screen_test_helpers::readSprite(
        data.screen, xPosition, yPosition, rowCount);

    ASSERT_EQ(testCase.expectedSpriteString, fetchedSprite);
    ASSERT_EQ(data.commonRegisters[0xF], 0);

    // clear screen with the same sprite
    drawSprite.execute(data);

    ASSERT_TRUE(screen_test_helpers::isScreenEmpty(data.screen));
    ASSERT_EQ(data.commonRegisters[0xF], 1);
}

const auto drawSpriteCases = std::to_array<DrawSpriteTestCase>({
    {"Alien", 0xD234, {0x66, 0x7E, 0xC3, 0x99}, "O##OO##O\nO######O\n##OOOO##\n#OO##OO#\n", 1, 1},
    {"Smile", 0xD235, {0x42, 0x42, 0x00, 0x81, 0x7E}, "O#OOOO#O\nO#OOOO#O\nOOOOOOOO\n#OOOOOO#\nO######O\n", 20, 10},
});

INSTANTIATE_TEST_SUITE_P(
    MachineCode, DrawSpriteTest, testing::ValuesIn(drawSpriteCases),
    [](const auto &info) { return std::string(info.param.name); });

} // namespace
