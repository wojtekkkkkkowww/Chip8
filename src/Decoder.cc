#include "Decoder.h"
#include "Opcode.h"
#include <stdexcept>

AssemblyCommand Decoder::decode(uint16_t command) {
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
    case 0xB000:
        return AssemblyCommand{JumpWithOffset{command}};
    case 0xC000:
        return AssemblyCommand{Random{command}};
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
        if(Opcode::FF(command) == 0x07)
            return AssemblyCommand{ReadDelayTimer{command}};
        if(Opcode::FF(command) == 0x15)
            return AssemblyCommand{SetDelayTimer{command}};
        if(Opcode::FF(command) == 0x18)
            return AssemblyCommand{SetSoundTimer{command}};
        if(Opcode::FF(command) == 0x1E)
            return AssemblyCommand{AddToIndexRegister{command}};
        if(Opcode::FF(command) == 0x33)
            return AssemblyCommand{StoreBinaryCodedDecimal{command}};
        if(Opcode::FF(command) == 0x29)
            return AssemblyCommand{SetFontAddress{command}};
        if(Opcode::FF(command) == 0x55)
            return AssemblyCommand{
                TransferRegisters{command, RegisterTransferDirection::Store}};
        if(Opcode::FF(command) == 0x65)
            return AssemblyCommand{
                TransferRegisters{command, RegisterTransferDirection::Load}};
        break;
    }

    throw std::runtime_error("Unknown Chip-8 opcode");
}
