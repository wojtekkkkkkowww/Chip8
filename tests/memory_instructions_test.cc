#include "ChipData.h"
#include "Decoder.h"
#include <algorithm>
#include <array>
#include <cstdint>
#include <gtest/gtest.h>
#include <string>
#include <string_view>

namespace {

struct TransferRegistersTestCase {
    std::string_view name;
    uint16_t opcode;
    std::array<uint8_t, 16> registers;
    std::array<uint8_t, 16> memory;
    std::array<uint8_t, 16> expectedRegisters;
    std::array<uint8_t, 16> expectedMemory;
};

class TransferRegistersTest
    : public testing::TestWithParam<TransferRegistersTestCase> {
  protected:
    std::array<uint8_t, 4096> prepareRam(ChipData &data) {
        const auto &testCase = GetParam();
        data.ram.fill(0xAA);

        // Prepare RAM:
        std::copy(testCase.memory.begin(), testCase.memory.end(),
                  data.ram.begin() + data.indexRegister);

        // Prepare expectedRam
        auto expectedRam = data.ram;
        std::copy(testCase.expectedMemory.begin(), testCase.expectedMemory.end(),
                  expectedRam.begin() + data.indexRegister);
        return expectedRam;
    }
};

constexpr std::array<uint8_t, 16> transferValues{
    0x12, 0x00, 0xFF, 0x34, 0x56, 0x78, 0x9A, 0xBC,
    0xDE, 0xF0, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66};

constexpr auto transferRegistersCases = std::to_array<TransferRegistersTestCase>({
    {"StoreSingleRegister", 0xF055, transferValues, {}, transferValues, {0x12}},
    {"StoreMultipleRegisters", 0xF355, transferValues, {}, transferValues, {0x12, 0x00, 0xFF, 0x34}},
    {"StoreAllRegisters", 0xFF55, transferValues, {}, transferValues, transferValues},
    {"LoadSingleRegister", 0xF065, {}, transferValues, {0x12}, transferValues},
    {"LoadMultipleRegisters", 0xF365, {}, transferValues, {0x12, 0x00, 0xFF, 0x34}, transferValues},
    {"LoadAllRegisters", 0xFF65, {}, transferValues, transferValues, transferValues},
});

TEST_P(TransferRegistersTest, ExecuteCommand) {
    const auto &testCase = GetParam();
    ChipData data{};
    Decoder decoder;
    data.indexRegister = 0x300;
    data.commonRegisters = testCase.registers;
    const auto expectedRam = prepareRam(data);

    auto command = decoder.decode(testCase.opcode);
    command.execute(data);

    EXPECT_EQ(data.commonRegisters, testCase.expectedRegisters);
    EXPECT_EQ(data.ram, expectedRam);
}

INSTANTIATE_TEST_SUITE_P(
    MachineCode, TransferRegistersTest, testing::ValuesIn(transferRegistersCases),
    [](const auto &info) { return std::string(info.param.name); });

struct BinaryCodedDecimalTestCase {
    uint8_t value;
    std::array<uint8_t, 3> digits;
};

class StoreBinaryCodedDecimalTest
    : public testing::TestWithParam<BinaryCodedDecimalTestCase> {};

TEST_P(StoreBinaryCodedDecimalTest, ExecuteCommand) {
    const auto &testCase = GetParam();
    ChipData data{};
    Decoder decoder;
    data.indexRegister = 0x300;
    data.commonRegisters[5] = testCase.value;
    data.ram.fill(0xAA);

    auto command = decoder.decode(0xF533);
    command.execute(data);

    for(size_t i = 0; i < testCase.digits.size(); ++i)
        EXPECT_EQ(data.ram[0x300 + i], testCase.digits[i]);
    EXPECT_EQ(data.ram[0x2FF], 0xAA);
    EXPECT_EQ(data.ram[0x303], 0xAA);
    EXPECT_EQ(data.indexRegister, 0x300);
    EXPECT_EQ(data.commonRegisters[5], testCase.value);
}

constexpr auto binaryCodedDecimalCases =
    std::to_array<BinaryCodedDecimalTestCase>({
        {0, {0, 0, 0}},
        {7, {0, 0, 7}},
        {42, {0, 4, 2}},
        {100, {1, 0, 0}},
        {255, {2, 5, 5}},
    });

INSTANTIATE_TEST_SUITE_P(
    MachineCode, StoreBinaryCodedDecimalTest,
    testing::ValuesIn(binaryCodedDecimalCases));

} // namespace
