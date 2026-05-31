#pragma once

#include <cstdint>
#include <cstring>

class Memory {
public:
  Memory();

  uint8_t read(uint16_t addr);
  void write(uint16_t addr, uint8_t value);

  bool loadROM(const char *filename);

  // advance hardware timers by given CPU cycles
  void tick(int cycles);

private:
  uint8_t rom[0x8000];
  int divCounter = 0;
  int timerCounter = 0;
  uint8_t vram[0x2000];

  uint8_t eram[0x2000];

  uint8_t wram[0x2000];

  uint8_t oam[0xA0];

  uint8_t io[0x80];

  uint8_t hram[0x7F];

  uint8_t IE;

  // internal timer counters (cycle accumulators)
  int div_counter = 0;
  int tima_counter = 0;
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
