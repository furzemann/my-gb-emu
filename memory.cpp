#include "memory.h"

Memory::Memory() {
  for (int i = 0; i < 65536; i++) {
    memory[i] = 0;
  }
}

uint8_t Memory::read(uint16_t addr) { return memory[addr]; }

void Memory::write(uint16_t addr, uint8_t value) { memory[addr] = value; }

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
