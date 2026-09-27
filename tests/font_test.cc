#include "ChipData.h"
#include "Decoder.h"
#include "Font.h"
#include <algorithm>
#include <gtest/gtest.h>
#include <string>

namespace {

TEST(MachineCode, LoadFontIntoRam) {
    ChipData data;
    data.ram.fill(0xAA);
    Font::loadInto(data.ram);

    EXPECT_EQ(data.ram[0x4F], 0xAA);
    EXPECT_EQ(data.ram[0xA0], 0xAA);
    EXPECT_TRUE(std::equal(Font::DATA.begin(), Font::DATA.end(), data.ram.begin() + 0x50));
}

struct FontAddressTestCase {
    const char *name;
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
        FontAddressTestCase{"Digit0", 0x0, 0x50},
        FontAddressTestCase{"DigitA", 0xA, 0x82},
        FontAddressTestCase{"DigitF", 0xF, 0x9B}),
    [](const auto &info) { return std::string(info.param.name); });

} // namespace
