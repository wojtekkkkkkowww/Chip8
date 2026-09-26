#pragma once

#include "AssemblyCommand.h"
#include <cstdint>

class Decoder {
  public:
    AssemblyCommand decode(uint16_t command);
};
