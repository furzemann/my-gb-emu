#pragma once

#include <cstdint>

class Memory {
public:
  Memory();

  uint8_t read(uint16_t addr);
  void write(uint16_t addr, uint8_t value);

private:
  uint8_t memory[65536];
};

// 0000-3FFF ROM bank 00
// 4000-7FFF ROM bank 01-NN
// 8000-9FFF 8KiB VRAM
// A000-BFFF 8KiB external RAM
// C000-CFFF WRAM
// D000-DFFF WRAM
// E000-FDFF Echo (unusable)
// FE00-FE9F OAM
// FEAO-FEFF Unsuable
// FF80-FF7F I/O registers
// FF80-FFFE High RAM
// FFFF-FFFF Interupt enabler
