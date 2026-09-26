#include "AssemblyCommand.h"

void AssemblyCommand::execute(ChipData &data) {
    std::visit([&data](auto &decodedInstruction) { decodedInstruction.execute(data); },
               instruction);
}
