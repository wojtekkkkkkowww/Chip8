#include <gtest/gtest.h>

#include <array>

class MonoScreen {
 public:
  static const uint8_t WIDTH = 64;
  static const uint8_t HIGTH = 32;

 private:
  std::array<uint8_t, WIDTH * WIDTH> screen;

 public:
  MonoScreen() {
    for (auto& pixel : screen) {
      pixel = 0;
    }
  }

  void set(size_t i, size_t j) {
    if (i >= WIDTH or j >= WIDTH) {
      throw std::out_of_range("screen coordinates out of bands");
    }
    screen[i + j * WIDTH] = 1;
  }

  void reset(size_t i, size_t j) {
    if (i >= WIDTH or j >= WIDTH) {
      throw std::out_of_range("screen coordinates out of bands");
    }
    screen[i + j * WIDTH] = 0;
  }

  uint8_t get(size_t i, size_t j) {
    if (i >= WIDTH or j >= WIDTH) {
      throw std::out_of_range("screen coordinates out of bands");
    }
    return screen[i + j * WIDTH];
  }

  /* skad wiediec gdzie jest poczatek sprita??
  void drawSprite() {
  }
  */
};

struct ChipData {
  std::array<uint8_t, 4096> ram;
  std::array<uint8_t, 16> common_registers;
  uint16_t index_register{};
  uint16_t program_counter{};
  MonoScreen screen;
};

// Demonstrate some basic assertions.
TEST(HelloTest, BasicAssertions) {
  // Expect two strings not to be equal.
  EXPECT_STRNE("hello", "world");
  // Expect equality.
  EXPECT_EQ(7 * 6, 42);
}

TEST(ScreenTest, ScreenInit) {
  MonoScreen screen;

  ASSERT_EQ(screen.get(0, 0), 0);

  screen.set(3, 4);
  ASSERT_EQ(screen.get(3, 4), 1);

  screen.reset(3, 4);
  ASSERT_EQ(screen.get(3, 4), 0);
}

enum class CommandName { ClearScreen, JumpToAdress };

class AssemblyCommand {
 public:
  virtual void execute(ChipData& data) = 0;
  virtual CommandName getName() = 0;
  virtual ~AssemblyCommand() = default;
};
// dziedziczenie po klasach jest private musi byc public
class ClearScreen : public AssemblyCommand {
 public:
  void execute(ChipData& data) override {
    for (size_t i = 0; i < MonoScreen::WIDTH; i++) {
      for (size_t j = 0; j < MonoScreen::WIDTH; j++) {
        data.screen.reset(i, j);
      }
    }
  }
  CommandName getName() override { return CommandName::ClearScreen; }
};

class JumpToAdress : public AssemblyCommand {
 private:
  uint8_t adress_{};

 public:
  JumpToAdress(uint8_t adress) : adress_(adress) {}
  void execute(ChipData& data) override {}
  CommandName getName() override { return CommandName::JumpToAdress; }
};

class Decoder {
 public:
  std::unique_ptr<AssemblyCommand> decode(uint16_t command) {
    uint16_t mask = 0xF000;
    switch (command & mask) {
      case 0x1000:
        return std::make_unique<JumpToAdress>(0);
      case 0x0000:
        return std::make_unique<ClearScreen>();
    }
    throw std::runtime_error("Unknown Chip-8 opcode");

    return std::make_unique<ClearScreen>();
  }
};

TEST(MachineCode, DecodingTest) {
  Decoder decoder;
  auto command = decoder.decode(0x00E0);
  ASSERT_EQ(command->getName(), CommandName::ClearScreen);

  auto command2 = decoder.decode(0x1111);
  ASSERT_EQ(command2->getName(), CommandName::JumpToAdress);
}

TEST(MachineCode, ExecuteCommand) {
  Decoder decoder;
  ChipData data;

  data.screen.set(0, 0);
  auto command = decoder.decode(0x00E0);
  command->execute(data);
  ASSERT_EQ(data.screen.get(0, 0), 0);
}
