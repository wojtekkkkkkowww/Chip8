#include "ChipData.h"
#include "Decoder.h"
#include <gtest/gtest.h>

namespace {

TEST(MachineCode, ClearScreen) {
    Decoder decoder;
    ChipData data;

    data.screen.set(0, 0);
    auto clearScreen = decoder.decode(0x00E0);
    clearScreen.execute(data);
    ASSERT_EQ(data.screen.get(0, 0), 0);
}

TEST(MachineCode, JumpToAddress) {
    Decoder decoder;
    ChipData data;

    auto jumpToAddress = decoder.decode(0x1111);
    jumpToAddress.execute(data);
    ASSERT_EQ(data.programCounter, 0x0111);
}

TEST(MachineCode, JumpWithOffset) {
    Decoder decoder;
    ChipData data;

    data.commonRegisters[0] = 0x23;
    auto jumpWithOffset = decoder.decode(0xB123);
    jumpWithOffset.execute(data);
    ASSERT_EQ(data.programCounter, 0x0146);
    ASSERT_EQ(data.commonRegisters[0], 0x23);
}

TEST(MachineCode, SaveToRegister) {
    Decoder decoder;
    ChipData data;

    auto saveToRegister = decoder.decode(0x6111);
    saveToRegister.execute(data);
    ASSERT_EQ(data.commonRegisters[0x01], 0x0011);
}

TEST(MachineCode, AddToRegister) {
    Decoder decoder;
    ChipData data;
    data.commonRegisters[1] = 0x11;

    auto addToRegister = decoder.decode(0x7111);
    addToRegister.execute(data);
    ASSERT_EQ(data.commonRegisters[0x01], 0x0022);
}

TEST(MachineCode, SaveToIndexRegister) {
    Decoder decoder;
    ChipData data;

    auto saveToIndexRegister = decoder.decode(0xA111);
    saveToIndexRegister.execute(data);
    ASSERT_EQ(data.indexRegister, 0x0111);
}

TEST(MachineCode, AddToIndexRegister) {
    Decoder decoder;
    ChipData data;
    data.indexRegister = 0x111;
    data.commonRegisters[1] = 0x22;

    auto addToIndexRegister = decoder.decode(0xF11E);
    addToIndexRegister.execute(data);
    ASSERT_EQ(data.indexRegister, 0x0133);
    ASSERT_EQ(data.commonRegisters[0x01], 0x0022);
}

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
