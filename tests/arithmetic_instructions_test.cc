#include "ChipData.h"
#include "Decoder.h"
#include <array>
#include <cstdint>
#include <gtest/gtest.h>
#include <string>
#include <string_view>

namespace {

struct TwoAddressTestCase {
    std::string_view name;
    uint16_t opcode;
    uint8_t x;
    uint8_t y;
    uint8_t initialFlag;
    uint8_t expectedX;
    uint8_t expectedFlag;
};

class TwoAddressInstructionTest
    : public testing::TestWithParam<TwoAddressTestCase> {};

TEST_P(TwoAddressInstructionTest, ExecuteCommand) {
    const auto &testCase = GetParam();
    ChipData data{};
    Decoder decoder;
    data.commonRegisters[1] = testCase.x;
    data.commonRegisters[2] = testCase.y;
    data.commonRegisters[0xF] = testCase.initialFlag;

    auto command = decoder.decode(testCase.opcode);
    command.execute(data);

    ASSERT_EQ(data.commonRegisters[1], testCase.expectedX);
    ASSERT_EQ(data.commonRegisters[2], testCase.y);
    ASSERT_EQ(data.commonRegisters[0xF], testCase.expectedFlag);
}

constexpr auto twoAddressCases = std::to_array<TwoAddressTestCase>({
    {"Copy", 0x8120, 0xAA, 0x55, 1, 0x55, 1},
    {"Or", 0x8121, 0xA5, 0x3C, 1, 0xBD, 0},
    {"And", 0x8122, 0xA5, 0x3C, 1, 0x24, 0},
    {"Xor", 0x8123, 0xA5, 0x3C, 1, 0x99, 0},
    {"AddWithoutCarry", 0x8124, 3, 4, 1, 7, 0},
    {"AddAtByteLimit", 0x8124, 0xFE, 1, 1, 0xFF, 0},
    {"AddWithCarry", 0x8124, 0xFF, 1, 0, 0, 1},
    {"SubtractWithBorrow", 0x8125, 3, 4, 1, 0xFF, 0},
    {"SubtractWithoutBorrow", 0x8125, 4, 3, 0, 1, 1},
    {"SubtractEqualValues", 0x8125, 4, 4, 0, 0, 1},
    {"ShiftRightWithLowBitClear", 0x8126, 0xFF, 4, 1, 2, 0},
    {"ShiftRightWithLowBitSet", 0x8126, 0, 5, 0, 2, 1},
    {"ReverseSubtractWithoutBorrow", 0x8127, 3, 4, 0, 1, 1},
    {"ReverseSubtractWithBorrow", 0x8127, 4, 3, 1, 0xFF, 0},
    {"ReverseSubtractEqualValues", 0x8127, 4, 4, 0, 0, 1},
    {"ShiftLeftWithHighBitClear", 0x812E, 0xFF, 0x41, 1, 0x82, 0},
    {"ShiftLeftWithHighBitSet", 0x812E, 0, 0x81, 0, 2, 1},
});

INSTANTIATE_TEST_SUITE_P(
    MachineCode, TwoAddressInstructionTest, testing::ValuesIn(twoAddressCases),
    [](const auto &info) {
        return std::string(info.param.name);
    });

} // namespace
