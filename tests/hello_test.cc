#include <algorithm>
#include <cstdint>
#include <gtest/gtest.h>
#include <optional>
#include <sstream>
#include <utility>
#include <variant>

#include "MonoScreen.h"
#include <array>
#include <stdexcept>

struct ChipData {
    std::array<uint8_t, 4096> ram{};
    std::array<uint8_t, 16> common_registers{};
    std::array<uint8_t, 16> keyboard{};
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

/*
enum class CommandName {
    ClearScreen, // +
    JumpToAdress, // +
    SaveToRegister, // +
    AddToRegister, // +
    SaveToIndexRegister, // +
    DrawSprite, // +
    TwoAdressInstruction, // +
    CallNativeRoutine,
    ReturnFromSubroutine,
    CallSubroutine,
    SkipIfEqualToValue, // +
    SkipIfNotEqualToValue, // +
    SkipIfRegistersEqual, // +
    SkipIfRegistersNotEqual, //+
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
*/

namespace Opcode {
static constexpr uint16_t FFF(uint16_t opcode) { return opcode & 0x0FFF; }
static constexpr uint8_t FF(uint16_t opcode) {
    return static_cast<uint8_t>(opcode & 0x00FF);
}
static constexpr uint8_t F(uint16_t opcode) {
    return static_cast<uint8_t>(opcode & 0x000F);
}
// >> has biger prio than &
// static constexpr uint8_t F000(uint16_t opcode) {
//    return static_cast<uint8_t>((opcode & 0xF000) >> 12);
//}
static constexpr uint8_t F00(uint16_t opcode) {
    return static_cast<uint8_t>((opcode & 0x0F00) >> 8);
}
static constexpr uint8_t F0(uint16_t opcode) {
    return static_cast<uint8_t>((opcode & 0x00F0) >> 4);
}
} // namespace Opcode

enum class SkipCondition {
    EqualToValue,
    NotEqualToValue,
    RegistersEqual,
    RegistersNotEqual,
    KeyPressed,
    KeyNotPressed
};

class SkipIf {
  private:
    SkipCondition condition_{};
    uint16_t opcode_;

  public:
    SkipIf(uint16_t opcode, SkipCondition condition)
        : condition_(condition), opcode_(opcode) {}
    void execute(ChipData &data) {
        bool conditionSatisfied = [this, &data]() {
            switch(condition_) {
            case SkipCondition::EqualToValue: {
                uint8_t regX = Opcode::F00(opcode_);
                uint8_t number = Opcode::FF(opcode_);
                return data.common_registers[regX] == number;
            }
            case SkipCondition::NotEqualToValue: {
                uint8_t regX = Opcode::F00(opcode_);
                uint8_t number = Opcode::FF(opcode_);
                return data.common_registers[regX] != number;
            }
            case SkipCondition::RegistersEqual: {
                uint8_t regX = Opcode::F00(opcode_);
                uint8_t regY = Opcode::F0(opcode_);
                return data.common_registers[regX] == data.common_registers[regY];
            }
            case SkipCondition::RegistersNotEqual: {
                uint8_t regX = Opcode::F00(opcode_);
                uint8_t regY = Opcode::F0(opcode_);
                return data.common_registers[regX] != data.common_registers[regY];
            }
            case SkipCondition::KeyPressed: {
                uint8_t regX = Opcode::F00(opcode_);
                uint8_t keyIndex = Opcode::F(data.common_registers[regX]);
                return static_cast<bool>(data.keyboard[keyIndex]);
            }
            case SkipCondition::KeyNotPressed: {
                uint8_t regX = Opcode::F00(opcode_);
                uint8_t keyIndex = Opcode::F(data.common_registers[regX]);
                return not static_cast<bool>(data.keyboard[keyIndex]);
            }
            default:
                return false;
            }
        }();

        if(conditionSatisfied)
            data.program_counter += 2; // points to next command
    }
};

class ClearScreen {
  public:
    void execute(ChipData &data) {
        for(size_t i = 0; i < MonoScreen::WIDTH; i++) {
            for(size_t j = 0; j < MonoScreen::HIGTH; j++) {
                data.screen.reset(i, j);
            }
        }
    }
};

class JumpToAdress {
  private:
    uint16_t adress_{};

  public:
    JumpToAdress(uint16_t command) : adress_(Opcode::FFF(command)) {}
    void execute(ChipData &data) { data.program_counter = adress_; }
};

class SaveToRegister {
  private:
    uint8_t register_{};
    uint8_t value_{};

  public:
    SaveToRegister(uint16_t command)
        : register_(Opcode::F00(command)), value_(Opcode::FF(command)) {}

    void execute(ChipData &data) {
        data.common_registers[register_] = value_;
    }
};

class SaveToIndexRegister {
  private:
    uint16_t value_{};

  public:
    SaveToIndexRegister(uint16_t command) : value_(Opcode::FFF(command)) {}

    void execute(ChipData &data) { data.index_register = value_; }
};

class AddToRegister {
  private:
    uint8_t register_{};
    uint8_t value_{};

  public:
    AddToRegister(uint16_t command)
        : register_(Opcode::F00(command)), value_(Opcode::FF(command)) {}

    void execute(ChipData &data) {
        data.common_registers[register_] += value_;
    }
};

class DrawSprite {
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
        : xReg_(Opcode::F00(command)), yReg_(Opcode::F0(command)),
          rows_(Opcode::F(command)) {}

    void execute(ChipData &data) {

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
};

class TwoAdressInstruction {
  private:
    uint8_t xReg_{};
    uint8_t yReg_{};
    uint8_t operationType_{};

  public:
    TwoAdressInstruction(uint16_t command)
        : xReg_(Opcode::F00(command)), yReg_(Opcode::F0(command)),
          operationType_(Opcode::F(command)) {}
    void execute(ChipData &data) {
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
};

class AssemblyCommand {
  private:
    using Storage = std::variant<ClearScreen, JumpToAdress, SaveToRegister,
                                 AddToRegister, SaveToIndexRegister, DrawSprite,
                                 TwoAdressInstruction, SkipIf>;
    Storage instruction_;

  public:
    template <class T>
    explicit AssemblyCommand(T instruction) // nie trzeba dac jakiegos &&T ?
        : instruction_(std::move(instruction)) {}

    template <class T>
    bool is() const {
        return std::holds_alternative<T>(instruction_);
    }

    void execute(ChipData &data) {
        std::visit([&data](auto &instruction) { instruction.execute(data); },
                   instruction_);
    }
};

class Decoder {
  public:
    AssemblyCommand decode(uint16_t command) {
        switch(command & 0xF000) {
        case 0x0000:
            if(command == 0x00E0)
                return AssemblyCommand{ClearScreen{}};
            break;
        case 0x1000:
            return AssemblyCommand{JumpToAdress{command}};
        case 0x3000:
            return AssemblyCommand{SkipIf{command, SkipCondition::EqualToValue}};
        case 0x4000:
            return AssemblyCommand{SkipIf{command, SkipCondition::NotEqualToValue}};
        case 0x5000:
            if(Opcode::F(command) == 0)
                return AssemblyCommand{
                    SkipIf{command, SkipCondition::RegistersEqual}};
            break;
        case 0x9000:
            if(Opcode::F(command) == 0)
                return AssemblyCommand{
                    SkipIf{command, SkipCondition::RegistersNotEqual}};
            break;
        case 0x6000:
            return AssemblyCommand{SaveToRegister{command}};
        case 0x7000:
            return AssemblyCommand{AddToRegister{command}};
        case 0x8000:
            return AssemblyCommand{TwoAdressInstruction{command}};
        case 0xA000:
            return AssemblyCommand{SaveToIndexRegister{command}};
        case 0xD000:
            return AssemblyCommand{DrawSprite{command}};
        case 0xE000:
            if(Opcode::FF(command) == 0x9E)
                return AssemblyCommand{
                    SkipIf{command, SkipCondition::KeyPressed}};
            if(Opcode::FF(command) == 0xA1)
                return AssemblyCommand{
                    SkipIf{command, SkipCondition::KeyNotPressed}};
            break;
        }

        throw std::runtime_error("Unknown Chip-8 opcode");
    }
};

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
    {0x1111, matchesCommand<JumpToAdress>},
    {0x3123, matchesCommand<SkipIf>},
    {0x4123, matchesCommand<SkipIf>},
    {0x5120, matchesCommand<SkipIf>},
    {0x9120, matchesCommand<SkipIf>},
    {0xE19E, matchesCommand<SkipIf>},
    {0xE1A1, matchesCommand<SkipIf>},
    {0x6111, matchesCommand<SaveToRegister>},
    {0x7111, matchesCommand<AddToRegister>},
    {0x8120, matchesCommand<TwoAdressInstruction>},
    {0x8121, matchesCommand<TwoAdressInstruction>},
    {0x8122, matchesCommand<TwoAdressInstruction>},
    {0x8123, matchesCommand<TwoAdressInstruction>},
    {0x8124, matchesCommand<TwoAdressInstruction>},
    {0x8125, matchesCommand<TwoAdressInstruction>},
    {0x8126, matchesCommand<TwoAdressInstruction>},
    {0x8127, matchesCommand<TwoAdressInstruction>},
    {0x812E, matchesCommand<TwoAdressInstruction>},
    {0xA111, matchesCommand<SaveToIndexRegister>},
    {0xD123, matchesCommand<DrawSprite>},
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
    clearScreen.execute(data);
    ASSERT_EQ(data.screen.get(0, 0), 0);

    auto jumpToAdress = decoder.decode(0x1111);
    jumpToAdress.execute(data);
    ASSERT_EQ(data.program_counter, 0x0111);

    auto saveToRegister = decoder.decode(0x6111);
    saveToRegister.execute(data);
    ASSERT_EQ(data.common_registers[0x01], 0x0011);

    auto addToRegister = decoder.decode(0x7111);
    addToRegister.execute(data);
    ASSERT_EQ(data.common_registers[0x01], 0x0022);

    auto saveToIndexRegister = decoder.decode(0xA111);
    saveToIndexRegister.execute(data);
    ASSERT_EQ(data.index_register, 0x0111);
}

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
    data.program_counter = 0x200;

    if(const auto *registersData = std::get_if<RegistersData>(&testCase.data)) {
        data.common_registers[1] = registersData->x;
        data.common_registers[2] = registersData->y;
    }
    if(const auto *keyData = std::get_if<KeyData>(&testCase.data)) {
        data.common_registers[1] = keyData->keyIndex;
        data.keyboard[keyData->keyIndex] = keyData->pressed;
    }

    auto command = decoder.decode(testCase.opcode);
    command.execute(data);

    ASSERT_EQ(data.program_counter, testCase.expectedProgramCounter);
}

INSTANTIATE_TEST_SUITE_P(
    MachineCode, SkipIfTest, testing::ValuesIn(skipIfCases),
    [](const auto &info) { return std::string(info.param.name); });

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
    drawScreen.execute(data);

    auto expectedSprite = "O##OO##O\nO######O\n##OOOO##\n#OO##OO#\n";
    auto fetchedSprite = readSprite(xPos, yPos, 4, data.screen);

    ASSERT_EQ(expectedSprite, fetchedSprite);
    ASSERT_EQ(data.common_registers[0xF], 0);

    // clear screen with the same sprite
    drawScreen.execute(data);

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
    command.execute(data);

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
