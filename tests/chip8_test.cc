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

TEST(Chip8Integration, NestedCallsReturnToTheirCallers) {
    Chip8 chip8;
    constexpr std::array<uint8_t, 14> rom{
        0x22, 0x04, // 0x200: Call subroutine A at 0x204.
        0x70, 0x01, // 0x202: Main increments V0 after A returns.
        0x22, 0x0A, // 0x204: A calls subroutine B at 0x20A.
        0x71, 0x02, // 0x206: A adds 2 to V1 after B returns.
        0x00, 0xEE, // 0x208: Return from A to main.
        0x72, 0x03, // 0x20A: B adds 3 to V2.
        0x00, 0xEE  // 0x20C: Return from B to A.
    };
    chip8.loadToRam(rom);
    const auto &data = chip8.getData();

    chip8.makeStep(); // Main calls A.
    ASSERT_EQ(data.programCounter, 0x204);
    ASSERT_EQ(data.stack.pointer, 1);
    ASSERT_EQ(data.stack.addresses[0], 0x202);

    chip8.makeStep(); // A calls B.
    ASSERT_EQ(data.programCounter, 0x20A);
    ASSERT_EQ(data.stack.pointer, 2);
    ASSERT_EQ(data.stack.addresses[0], 0x202);
    ASSERT_EQ(data.stack.addresses[1], 0x206);

    chip8.makeStep(); // B updates V2.
    chip8.makeStep(); // B returns to A.
    ASSERT_EQ(data.programCounter, 0x206);
    ASSERT_EQ(data.stack.pointer, 1);
    ASSERT_EQ(data.stack.addresses[0], 0x202);

    chip8.makeStep(); // A updates V1.
    chip8.makeStep(); // A returns to main.
    ASSERT_EQ(data.programCounter, 0x202);
    ASSERT_EQ(data.stack.pointer, 0);

    chip8.makeStep(); // Main updates V0.
    EXPECT_EQ(data.commonRegisters[0], 1);
    EXPECT_EQ(data.commonRegisters[1], 2);
    EXPECT_EQ(data.commonRegisters[2], 3);
    EXPECT_EQ(data.programCounter, 0x204);
    EXPECT_EQ(data.stack.pointer, 0);
}

TEST(Chip8Integration, LoopWithConditionalSkip) {
    // Pseudocode:
    // V0 = 0
    // loop:
    //     V0 += 1
    //     if V0 == 3, skip the next instruction
    //     jump to loop
    // V1 = 0xAA
    Chip8 chip8;
    constexpr std::array<uint8_t, 10> rom{
        0x60, 0x00, // 0x200: Set V0 to 0.
        0x70, 0x01, // 0x202: Increment V0.
        0x30, 0x03, // 0x204: Skip the jump when V0 equals 3.
        0x12, 0x02, // 0x206: Jump back to the increment.
        0x61, 0xAA  // 0x208: Mark that the loop has finished.
    };
    chip8.loadToRam(rom);

    for(int step = 0; step < 10; ++step)
        chip8.makeStep();

    const auto &data = chip8.getData();
    EXPECT_EQ(data.commonRegisters[0], 3);
    EXPECT_EQ(data.commonRegisters[1], 0xAA);
    EXPECT_EQ(data.programCounter, 0x20A);
}

TEST(Chip8Integration, SaveAndReadFromRam) {
    // Pseudocode:
    // V0 = 0x12, V1 = 0x34, V2 = 0x56
    // I = 0x300
    // Store V0-V2 in RAM using Fx55.
    // V0 = 0, V1 = 0, V2 = 0
    // I = 0x300
    // Restore V0-V2 from RAM using Fx65.
    Chip8 chip8;
    constexpr std::array<uint8_t, 20> rom{
        0x60, 0x12, // 0x200: Set V0 to 0x12.
        0x61, 0x34, // 0x202: Set V1 to 0x34.
        0x62, 0x56, // 0x204: Set V2 to 0x56.
        0xA3, 0x00, // 0x206: Set I to 0x300.
        0xF2, 0x55, // 0x208: Store V0-V2 in RAM.
        0x60, 0x00, // 0x20A: Clear V0.
        0x61, 0x00, // 0x20C: Clear V1.
        0x62, 0x00, // 0x20E: Clear V2.
        0xA3, 0x00, // 0x210: Reset I to 0x300.
        0xF2, 0x65  // 0x212: Restore V0-V2 from RAM.
    };
    chip8.loadToRam(rom);
    const auto &data = chip8.getData();

    for(int step = 0; step < 5; ++step)
        chip8.makeStep();

    EXPECT_EQ(data.ram[0x300], 0x12);
    EXPECT_EQ(data.ram[0x301], 0x34);
    EXPECT_EQ(data.ram[0x302], 0x56);

    for(int step = 0; step < 3; ++step)
        chip8.makeStep();

    ASSERT_EQ(data.commonRegisters[0], 0);
    ASSERT_EQ(data.commonRegisters[1], 0);
    ASSERT_EQ(data.commonRegisters[2], 0);
    ASSERT_EQ(data.indexRegister, 0x303); // Fx55 advances I by x + 1 (three bytes).
    chip8.makeStep(); // Reset I.
    chip8.makeStep(); // Restore registers.

    EXPECT_EQ(data.commonRegisters[0], 0x12);
    EXPECT_EQ(data.commonRegisters[1], 0x34);
    EXPECT_EQ(data.commonRegisters[2], 0x56);
    EXPECT_EQ(data.indexRegister, 0x303); // Fx65 also advances I by three bytes.
    EXPECT_EQ(data.programCounter, 0x214);
}

} // namespace
