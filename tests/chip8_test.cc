#include "Chip8.h"
#include "test_helpers.h"
#include <array>
#include <cstdint>
#include <gtest/gtest.h>

namespace {

TEST(Chip8Integration, WaitForKey) {
    Chip8 chip8;
    constexpr uint8_t keyRegister = 3;
    constexpr uint8_t key = 0xA;
    constexpr std::array<uint8_t, 8> rom{
        0x63, 0xFF, // 0x63FF: Set V3 to 0xFF.
        0xF3, 0x0A, // 0xF30A: Wait for a key release and store the key in V3.
        0xF3, 0x0A, // 0xF30A: Wait for another key release.
        0x73, 0x01  // 0x7301: Add 1 to V3 after waiting finishes.
    };
    chip8.loadToRam(rom);
    chip8.makeStep(); // Set V3 to a sentinel value before waiting for a key.
    const auto &data = chip8.getData();
    const auto waitAddress = data.programCounter;

    chip8.makeStep(); // Wait for a key.
    ASSERT_EQ(data.keyboard.keys[key], 0); // Assert that the key is not pressed.
    ASSERT_EQ(data.programCounter, waitAddress); // Assert that PC has not changed.
    ASSERT_EQ(data.commonRegisters[keyRegister], 0xFF);

    chip8.setKey(key, true);
    chip8.makeStep();
    ASSERT_EQ(data.keyboard.keys[key], 1);
    ASSERT_EQ(data.programCounter, waitAddress);
    ASSERT_EQ(data.commonRegisters[keyRegister], 0xFF);

    chip8.setKey(key, false);
    chip8.makeStep();
    ASSERT_EQ(data.keyboard.keys[key], 0);
    ASSERT_EQ(data.commonRegisters[keyRegister], key);
    ASSERT_EQ(data.programCounter, waitAddress + 2);

    chip8.makeStep(); // The next wait must not reuse the previous release.
    ASSERT_EQ(data.programCounter, waitAddress + 2);

    chip8.setKey(key, true);
    chip8.setKey(key, false);
    chip8.makeStep(); // Complete the second wait with a new release.
    ASSERT_EQ(data.commonRegisters[keyRegister], key);
    ASSERT_EQ(data.programCounter, waitAddress + 4);

    chip8.makeStep(); // Normal execution resumes: add 1 to V3.
    ASSERT_EQ(data.commonRegisters[keyRegister], key + 1);
    ASSERT_EQ(data.programCounter, waitAddress + 6);
}

TEST(Chip8Integration, drawBuiltInFont) {
    Chip8 chip8;
    constexpr std::array<uint8_t, 12> rom{
        0x60, 0x04, // 0x6004: Set V0 to the x coordinate, 4.
        0x61, 0x02, // 0x6102: Set V1 to the y coordinate, 2.
        0x62, 0x0A, // 0x620A: Select the hexadecimal digit A.
        0xF2, 0x29, // 0xF229: Set I to the built-in glyph for V2.
        0xD0, 0x15, // 0xD015: Draw five rows at (V0, V1).
        0x00, 0xE0  // 0x00E0: Clear the screen.
    };
    chip8.loadToRam(rom);
    const auto &data = chip8.getData();
    ASSERT_TRUE(screen_test_helpers::isScreenEmpty(data.screen));

    for(int step = 0; step < 5; ++step)
        chip8.makeStep();

    EXPECT_EQ(screen_test_helpers::readSprite(data.screen, 4, 2, 5),
              "####OOOO\n"
              "#OO#OOOO\n"
              "####OOOO\n"
              "#OO#OOOO\n"
              "#OO#OOOO\n");
    EXPECT_EQ(data.commonRegisters[0xF], 0);
    EXPECT_EQ(data.programCounter, 0x20A);

    chip8.makeStep();
    EXPECT_TRUE(screen_test_helpers::isScreenEmpty(data.screen));
    EXPECT_EQ(data.programCounter, 0x20C);
}

} // namespace
