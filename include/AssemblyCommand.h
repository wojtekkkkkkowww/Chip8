#pragma once

#include "DrawSprite.h"
#include "Instructions.h"
#include <utility>
#include <variant>

class AssemblyCommand {
  private:
    using Storage = std::variant<ClearScreen, JumpToAddress, SaveToRegister,
                                 AddToRegister, SaveToIndexRegister, DrawSprite,
                                 TwoAddressInstruction, SkipIf, AddToIndexRegister,
                                 StoreBinaryCodedDecimal, JumpWithOffset,
                                 TransferRegisters>;
    Storage instruction;

  public:
    template <class T>
    explicit AssemblyCommand(T decodedInstruction)
        : instruction(std::move(decodedInstruction)) {}

    template <class T>
    bool is() const {
        return std::holds_alternative<T>(instruction);
    }

    void execute(ChipData &data);
};
