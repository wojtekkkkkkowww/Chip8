#include "ChipData.h"
#include "Decoder.h"
#include <array>
#include <cstdint>
#include <gtest/gtest.h>
#include <string>
#include <string_view>
#include <variant>

namespace {

struct RegistersData {
    uint8_t x;
    uint8_t y;
};
struct KeyData {
    uint8_t keyIndex;
    bool pressed;
};

struct SkipIfTestCase {
    std::string_view name;
    uint16_t opcode;
    uint16_t expectedProgramCounter;
    std::variant<RegistersData, KeyData> data;
};

class SkipIfTest
    : public testing::TestWithParam<SkipIfTestCase> {};

constexpr auto skipIfCases = std::to_array<SkipIfTestCase>({
    {"EqualValueTrue", 0x3123, 0x202, RegistersData{0x23, 0}},
    {"EqualValueFalse", 0x3123, 0x200, RegistersData{0x24, 0}},
    {"NotEqualValueTrue", 0x4123, 0x202, RegistersData{0x24, 0}},
    {"NotEqualValueFalse", 0x4123, 0x200, RegistersData{0x23, 0}},
    {"EqualRegistersTrue", 0x5120, 0x202, RegistersData{7, 7}},
    {"EqualRegistersFalse", 0x5120, 0x200, RegistersData{7, 8}},
    {"NotEqualRegistersTrue", 0x9120, 0x202, RegistersData{7, 8}},
    {"NotEqualRegistersFalse", 0x9120, 0x200, RegistersData{7, 7}},
    {"KeyPressedTrue", 0xE19E, 0x202, KeyData{0xA, true}},
    {"KeyPressedFalse", 0xE19E, 0x200, KeyData{0xA, false}},
    {"KeyNotPressedTrue", 0xE1A1, 0x202, KeyData{0xA, false}},
    {"KeyNotPressedFalse", 0xE1A1, 0x200, KeyData{0xA, true}},
});

TEST_P(SkipIfTest, ExecuteCommand) {
    const auto &testCase = GetParam();
    ChipData data{};
    Decoder decoder;
    data.programCounter = 0x200;

    if(const auto *registersData = std::get_if<RegistersData>(&testCase.data)) {
        data.commonRegisters[1] = registersData->x;
        data.commonRegisters[2] = registersData->y;
    }
    if(const auto *keyData = std::get_if<KeyData>(&testCase.data)) {
        data.commonRegisters[1] = keyData->keyIndex;
        data.keyboard.keys[keyData->keyIndex] = keyData->pressed;
    }

    auto command = decoder.decode(testCase.opcode);
    command.execute(data);

    ASSERT_EQ(data.programCounter, testCase.expectedProgramCounter);
}

INSTANTIATE_TEST_SUITE_P(
    MachineCode, SkipIfTest, testing::ValuesIn(skipIfCases),
    [](const auto &info) { return std::string(info.param.name); });

} // namespace
