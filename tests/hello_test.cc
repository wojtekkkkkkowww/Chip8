#include <gtest/gtest.h>
#include <optional>

#include "MonoScreen.h"
#include <array>

struct ChipData {
    std::array<uint8_t, 4096> ram;
    std::array<uint8_t, 16> common_registers;
    uint16_t index_register{};
    uint16_t program_counter{};
    MonoScreen screen;
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

enum class CommandName { ClearScreen,
                         JumpToAdress,
                         SaveToRegister,
                         AddToRegister,
                         SaveToIndexRegister,
                         DrawSprite,
                         TwoAdressInstruction
};

class AssemblyCommand {
  public:
    virtual void execute(ChipData &data) = 0;
    virtual CommandName getName() = 0;
    virtual ~AssemblyCommand() = default;
    
    static constexpr uint16_t FFF(uint16_t opcode){
        return opcode & 0x0FFF;
    }
    static constexpr uint8_t FF(uint16_t opcode) {
        return opcode & 0x00FF;
    }
    static constexpr uint8_t F(uint16_t opcode) {
        return opcode & 0x000F;
    }
    // >> has biger prio than &
    static constexpr uint8_t F000(uint16_t opcode) {
        return (opcode & 0xF000) >> 12;
    }
    static constexpr uint8_t F00(uint16_t opcode) {
        return (opcode & 0x0F00) >> 8;
    }
    static constexpr uint8_t F0(uint16_t opcode) {
        return (opcode & 0x00F0) >> 4;
    }


};

class ClearScreen : public AssemblyCommand {
  public:
    void execute(ChipData &data) override {
        for(size_t i = 0; i < MonoScreen::WIDTH; i++) {
            for(size_t j = 0; j < MonoScreen::WIDTH; j++) {
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
    void execute(ChipData &data) override {
        data.program_counter = adress_;
    }
    CommandName getName() override { return CommandName::JumpToAdress; }
};

class SaveToRegister : public AssemblyCommand {
  private:
    uint8_t register_{};
    uint8_t value_{};
  public:
    SaveToRegister(uint16_t command) : register_(F00(command)), value_(FF(command)) {
    }

    void execute(ChipData &data) override {
        data.common_registers[register_] = value_;
    }

    CommandName getName() override { return CommandName::SaveToRegister; }
};

class SaveToIndexRegister : public AssemblyCommand {
  private:
    uint16_t value_{};

  public:
    SaveToIndexRegister(uint16_t command) : value_(FFF(command)) {
    }

    void execute(ChipData &data) override {
        data.index_register = value_;
    }

    CommandName getName() override { return CommandName::SaveToIndexRegister; }
};

class AddToRegister : public AssemblyCommand {
  private:
    uint8_t register_{};
    uint8_t value_{};

  public:
    AddToRegister(uint16_t command) : register_(F00(command)), value_(FF(command)) {
    }

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

  public:
    DrawSprite(uint16_t command) : xReg_(F00(command)), yReg_(F0(command)), rows_(F(command)) {
    }

    void execute(ChipData &data) override {

        uint8_t xPos = data.common_registers[xReg_] % MonoScreen::WIDTH;
        uint8_t yPos = data.common_registers[yReg_] % MonoScreen::HIGTH;

        uint8_t colisionRegister = 0xF;

        data.common_registers[colisionRegister] = 0;

        for(size_t i = 0; i < rows_; i++) {

            uint8_t spriteByte = data.ram[data.index_register + i];
            uint8_t currentYpos = (yPos + i) % MonoScreen::HIGTH;

            for(size_t j = 0; j < 8; j++) {

                uint8_t currentXpos = (xPos + j) % MonoScreen::WIDTH;
                uint8_t color = spriteByte & (0x1 << (7 - j));

                if(color > 0) {
                    if(data.screen.get(currentXpos, currentYpos) == 1) {
                        data.common_registers[colisionRegister] = 1;
                        data.screen.reset(currentXpos, currentYpos);
                    } else {
                        data.screen.set(currentXpos, currentYpos);
                    }
                }
            }
        }
    }

    CommandName getName() override { return CommandName::DrawSprite; }
};

class TwoAdressInstruction : public AssemblyCommand{
  private:
    uint8_t xReg_{};
    uint8_t yReg_{};
    uint8_t operationType_{};
  public:
    TwoAdressInstruction(uint16_t command) : xReg_(F00(command)), yReg_(F0(command)), operationType_(F(command)) {
    }
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
            break;

        case 2:
            xVal &= yVal;
            break;

        case 3:
            xVal ^= yVal;
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
        }
    }
    CommandName getName() override { return CommandName::TwoAdressInstruction; }
};

class Decoder {
  public:
    std::unique_ptr<AssemblyCommand> decode(uint16_t command) {
        auto choseCommand = [command]() -> std::optional<std::unique_ptr<AssemblyCommand>> {
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
            return std::nullopt;
        };

        auto assemblyCommand = choseCommand();
        if(not assemblyCommand.value()) {

            throw std::runtime_error("Unknown Chip-8 opcode");
        }

        return std::move(assemblyCommand.value());
    }
};

TEST(MachineCode, DecodingTest) {
    Decoder decoder;
    auto command = decoder.decode(0x00E0);
    ASSERT_EQ(command->getName(), CommandName::ClearScreen);

    auto command2 = decoder.decode(0x1111);
    ASSERT_EQ(command2->getName(), CommandName::JumpToAdress);

    auto command3 = decoder.decode(0x6111);
    ASSERT_EQ(command3->getName(), CommandName::SaveToRegister);

    auto command4 = decoder.decode(0x7111);
    ASSERT_EQ(command4->getName(), CommandName::AddToRegister);

    auto command5 = decoder.decode(0xA111);
    ASSERT_EQ(command5->getName(), CommandName::SaveToIndexRegister);

    auto command6 = decoder.decode(0xD111);
    ASSERT_EQ(command6->getName(), CommandName::DrawSprite);

    auto command7 = decoder.decode(0x8000);
    ASSERT_EQ(command7->getName(), CommandName::TwoAdressInstruction);
}

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

std::string readSprite(uint8_t xPos, uint8_t yPos, uint8_t rows, const MonoScreen &screen) {

    std::string line{};
    std::string sprite{};
    std::stringstream ss{};
    ss << screen;
    size_t index{0};
    while(std::getline(ss, line) and (std::count(sprite.begin(), sprite.end(), '\n') < rows)) {
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


TEST(MachineCode, TestTwoAdressInstruction) {
    ChipData data;
    Decoder decoder;

    data.common_registers[1] = 3;
    data.common_registers[2] = 4;
    auto sub = decoder.decode(0x8125);
    sub->execute(data);
    ASSERT_EQ(data.common_registers[1], 0xFF);
    ASSERT_EQ(data.common_registers[0xF], 0);

}
