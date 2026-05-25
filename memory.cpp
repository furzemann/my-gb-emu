#include "memory.h"
#include <fstream>

Memory::Memory() {
  memset(rom, 0, sizeof(rom));
  memset(vram, 0, sizeof(vram));
  memset(eram, 0, sizeof(eram));
  memset(wram, 0, sizeof(wram));
  memset(oam, 0, sizeof(oam));
  memset(io, 0, sizeof(io));
  memset(hram, 0, sizeof(hram));
  io[0x40] = 0x91; // LCDC
  io[0x42] = 0x00; // SCY
  io[0x43] = 0x00; // SCX
  io[0x44] = 0x00; // LY
  io[0x47] = 0xFC; // BG palette
  io[0x0F] = 0xE1; // IF
  IE = 0;
}

uint8_t Memory::read(uint16_t addr) {
  if (addr <= 0x7FFF)
    return rom[addr];

  else if (addr <= 0x9FFF)
    return vram[addr - 0x8000];

  else if (addr <= 0xBFFF)
    return eram[addr - 0xA000];

  else if (addr <= 0xDFFF)
    return wram[addr - 0xC000];

  // echo RAM
  else if (addr <= 0xFDFF)
    return wram[addr - 0xE000];

  else if (addr <= 0xFE9F)
    return oam[addr - 0xFE00];

  else if (addr >= 0xFF00 && addr <= 0xFF7F)
    return io[addr - 0xFF00];

  else if (addr >= 0xFF80 && addr <= 0xFFFE)
    return hram[addr - 0xFF80];

  else if (addr == 0xFFFF)
    return IE;

  return 0xFF;
}

void Memory::write(uint16_t addr, uint8_t value) {
  if (addr <= 0x7FFF)
    return;

  else if (addr <= 0x9FFF)
    vram[addr - 0x8000] = value;

  else if (addr <= 0xBFFF)
    eram[addr - 0xA000] = value;

  else if (addr <= 0xDFFF)
    wram[addr - 0xC000] = value;

  else if (addr <= 0xFDFF)
    wram[addr - 0xE000] = value;

  else if (addr <= 0xFE9F)
    oam[addr - 0xFE00] = value;

  else if (addr >= 0xFF00 && addr <= 0xFF7F)
    io[addr - 0xFF00] = value;

  else if (addr >= 0xFF80 && addr <= 0xFFFE)
    hram[addr - 0xFF80] = value;

  else if (addr == 0xFFFF)
    IE = value;
}

bool Memory::loadROM(const char *filename) {
  std::ifstream file(filename, std::ios::binary);

  if (!file)
    return false;

  file.read((char *)rom, sizeof(rom));

  return true;
}

// 0000-3FFF ROM bank 00
// 4000-7FFF ROM bank 01-NN
// 8000-9FFF 8KiB VRAM
// A000-BFFF 8KiB external RAM
// C000-CFFF WRAM
// D000-DFFF WRAM
// E000-FDFF Echo (unusable)
// FE00-FE9F OAM
// FEAO-FEFF Unsuable
// FF00-FF7F I/O registers
// FF80-FFFE High RAM
// FFFF-FFFF Interupt enabler
