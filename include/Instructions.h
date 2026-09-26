#pragma once

#include <cstdint>

struct ChipData;

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
    SkipIf(uint16_t rawOpcode, SkipCondition skipCondition);
    void execute(ChipData &data);
};

class ClearScreen {
  public:
    void execute(ChipData &data);
};

class JumpToAddress {
  private:
    uint16_t address{};

  public:
    JumpToAddress(uint16_t command);
    void execute(ChipData &data);
};

class JumpWithOffset {
  private:
    uint16_t address{};

  public:
    JumpWithOffset(uint16_t command);
    void execute(ChipData &data);
};

class SaveToRegister {
  private:
    uint8_t registerIndex{};
    uint8_t value{};

  public:
    SaveToRegister(uint16_t command);

    void execute(ChipData &data);
};

class SaveToIndexRegister {
  private:
    uint16_t value{};

  public:
    SaveToIndexRegister(uint16_t command);

    void execute(ChipData &data);
};

class AddToIndexRegister {
  private:
    uint8_t registerIndex{};

  public:
    AddToIndexRegister(uint16_t command);

    void execute(ChipData &data);
};

enum class RegisterTransferDirection { Store,
                                       Load };

class TransferRegisters {
  private:
    uint8_t registerIndex{};
    RegisterTransferDirection direction;

  public:
    TransferRegisters(uint16_t command, RegisterTransferDirection transferDirection);

    void execute(ChipData &data);
};

class StoreBinaryCodedDecimal {
  private:
    uint8_t registerIndex{};

  public:
    StoreBinaryCodedDecimal(uint16_t command);

    void execute(ChipData &data);
};

class AddToRegister {
  private:
    uint8_t registerIndex{};
    uint8_t value{};

  public:
    AddToRegister(uint16_t command);

    void execute(ChipData &data);
};

class TwoAddressInstruction {
  private:
    uint8_t xRegisterIndex{};
    uint8_t yRegisterIndex{};
    uint8_t operationType{};

  public:
    TwoAddressInstruction(uint16_t command);
    void execute(ChipData &data);
};
