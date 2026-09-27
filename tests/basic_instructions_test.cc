#include "ChipData.h"
#include "Decoder.h"
#include "Font.h"
#include <gtest/gtest.h>

namespace {

TEST(MachineCode, ExecuteCommand) {
    Decoder decoder;
    ChipData data;

    data.screen.set(0, 0);
    auto clearScreen = decoder.decode(0x00E0);
    clearScreen.execute(data);
    ASSERT_EQ(data.screen.get(0, 0), 0);

    auto jumpToAddress = decoder.decode(0x1111);
    jumpToAddress.execute(data);
    ASSERT_EQ(data.programCounter, 0x0111);

    data.commonRegisters[0] = 0x23;
    auto jumpWithOffset = decoder.decode(0xB123);
    jumpWithOffset.execute(data);
    ASSERT_EQ(data.programCounter, 0x0146);
    ASSERT_EQ(data.commonRegisters[0], 0x23);

    auto saveToRegister = decoder.decode(0x6111);
    saveToRegister.execute(data);
    ASSERT_EQ(data.commonRegisters[0x01], 0x0011);

    auto addToRegister = decoder.decode(0x7111);
    addToRegister.execute(data);
    ASSERT_EQ(data.commonRegisters[0x01], 0x0022);

    auto saveToIndexRegister = decoder.decode(0xA111);
    saveToIndexRegister.execute(data);
    ASSERT_EQ(data.indexRegister, 0x0111);

    auto addToIndexRegister = decoder.decode(0xF11E);
    addToIndexRegister.execute(data);
    ASSERT_EQ(data.indexRegister, 0x0133);
    ASSERT_EQ(data.commonRegisters[0x01], 0x0022);

    data.commonRegisters[3] = 60;
    auto setDelayTimer = decoder.decode(0xF315);
    setDelayTimer.execute(data);
    ASSERT_EQ(data.delayTimer, 60);
    ASSERT_EQ(data.commonRegisters[3], 60);
    ASSERT_EQ(data.soundTimer, 0);

    auto readDelayTimer = decoder.decode(0xF407);
    readDelayTimer.execute(data);
    ASSERT_EQ(data.commonRegisters[4], 60);
    ASSERT_EQ(data.delayTimer, 60);

    data.commonRegisters[5] = 30;
    auto setSoundTimer = decoder.decode(0xF518);
    setSoundTimer.execute(data);
    ASSERT_EQ(data.soundTimer, 30);
    ASSERT_EQ(data.commonRegisters[5], 30);
    ASSERT_EQ(data.delayTimer, 60);
}

TEST(MachineCode, LoadFontIntoRam) {
    ChipData data;
    data.ram.fill(0xAA);
    Font::loadInto(data.ram);

    EXPECT_EQ(data.ram[0x4F], 0xAA);
    EXPECT_EQ(data.ram[0xA0], 0xAA);
    EXPECT_TRUE(std::equal(Font::DATA.begin(), Font::DATA.end(), data.ram.begin() + 0x50));
}

struct FontAddressTestCase {
    uint8_t digit;
    uint16_t expectedAddress;
};

class FontAddressTest : public testing::TestWithParam<FontAddressTestCase> {};

TEST_P(FontAddressTest, ExecuteCommand) {
    const auto &testCase = GetParam();
    Decoder decoder;
    ChipData data;
    data.commonRegisters[3] = testCase.digit;

    auto command = decoder.decode(0xF329);
    command.execute(data);

    EXPECT_EQ(data.indexRegister, testCase.expectedAddress);
    EXPECT_EQ(data.commonRegisters[3], testCase.digit);
}

INSTANTIATE_TEST_SUITE_P(
    MachineCode, FontAddressTest,
    testing::Values(
        FontAddressTestCase{0x0, 0x50},
        FontAddressTestCase{0xA, 0x82},
        FontAddressTestCase{0xF, 0x9B}));

TEST(MachineCode, RandomInstructionSanityCheck) {
    Decoder decoder;
    ChipData data;
    data.randomGenerator.seed(123);

    std::mt19937 referenceGenerator{123};
    std::uniform_int_distribution<unsigned int> distribution{0, 255};
    const auto expected = distribution(referenceGenerator) & 0x0F;

    auto random = decoder.decode(0xC30F);
    random.execute(data);
    EXPECT_EQ(data.commonRegisters[3], expected);
}

} // namespace
