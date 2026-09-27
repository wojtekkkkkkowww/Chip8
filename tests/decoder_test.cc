#include "Decoder.h"
#include <array>
#include <cstdint>
#include <functional>
#include <gtest/gtest.h>
#include <sstream>

namespace {

struct DecodingTestCase {
    uint16_t opcode;
    // bool (*matches)(const AssemblyCommand &);
    std::function<bool(const AssemblyCommand &)> matches;
};

class DecodingTest : public testing::TestWithParam<DecodingTestCase> {};

TEST_P(DecodingTest, RecognizesCommand) {
    const auto &testCase = GetParam();
    Decoder decoder;
    auto command = decoder.decode(testCase.opcode);
    EXPECT_TRUE(testCase.matches(command));
}

template <class T>
bool matchesCommand(const AssemblyCommand &command) {
    return command.is<T>();
}

const auto decodingCases = std::to_array<DecodingTestCase>({
    {0x00E0, matchesCommand<ClearScreen>},
    {0x00EE, matchesCommand<ReturnFromSubroutine>},
    {0x2345, matchesCommand<CallSubroutine>},
    {0x1111, matchesCommand<JumpToAddress>},
    {0x3123, matchesCommand<SkipIf>},
    {0x4123, matchesCommand<SkipIf>},
    {0x5120, matchesCommand<SkipIf>},
    {0x9120, matchesCommand<SkipIf>},
    {0xE19E, matchesCommand<SkipIf>},
    {0xE1A1, matchesCommand<SkipIf>},
    {0x6111, matchesCommand<SaveToRegister>},
    {0x7111, matchesCommand<AddToRegister>},
    {0x8120, matchesCommand<TwoAddressInstruction>},
    {0x8121, matchesCommand<TwoAddressInstruction>},
    {0x8122, matchesCommand<TwoAddressInstruction>},
    {0x8123, matchesCommand<TwoAddressInstruction>},
    {0x8124, matchesCommand<TwoAddressInstruction>},
    {0x8125, matchesCommand<TwoAddressInstruction>},
    {0x8126, matchesCommand<TwoAddressInstruction>},
    {0x8127, matchesCommand<TwoAddressInstruction>},
    {0x812E, matchesCommand<TwoAddressInstruction>},
    {0xA111, matchesCommand<SaveToIndexRegister>},
    {0xB123, matchesCommand<JumpWithOffset>},
    {0xD123, matchesCommand<DrawSprite>},
    {0xF31E, matchesCommand<AddToIndexRegister>},
    {0xF533, matchesCommand<StoreBinaryCodedDecimal>},
    {0xF355, matchesCommand<TransferRegisters>},
    {0xF365, matchesCommand<TransferRegisters>},
});

INSTANTIATE_TEST_SUITE_P(
    MachineCode, DecodingTest, testing::ValuesIn(decodingCases),
    [](const auto &info) {
        std::ostringstream name;
        name << "Opcode" << std::hex << info.param.opcode;
        return name.str();
    });

} // namespace
