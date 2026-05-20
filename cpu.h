#pragma once

#include <cstdint>

class CPU {
public:
  uint8_t A, B, C, D, E, H, L, F;

  uint16_t SP;
  uint16_t PC;

  uint8_t step();
};
