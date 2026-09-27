#include "ChipData.h"
#include "Decoder.h"
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

} // namespace
