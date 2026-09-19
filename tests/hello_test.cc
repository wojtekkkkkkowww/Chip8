#include <algorithm>
#include <cstdint>
#include <gtest/gtest.h>
#include <optional>
#include <sstream>

#include "MonoScreen.h"
#include <array>
#include <stdexcept>

struct ChipData {
    std::array<uint8_t, 4096> ram{};
    std::array<uint8_t, 16> common_registers{};
    uint16_t index_register{};
    uint16_t program_counter{};
    MonoScreen screen;
};

enum class SpriteColisionType { Clipping,
                                Wrapping };

struct Config {
    SpriteColisionType colisionType{SpriteColisionType::Clipping};
};

class ConfigStorage {
  private:
    inline static Config config{};
    ConfigStorage() = delete;

  public:
    static void SetConfig(Config config_) { config = config_; };
    static Config getConfig() { return config; };
};

TEST(ScreenTest, ScreenInit) {
    MonoScreen screen;

    ASSERT_EQ(screen.get(0, 0), 0);
    //
    screen.set(3, 4);
    ASSERT_EQ(screen.get(3, 4), 1);

    screen.reset(3, 4);
    ASSERT_EQ(screen.get(3, 4), 0);
}

enum class CommandName {
    ClearScreen,
    JumpToAdress,
    SaveToRegister,
    AddToRegister,
    SaveToIndexRegister,
    DrawSprite,
    TwoAdressInstruction,
    CallNativeRoutine,
    ReturnFromSubroutine,
    CallSubroutine,
    SkipIfEqualToValue,
    SkipIfNotEqualToValue,
    SkipIfRegistersEqual,
    SkipIfRegistersNotEqual,
    JumpWithOffset,
    RandomWithMask,
    SkipIfKeyPressed,
    SkipIfKeyNotPressed,
    ReadDelayTimer,
    WaitForKey,
    SetDelayTimer,
    SetSoundTimer,
    AddToIndexRegister,
    SetIndexToFontSprite,
    StoreBinaryCodedDecimal,
    StoreRegisters,
    LoadRegisters
};

class AssemblyCommand {
  public:
    virtual void execute(ChipData &data) = 0;
    virtual CommandName getName() = 0;
    virtual ~AssemblyCommand() = default;

    static constexpr uint16_t FFF(uint16_t opcode) { return opcode & 0x0FFF; }
    static constexpr uint8_t FF(uint16_t opcode) {
        return static_cast<uint8_t>(opcode & 0x00FF);
    }
    static constexpr uint8_t F(uint16_t opcode) {
        return static_cast<uint8_t>(opcode & 0x000F);
    }
    // >> has biger prio than &
    static constexpr uint8_t F000(uint16_t opcode) {
        return static_cast<uint8_t>((opcode & 0xF000) >> 12);
    }
    static constexpr uint8_t F00(uint16_t opcode) {
        return static_cast<uint8_t>((opcode & 0x0F00) >> 8);
    }
    static constexpr uint8_t F0(uint16_t opcode) {
        return static_cast<uint8_t>((opcode & 0x00F0) >> 4);
    }
};

class ClearScreen : public AssemblyCommand {
  public:
    void execute(ChipData &data) override {
        for(size_t i = 0; i < MonoScreen::WIDTH; i++) {
            for(size_t j = 0; j < MonoScreen::HIGTH; j++) {
                data.screen.reset(i, j);
            }
        }
    }
    CommandName getName() override { return CommandName::ClearScreen; }
};

class JumpToAdress : public AssemblyCommand {
  private:
    uint16_t adress_{};

  public:
    JumpToAdress(uint16_t command) : adress_(FFF(command)) {}
    void execute(ChipData &data) override { data.program_counter = adress_; }
    CommandName getName() override { return CommandName::JumpToAdress; }
};

class SaveToRegister : public AssemblyCommand {
  private:
    uint8_t register_{};
    uint8_t value_{};

  public:
    SaveToRegister(uint16_t command)
        : register_(F00(command)), value_(FF(command)) {}

    void execute(ChipData &data) override {
        data.common_registers[register_] = value_;
    }

    CommandName getName() override { return CommandName::SaveToRegister; }
};

class SaveToIndexRegister : public AssemblyCommand {
  private:
    uint16_t value_{};

  public:
    SaveToIndexRegister(uint16_t command) : value_(FFF(command)) {}

    void execute(ChipData &data) override { data.index_register = value_; }

    CommandName getName() override { return CommandName::SaveToIndexRegister; }
};

class AddToRegister : public AssemblyCommand {
  private:
    uint8_t register_{};
    uint8_t value_{};

  public:
    AddToRegister(uint16_t command)
        : register_(F00(command)), value_(FF(command)) {}

    void execute(ChipData &data) override {
        data.common_registers[register_] += value_;
    }

    CommandName getName() override { return CommandName::AddToRegister; }
};

class DrawSprite : public AssemblyCommand {
  private:
    uint8_t xReg_{};
    uint8_t yReg_{};
    uint8_t rows_{};

    template <uint8_t size>
    std::optional<uint8_t> newPos(uint8_t pos, uint8_t offset) {

        uint8_t next = pos + offset;
        switch(ConfigStorage::getConfig().colisionType) {
        case SpriteColisionType::Wrapping:
            return next % size;
        case SpriteColisionType::Clipping:
            if(next >= size)
                return std::nullopt;
            return next;
        }
        throw std::logic_error("Unknown SpriteColisionType");
    }

  public:
    DrawSprite(uint16_t command)
        : xReg_(F00(command)), yReg_(F0(command)), rows_(F(command)) {}

    void execute(ChipData &data) override {

        uint8_t xPos = data.common_registers[xReg_] % MonoScreen::WIDTH;
        uint8_t yPos = data.common_registers[yReg_] % MonoScreen::HIGTH;

        uint8_t colisionRegister = 0xF;

        data.common_registers[colisionRegister] = 0;

        for(uint8_t i = 0; i < rows_; i++) {

            uint8_t spriteByte = data.ram.at(data.index_register + i);
            auto currentYpos = newPos<MonoScreen::HIGTH>(yPos, i);

            if(not currentYpos)
                continue;

            for(uint8_t j = 0; j < 8; j++) {

                auto currentXpos = newPos<MonoScreen::WIDTH>(xPos, j);

                if(not currentXpos)
                    continue;

                uint8_t color = spriteByte & (0x1 << (7 - j));

                if(color > 0) {
                    if(data.screen.get(*currentXpos, *currentYpos) == 1) {
                        data.common_registers[colisionRegister] = 1;
                        data.screen.reset(*currentXpos, *currentYpos);
                    } else {
                        data.screen.set(*currentXpos, *currentYpos);
                    }
                }
            }
        }
    }

    CommandName getName() override { return CommandName::DrawSprite; }
};

class TwoAdressInstruction : public AssemblyCommand {
  private:
    uint8_t xReg_{};
    uint8_t yReg_{};
    uint8_t operationType_{};

  public:
    TwoAdressInstruction(uint16_t command)
        : xReg_(F00(command)), yReg_(F0(command)), operationType_(F(command)) {}
    void execute(ChipData &data) override {
        auto &xVal = data.common_registers[xReg_];
        auto &yVal = data.common_registers[yReg_];
        auto &fV = data.common_registers[0xF];

        switch(operationType_) {
        case 0:
            xVal = yVal;
            break;

        case 1:
            xVal |= yVal;
            fV = 0; // COSMAC based variants resets fv
            break;

        case 2:
            xVal &= yVal;
            fV = 0;
            break;

        case 3:
            xVal ^= yVal;
            fV = 0;
            break;

        case 4: {
            const uint16_t sum = xVal + yVal;
            xVal = static_cast<uint8_t>(sum);
            fV = sum > 0xFF;
            break;
        }

        case 5: {
            const uint8_t flag = xVal >= yVal;
            xVal -= yVal;
            fV = flag;
            break;
        }

        case 6: {
            const uint8_t value = yVal;
            const uint8_t flag = value & 0x1;
            xVal = (value >> 1);
            fV = flag;
            break;
        }
        case 7: {
            const uint8_t flag = yVal >= xVal;
            xVal = yVal - xVal;
            fV = flag;
            break;
        }

        case 0xE: {
            const uint8_t value = yVal;
            const uint8_t flag = (value >> 7) & 0x1;
            xVal = static_cast<uint8_t>((value << 1));
            fV = flag;
            break;
        }
        default: {
            throw std::logic_error("Unknown operation");
        }
        }
    }
    CommandName getName() override { return CommandName::TwoAdressInstruction; }
};

class Decoder {
  public:
    std::unique_ptr<AssemblyCommand> decode(uint16_t command) {
        switch(command & 0xF000) {
        case 0x0000:
            return std::make_unique<ClearScreen>();
        case 0x1000:
            return std::make_unique<JumpToAdress>(command);
        case 0x6000:
            return std::make_unique<SaveToRegister>(command);
        case 0x7000:
            return std::make_unique<AddToRegister>(command);
        case 0x8000:
            return std::make_unique<TwoAdressInstruction>(command);
        case 0xA000:
            return std::make_unique<SaveToIndexRegister>(command);
        case 0xD000:
            return std::make_unique<DrawSprite>(command);
        }

        throw std::runtime_error("Unknown Chip-8 opcode");
    }
};

struct DecodingTestCase {
    uint16_t opcode;
    CommandName expectedName;
};

class DecodingTest : public testing::TestWithParam<DecodingTestCase> {};

TEST_P(DecodingTest, RecognizesCommand) {
    const auto &testCase = GetParam();
    Decoder decoder;
    auto command = decoder.decode(testCase.opcode);
    ASSERT_NE(command, nullptr);
    EXPECT_EQ(command->getName(), testCase.expectedName);
}

constexpr auto decodingCases = std::to_array<DecodingTestCase>({
    {0x0123, CommandName::CallNativeRoutine},
    {0x00E0, CommandName::ClearScreen},
    {0x00EE, CommandName::ReturnFromSubroutine},
    {0x1111, CommandName::JumpToAdress},
    {0x2111, CommandName::CallSubroutine},
    {0x31AB, CommandName::SkipIfEqualToValue},
    {0x41AB, CommandName::SkipIfNotEqualToValue},
    {0x5120, CommandName::SkipIfRegistersEqual},
    {0x6111, CommandName::SaveToRegister},
    {0x7111, CommandName::AddToRegister},
    {0x8120, CommandName::TwoAdressInstruction},
    {0x8121, CommandName::TwoAdressInstruction},
    {0x8122, CommandName::TwoAdressInstruction},
    {0x8123, CommandName::TwoAdressInstruction},
    {0x8124, CommandName::TwoAdressInstruction},
    {0x8125, CommandName::TwoAdressInstruction},
    {0x8126, CommandName::TwoAdressInstruction},
    {0x8127, CommandName::TwoAdressInstruction},
    {0x812E, CommandName::TwoAdressInstruction},
    {0x9120, CommandName::SkipIfRegistersNotEqual},
    {0xA111, CommandName::SaveToIndexRegister},
    {0xB123, CommandName::JumpWithOffset},
    {0xC1AB, CommandName::RandomWithMask},
    {0xD123, CommandName::DrawSprite},
    {0xE19E, CommandName::SkipIfKeyPressed},
    {0xE1A1, CommandName::SkipIfKeyNotPressed},
    {0xF107, CommandName::ReadDelayTimer},
    {0xF10A, CommandName::WaitForKey},
    {0xF115, CommandName::SetDelayTimer},
    {0xF118, CommandName::SetSoundTimer},
    {0xF11E, CommandName::AddToIndexRegister},
    {0xF129, CommandName::SetIndexToFontSprite},
    {0xF133, CommandName::StoreBinaryCodedDecimal},
    {0xF155, CommandName::StoreRegisters},
    {0xF165, CommandName::LoadRegisters},
});

INSTANTIATE_TEST_SUITE_P(
    MachineCode, DecodingTest, testing::ValuesIn(decodingCases),
    [](const auto &info) {
        std::ostringstream name;
        name << "Opcode" << std::hex << info.param.opcode;
        return name.str();
    });

TEST(MachineCode, ExecuteCommand) {
    Decoder decoder;
    ChipData data;

    data.screen.set(0, 0);
    auto clearScreen = decoder.decode(0x00E0);
    clearScreen->execute(data);
    ASSERT_EQ(data.screen.get(0, 0), 0);

    auto jumpToAdress = decoder.decode(0x1111);
    jumpToAdress->execute(data);
    ASSERT_EQ(data.program_counter, 0x0111);

    auto saveToRegister = decoder.decode(0x6111);
    saveToRegister->execute(data);
    ASSERT_EQ(data.common_registers[0x01], 0x0011);

    auto addToRegister = decoder.decode(0x7111);
    addToRegister->execute(data);
    ASSERT_EQ(data.common_registers[0x01], 0x0022);

    auto saveToIndexRegister = decoder.decode(0xA111);
    saveToIndexRegister->execute(data);
    ASSERT_EQ(data.index_register, 0x0111);
}

std::string readSprite(uint8_t xPos, uint8_t yPos, uint8_t rows,
                       const MonoScreen &screen) {

    std::string line{};
    std::string sprite{};
    std::stringstream ss{};
    ss << screen;
    size_t index{0};
    while(std::getline(ss, line) and
          (std::count(sprite.begin(), sprite.end(), '\n') < rows)) {
        if(index >= yPos) {
            sprite.append(line.substr(xPos, 8));
            sprite.push_back('\n');
        }
        index++;
    }
    return sprite;
}

TEST(MachineCode, DrawSprite) {
    Decoder decoder;
    ChipData data;

    // loading sprite to ram
    data.index_register = 0;
    data.ram[0] = 0x66;
    data.ram[1] = 0x7E;
    data.ram[2] = 0xC3;
    data.ram[3] = 0x99;

    // setup X and Y
    uint8_t xPos = 1;
    uint8_t yPos = 1;
    data.common_registers[2] = xPos;
    data.common_registers[3] = yPos;

    auto drawScreen = decoder.decode(0xD234);
    drawScreen->execute(data);

    auto expectedSprite = "O##OO##O\nO######O\n##OOOO##\n#OO##OO#\n";
    auto fetchedSprite = readSprite(xPos, yPos, 4, data.screen);

    ASSERT_EQ(expectedSprite, fetchedSprite);
    ASSERT_EQ(data.common_registers[0xF], 0);

    // clear screen with the same sprite
    drawScreen->execute(data);

    expectedSprite = "OOOOOOOO\nOOOOOOOO\nOOOOOOOO\nOOOOOOOO\n";
    fetchedSprite = readSprite(xPos, yPos, 4, data.screen);

    ASSERT_EQ(expectedSprite, fetchedSprite);
    ASSERT_EQ(data.common_registers[0xF], 1);
}

struct TwoAdressTestCase {
    std::string_view name;
    uint16_t opcode;
    uint8_t x;
    uint8_t y;
    uint8_t initialFlag;
    uint8_t expectedX;
    uint8_t expectedFlag;
};

class TwoAdressInstructionTest
    : public testing::TestWithParam<TwoAdressTestCase> {};

TEST_P(TwoAdressInstructionTest, ExecuteCommand) {
    const auto &testCase = GetParam();
    ChipData data{};
    Decoder decoder;
    data.common_registers[1] = testCase.x;
    data.common_registers[2] = testCase.y;
    data.common_registers[0xF] = testCase.initialFlag;

    auto command = decoder.decode(testCase.opcode);
    command->execute(data);

    ASSERT_EQ(data.common_registers[1], testCase.expectedX);
    ASSERT_EQ(data.common_registers[2], testCase.y);
    ASSERT_EQ(data.common_registers[0xF], testCase.expectedFlag);
}

constexpr auto twoAdressCases = std::to_array<TwoAdressTestCase>({
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
    MachineCode, TwoAdressInstructionTest, testing::ValuesIn(twoAdressCases),
    [](const auto &info) {
        return std::string(info.param.name);
    });
