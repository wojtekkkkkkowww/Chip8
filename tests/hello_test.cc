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
                         DrawSprite };

class AssemblyCommand {
  public:
    virtual void execute(ChipData &data) = 0;
    virtual CommandName getName() = 0;
    virtual ~AssemblyCommand() = default;
};
// dziedziczenie po klasach jest private musi byc public
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
    JumpToAdress(uint16_t adress) : adress_(adress) {}
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
    SaveToRegister(uint8_t reg, uint8_t value) : register_(reg), value_(value) {
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
    SaveToIndexRegister(uint16_t value) : value_(value) {
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
    AddToRegister(uint8_t reg, uint8_t value) : register_(reg), value_(value) {
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
    DrawSprite(uint8_t xReg, uint8_t yReg, uint8_t rows) : xReg_(xReg), yReg_(yReg), rows_(rows) {
    }

    void execute(ChipData &data) override {

        uint8_t xPos = data.common_registers[xReg_] % MonoScreen::WIDTH;
        uint8_t yPos = data.common_registers[yReg_] % MonoScreen::HIGTH;

        uint8_t colisionRegister = 0xF;

        data.common_registers[colisionRegister] = 0;

        for(size_t i = 0; i < rows_; i++) {

            uint8_t spriteBite = data.ram[data.index_register + i];
            uint8_t currentYpos = (yPos + i) % MonoScreen::HIGTH;

            for(size_t j = 0; j < 8; j++) {

                uint8_t color = spriteBite & (0x1 << (7 - j));
                uint8_t currentXpos = (xPos + j) % MonoScreen::WIDTH;

                if(color == 1) {
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

class Decoder {
  public:
    std::unique_ptr<AssemblyCommand> decode(uint16_t command) {
        auto choseCommand = [command]() -> std::optional<std::unique_ptr<AssemblyCommand>> {
            switch(command & 0xF000) {
            case 0x1000:
                return std::make_unique<JumpToAdress>(command & 0x0FFF);
            case 0x0000:
                return std::make_unique<ClearScreen>();
            case 0x6000:
                return std::make_unique<SaveToRegister>((command & 0x0F00) >> 8, command & 0x00FF);
            case 0x7000:
                return std::make_unique<AddToRegister>((command & 0x0F00) >> 8, command & 0x00FF);
            case 0xA000:
                return std::make_unique<SaveToIndexRegister>(command & 0x0FFF);
            case 0xD000:
                return std::make_unique<DrawSprite>((command & 0x0F00) >> 8, (command & 0x00F0) >> 4, (command & 0x000F));
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

TEST(MachineCode, DrawSprite) {
    // deklaracja obrazka w ram
    Decoder decoder;
    ChipData data;
    auto drawScreen = decoder.decode(0xD114);
    drawScreen->execute(data);
    // print obrazka
    // sprawdzenie czy jest taki jak powinien
}
