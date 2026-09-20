#include "gtest/gtest.h"
#include <algorithm>
#include <cstdint>
#include <gtest/gtest.h>
#include <optional>
#include <sstream>
#include <utility>
#include <variant>
#include <vector>

#include "MonoScreen.h"
#include <array>
#include <stdexcept>

struct ChipData {
    std::array<uint8_t, 4096> ram{};
    std::array<uint8_t, 16> commonRegisters{};
    std::array<uint8_t, 16> keyboard{};
    uint16_t indexRegister{};
    uint16_t programCounter{};
    MonoScreen screen;
};

enum class SpriteCollisionType { Clipping,
                                 Wrapping };

struct Config {
    SpriteCollisionType collisionType{SpriteCollisionType::Clipping};
};

class ConfigStorage {
  private:
    inline static Config config{};
    ConfigStorage() = delete;

  public:
    static void SetConfig(Config newConfig) { config = newConfig; };
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
    JumpToAddress, // +
    SaveToRegister, // +
    AddToRegister, // +
    SaveToIndexRegister, // +
    DrawSprite, // +
    TwoAddressInstruction, // +
    CallNativeRoutine,
    ReturnFromSubroutine,
    CallSubroutine,
    SkipIfEqualToValue, // +
    SkipIfNotEqualToValue, // +
    SkipIfRegistersEqual, // +
    SkipIfRegistersNotEqual, //+
    JumpWithOffset,
    RandomWithMask,
    SkipIfKeyPressed, //+
    SkipIfKeyNotPressed, //+
    ReadDelayTimer,
    WaitForKey,
    SetDelayTimer,
    SetSoundTimer,
    AddToIndexRegister, // +
    SetIndexToFontSprite,
    StoreBinaryCodedDecimal, // +
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
// >> has higher precedence than &
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
    SkipCondition condition{};
    uint16_t opcode;

  public:
    SkipIf(uint16_t rawOpcode, SkipCondition skipCondition)
        : condition(skipCondition), opcode(rawOpcode) {}
    void execute(ChipData &data) {
        bool conditionSatisfied = [this, &data]() {
            switch(condition) {
            case SkipCondition::EqualToValue: {
                uint8_t xRegisterIndex = Opcode::F00(opcode);
                uint8_t value = Opcode::FF(opcode);
                return data.commonRegisters[xRegisterIndex] == value;
            }
            case SkipCondition::NotEqualToValue: {
                uint8_t xRegisterIndex = Opcode::F00(opcode);
                uint8_t value = Opcode::FF(opcode);
                return data.commonRegisters[xRegisterIndex] != value;
            }
            case SkipCondition::RegistersEqual: {
                uint8_t xRegisterIndex = Opcode::F00(opcode);
                uint8_t yRegisterIndex = Opcode::F0(opcode);
                return data.commonRegisters[xRegisterIndex] == data.commonRegisters[yRegisterIndex];
            }
            case SkipCondition::RegistersNotEqual: {
                uint8_t xRegisterIndex = Opcode::F00(opcode);
                uint8_t yRegisterIndex = Opcode::F0(opcode);
                return data.commonRegisters[xRegisterIndex] != data.commonRegisters[yRegisterIndex];
            }
            case SkipCondition::KeyPressed: {
                uint8_t xRegisterIndex = Opcode::F00(opcode);
                uint8_t keyIndex = Opcode::F(data.commonRegisters[xRegisterIndex]);
                return static_cast<bool>(data.keyboard[keyIndex]);
            }
            case SkipCondition::KeyNotPressed: {
                uint8_t xRegisterIndex = Opcode::F00(opcode);
                uint8_t keyIndex = Opcode::F(data.commonRegisters[xRegisterIndex]);
                return not static_cast<bool>(data.keyboard[keyIndex]);
            }
            default:
                return false;
            }
        }();

        if(conditionSatisfied)
            data.programCounter += 2; // points to the next command
    }
};

class ClearScreen {
  public:
    void execute(ChipData &data) {
        for(size_t i = 0; i < MonoScreen::WIDTH; i++) {
            for(size_t j = 0; j < MonoScreen::HEIGHT; j++) {
                data.screen.reset(i, j);
            }
        }
    }
};

class JumpToAddress {
  private:
    uint16_t address{};

  public:
    JumpToAddress(uint16_t command) : address(Opcode::FFF(command)) {}
    void execute(ChipData &data) { data.programCounter = address; }
};

class SaveToRegister {
  private:
    uint8_t registerIndex{};
    uint8_t value{};

  public:
    SaveToRegister(uint16_t command)
        : registerIndex(Opcode::F00(command)), value(Opcode::FF(command)) {}

    void execute(ChipData &data) {
        data.commonRegisters[registerIndex] = value;
    }
};

class SaveToIndexRegister {
  private:
    uint16_t value{};

  public:
    SaveToIndexRegister(uint16_t command) : value(Opcode::FFF(command)) {}

    void execute(ChipData &data) { data.indexRegister = value; }
};

class AddToIndexRegister {
  private:
    uint8_t registerIndex{};

  public:
    AddToIndexRegister(uint16_t command) : registerIndex(Opcode::F00(command)) {}

    void execute(ChipData &data) {
        data.indexRegister += data.commonRegisters[registerIndex];
    }
};

class StoreBinaryCodedDecimal {
  private:
    uint8_t registerIndex{};

  public:
    StoreBinaryCodedDecimal(uint16_t command) : registerIndex(Opcode::F00(command)) {}

    void execute(ChipData &data) {
        const uint8_t value = data.commonRegisters[registerIndex];
        data.ram[data.indexRegister] = value / 100;
        data.ram[data.indexRegister + 1] = (value / 10) % 10;
        data.ram[data.indexRegister + 2] = value % 10;
    }
};

class AddToRegister {
  private:
    uint8_t registerIndex{};
    uint8_t value{};

  public:
    AddToRegister(uint16_t command)
        : registerIndex(Opcode::F00(command)), value(Opcode::FF(command)) {}

    void execute(ChipData &data) {
        data.commonRegisters[registerIndex] += value;
    }
};

class DrawSprite {
  private:
    uint8_t xRegisterIndex{};
    uint8_t yRegisterIndex{};
    uint8_t rowCount{};

    template <uint8_t size>
    std::optional<uint8_t> offsetPosition(uint8_t position, uint8_t offset) {

        uint8_t nextPosition = position + offset;
        switch(ConfigStorage::getConfig().collisionType) {
        case SpriteCollisionType::Wrapping:
            return nextPosition % size;
        case SpriteCollisionType::Clipping:
            if(nextPosition >= size)
                return std::nullopt;
            return nextPosition;
        }
        throw std::logic_error("Unknown SpriteCollisionType");
    }

  public:
    DrawSprite(uint16_t command)
        : xRegisterIndex(Opcode::F00(command)), yRegisterIndex(Opcode::F0(command)),
          rowCount(Opcode::F(command)) {}

    void execute(ChipData &data) {

        uint8_t xPosition = data.commonRegisters[xRegisterIndex] % MonoScreen::WIDTH;
        uint8_t yPosition = data.commonRegisters[yRegisterIndex] % MonoScreen::HEIGHT;

        uint8_t collisionRegisterIndex = 0xF;

        data.commonRegisters[collisionRegisterIndex] = 0;

        for(uint8_t i = 0; i < rowCount; i++) {

            uint8_t spriteByte = data.ram.at(data.indexRegister + i);
            auto currentYPosition = offsetPosition<MonoScreen::HEIGHT>(yPosition, i);

            if(not currentYPosition)
                continue;

            for(uint8_t j = 0; j < 8; j++) {

                auto currentXPosition = offsetPosition<MonoScreen::WIDTH>(xPosition, j);

                if(not currentXPosition)
                    continue;

                uint8_t color = spriteByte & (0x1 << (7 - j));

                if(color > 0) {
                    if(data.screen.get(*currentXPosition, *currentYPosition) == 1) {
                        data.commonRegisters[collisionRegisterIndex] = 1;
                        data.screen.reset(*currentXPosition, *currentYPosition);
                    } else {
                        data.screen.set(*currentXPosition, *currentYPosition);
                    }
                }
            }
        }
    }
};

class TwoAddressInstruction {
  private:
    uint8_t xRegisterIndex{};
    uint8_t yRegisterIndex{};
    uint8_t operationType{};

  public:
    TwoAddressInstruction(uint16_t command)
        : xRegisterIndex(Opcode::F00(command)), yRegisterIndex(Opcode::F0(command)),
          operationType(Opcode::F(command)) {}
    void execute(ChipData &data) {
        auto &xValue = data.commonRegisters[xRegisterIndex];
        auto &yValue = data.commonRegisters[yRegisterIndex];
        auto &flagRegister = data.commonRegisters[0xF];

        switch(operationType) {
        case 0:
            xValue = yValue;
            break;

        case 1:
            xValue |= yValue;
            flagRegister = 0; // COSMAC-based variants reset VF
            break;

        case 2:
            xValue &= yValue;
            flagRegister = 0;
            break;

        case 3:
            xValue ^= yValue;
            flagRegister = 0;
            break;

        case 4: {
            const uint16_t sum = xValue + yValue;
            xValue = static_cast<uint8_t>(sum);
            flagRegister = sum > 0xFF;
            break;
        }

        case 5: {
            const uint8_t flag = xValue >= yValue;
            xValue -= yValue;
            flagRegister = flag;
            break;
        }

        case 6: {
            const uint8_t value = yValue;
            const uint8_t flag = value & 0x1;
            xValue = (value >> 1);
            flagRegister = flag;
            break;
        }
        case 7: {
            const uint8_t flag = yValue >= xValue;
            xValue = yValue - xValue;
            flagRegister = flag;
            break;
        }

        case 0xE: {
            const uint8_t value = yValue;
            const uint8_t flag = (value >> 7) & 0x1;
            xValue = static_cast<uint8_t>((value << 1));
            flagRegister = flag;
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
    using Storage = std::variant<ClearScreen, JumpToAddress, SaveToRegister,
                                 AddToRegister, SaveToIndexRegister, DrawSprite,
                                 TwoAddressInstruction, SkipIf, AddToIndexRegister,
                                 StoreBinaryCodedDecimal>;
    Storage instruction;

  public:
    template <class T>
    explicit AssemblyCommand(T decodedInstruction) // Czy nie trzeba dać tutaj T&&?
        : instruction(std::move(decodedInstruction)) {}

    template <class T>
    bool is() const {
        return std::holds_alternative<T>(instruction);
    }

    void execute(ChipData &data) {
        std::visit([&data](auto &decodedInstruction) { decodedInstruction.execute(data); },
                   instruction);
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
            return AssemblyCommand{JumpToAddress{command}};
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
            return AssemblyCommand{TwoAddressInstruction{command}};
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
        case 0xF000:
            if(Opcode::FF(command) == 0x1E)
                return AssemblyCommand{AddToIndexRegister{command}};
            if(Opcode::FF(command) == 0x33)
                return AssemblyCommand{StoreBinaryCodedDecimal{command}};
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
    {0xD123, matchesCommand<DrawSprite>},
    {0xF31E, matchesCommand<AddToIndexRegister>},
    {0xF533, matchesCommand<StoreBinaryCodedDecimal>},
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

    auto jumpToAddress = decoder.decode(0x1111);
    jumpToAddress.execute(data);
    ASSERT_EQ(data.programCounter, 0x0111);

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
}

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
        data.keyboard[keyData->keyIndex] = keyData->pressed;
    }

    auto command = decoder.decode(testCase.opcode);
    command.execute(data);

    ASSERT_EQ(data.programCounter, testCase.expectedProgramCounter);
}

INSTANTIATE_TEST_SUITE_P(
    MachineCode, SkipIfTest, testing::ValuesIn(skipIfCases),
    [](const auto &info) { return std::string(info.param.name); });

struct DrawSpriteTestCase {
    std::string_view name;
    uint16_t opcode;
    std::vector<uint8_t> sprite;
    std::string_view expectedSpriteString;
    uint8_t startingXPosition{};
    uint8_t startingYPosition{};
};
class DrawSpriteTest : public testing::TestWithParam<DrawSpriteTestCase> {
  protected:
    std::string readSprite(uint8_t xPosition, uint8_t yPosition, uint8_t rows,
                           const MonoScreen &screen) {
        std::string line{};
        std::string sprite{};
        std::stringstream screenStream{};
        screenStream << screen;
        size_t index{0};
        while(std::getline(screenStream, line) and
              (std::count(sprite.begin(), sprite.end(), '\n') < rows)) {
            if(index >= yPosition) {
                sprite.append(line.substr(xPosition, 8));
                sprite.push_back('\n');
            }
            index++;
        }
        return sprite;
    }
};

TEST_P(DrawSpriteTest, DrawSprite) {
    Decoder decoder;
    ChipData data;
    const auto &testCase = GetParam();

    // loading sprite to ram
    data.indexRegister = 0;
    for(size_t index = 0; uint8_t byte : testCase.sprite) {
        data.ram[index++] = byte;
    }

    // setup X and Y
    uint8_t xPosition = testCase.startingXPosition;
    uint8_t yPosition = testCase.startingYPosition;
    data.commonRegisters[Opcode::F00(testCase.opcode)] = xPosition;
    data.commonRegisters[Opcode::F0(testCase.opcode)] = yPosition;

    const auto rowCount = static_cast<uint8_t>(testCase.sprite.size());
    auto drawSprite = decoder.decode(testCase.opcode);
    drawSprite.execute(data);

    auto fetchedSprite = readSprite(xPosition, yPosition, rowCount, data.screen);

    ASSERT_EQ(testCase.expectedSpriteString, fetchedSprite);
    ASSERT_EQ(data.commonRegisters[0xF], 0);

    // clear screen with the same sprite
    drawSprite.execute(data);

    auto expectedSprite = [&testCase]() {
        std::string result{};
        for(size_t i = 0; i < testCase.sprite.size(); i++) {
            result.append("OOOOOOOO\n");
        }
        return result;
    }();
    fetchedSprite = readSprite(xPosition, yPosition, rowCount, data.screen);

    ASSERT_EQ(expectedSprite, fetchedSprite);
    ASSERT_EQ(data.commonRegisters[0xF], 1);
}

const auto drawSpriteCases = std::to_array<DrawSpriteTestCase>({
    {"Alien", 0xD234, {0x66, 0x7E, 0xC3, 0x99}, "O##OO##O\nO######O\n##OOOO##\n#OO##OO#\n", 1, 1},
    {"Smile", 0xD235, {0x42, 0x42, 0x00, 0x81, 0x7E}, "O#OOOO#O\nO#OOOO#O\nOOOOOOOO\n#OOOOOO#\nO######O\n", 20, 10},
});

INSTANTIATE_TEST_SUITE_P(
    MachineCode, DrawSpriteTest, testing::ValuesIn(drawSpriteCases),
    [](const auto &info) { return std::string(info.param.name); });

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
