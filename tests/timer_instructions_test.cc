#include "ChipData.h"
#include "Decoder.h"
#include <gtest/gtest.h>

namespace {

TEST(MachineCode, SetDelayTimer) {
    Decoder decoder;
    ChipData data;

    data.commonRegisters[3] = 60;
    auto setDelayTimer = decoder.decode(0xF315);
    setDelayTimer.execute(data);
    ASSERT_EQ(data.timers.delay, 60);
    ASSERT_EQ(data.commonRegisters[3], 60);
    ASSERT_EQ(data.timers.sound, 0);
}

TEST(MachineCode, ReadDelayTimer) {
    Decoder decoder;
    ChipData data;
    data.timers.delay = 60;

    auto readDelayTimer = decoder.decode(0xF407);
    readDelayTimer.execute(data);
    ASSERT_EQ(data.commonRegisters[4], 60);
    ASSERT_EQ(data.timers.delay, 60);
}

TEST(MachineCode, SetSoundTimer) {
    Decoder decoder;
    ChipData data;
    data.timers.delay = 60;

    data.commonRegisters[5] = 30;
    auto setSoundTimer = decoder.decode(0xF518);
    setSoundTimer.execute(data);
    ASSERT_EQ(data.timers.sound, 30);
    ASSERT_EQ(data.commonRegisters[5], 30);
    ASSERT_EQ(data.timers.delay, 60);
}

} // namespace
