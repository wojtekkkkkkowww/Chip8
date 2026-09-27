#include "Instructions.h"
#include "ChipData.h"
#include "Font.h"
#include "Opcode.h"
#include <cstddef>
#include <stdexcept>

SkipIf::SkipIf(uint16_t rawOpcode, SkipCondition skipCondition)
    : condition(skipCondition), opcode(rawOpcode) {}

void SkipIf::execute(ChipData &data) {
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

void ClearScreen::execute(ChipData &data) {
    for(size_t i = 0; i < MonoScreen::WIDTH; i++) {
        for(size_t j = 0; j < MonoScreen::HEIGHT; j++) {
            data.screen.reset(i, j);
        }
    }
}

JumpToAddress::JumpToAddress(uint16_t command) : address(Opcode::FFF(command)) {}

void JumpToAddress::execute(ChipData &data) { data.programCounter = address; }

JumpWithOffset::JumpWithOffset(uint16_t command) : address(Opcode::FFF(command)) {}

void JumpWithOffset::execute(ChipData &data) {
    data.programCounter = address + data.commonRegisters[0];
}

SaveToRegister::SaveToRegister(uint16_t command)
    : registerIndex(Opcode::F00(command)), value(Opcode::FF(command)) {}

void SaveToRegister::execute(ChipData &data) {
    data.commonRegisters[registerIndex] = value;
}

SaveToIndexRegister::SaveToIndexRegister(uint16_t command) : value(Opcode::FFF(command)) {}

void SaveToIndexRegister::execute(ChipData &data) { data.indexRegister = value; }

AddToIndexRegister::AddToIndexRegister(uint16_t command) : registerIndex(Opcode::F00(command)) {}

void AddToIndexRegister::execute(ChipData &data) {
    data.indexRegister += data.commonRegisters[registerIndex];
}

TransferRegisters::TransferRegisters(uint16_t command, RegisterTransferDirection transferDirection)
    : registerIndex(Opcode::F00(command)), direction(transferDirection) {}

void TransferRegisters::execute(ChipData &data) {
    for(size_t i = 0; i <= registerIndex; ++i) {
        if(direction == RegisterTransferDirection::Store)
            data.ram.at(data.indexRegister + i) = data.commonRegisters[i];
        else
            data.commonRegisters[i] = data.ram.at(data.indexRegister + i);
    }
}

StoreBinaryCodedDecimal::StoreBinaryCodedDecimal(uint16_t command) : registerIndex(Opcode::F00(command)) {}

void StoreBinaryCodedDecimal::execute(ChipData &data) {
    const uint8_t value = data.commonRegisters[registerIndex];
    data.ram[data.indexRegister] = value / 100;
    data.ram[data.indexRegister + 1] = (value / 10) % 10;
    data.ram[data.indexRegister + 2] = value % 10;
}

AddToRegister::AddToRegister(uint16_t command)
    : registerIndex(Opcode::F00(command)), value(Opcode::FF(command)) {}

void AddToRegister::execute(ChipData &data) {
    data.commonRegisters[registerIndex] += value;
}

SetFontAddress::SetFontAddress(uint16_t command) : registerIndex(Opcode::F00(command)) {}

void SetFontAddress::execute(ChipData &data) {
    data.indexRegister = static_cast<uint16_t>(
        Font::START_ADDRESS + data.commonRegisters[registerIndex] * Font::BYTES_PER_GLYPH);
}

Random::Random(uint16_t command)
    : registerIndex(Opcode::F00(command)), mask(Opcode::FF(command)) {}

void Random::execute(ChipData &data) {
    std::uniform_int_distribution<unsigned int> distribution{0, 255};
    data.commonRegisters[registerIndex] =
        static_cast<uint8_t>(distribution(data.randomGenerator) & mask);
}

ReadDelayTimer::ReadDelayTimer(uint16_t command) : registerIndex(Opcode::F00(command)) {}

void ReadDelayTimer::execute(ChipData &data) {
    data.commonRegisters[registerIndex] = data.delayTimer;
}

SetDelayTimer::SetDelayTimer(uint16_t command) : registerIndex(Opcode::F00(command)) {}

void SetDelayTimer::execute(ChipData &data) {
    data.delayTimer = data.commonRegisters[registerIndex];
}

SetSoundTimer::SetSoundTimer(uint16_t command) : registerIndex(Opcode::F00(command)) {}

void SetSoundTimer::execute(ChipData &data) {
    data.soundTimer = data.commonRegisters[registerIndex];
}

TwoAddressInstruction::TwoAddressInstruction(uint16_t command)
    : xRegisterIndex(Opcode::F00(command)), yRegisterIndex(Opcode::F0(command)),
      operationType(Opcode::F(command)) {}

void TwoAddressInstruction::execute(ChipData &data) {
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
