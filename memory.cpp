
#include "memory.h"

#include <cstring>
#include <fstream>

Memory::Memory() {

  memset(rom, 0, sizeof(rom));
  memset(vram, 0, sizeof(vram));
  memset(eram, 0, sizeof(eram));
  memset(wram, 0, sizeof(wram));
  memset(oam, 0, sizeof(oam));
  memset(io, 0, sizeof(io));
  memset(hram, 0, sizeof(hram));

  // ===== Post boot values =====

  io[0x00] = 0xFF; // JOYP

  io[0x04] = 0x00; // DIV
  io[0x05] = 0x00; // TIMA
  io[0x06] = 0x00; // TMA
  io[0x07] = 0x00; // TAC

  io[0x40] = 0x93; // LCDC (LCD + BG + Sprites)
  io[0x41] = 0x00; // STAT
  io[0x42] = 0x00; // SCY
  io[0x43] = 0x00; // SCX
  io[0x44] = 0x00; // LY
  io[0x45] = 0x00; // LYC

  io[0x47] = 0xFC; // BGP
  io[0x48] = 0xFF; // OBP0
  io[0x49] = 0xFF; // OBP1

  io[0x4A] = 0x00; // WY
  io[0x4B] = 0x00; // WX

  io[0x0F] = 0xE1; // IF

  IE = 0x00;

  divCounter = 0;
  timerCounter = 0;
}

void Memory::tick(int cycles) {

  // ===== DIV =====

  divCounter += cycles;

  while (divCounter >= 256) {

    divCounter -= 256;

    io[0x04]++;
  }

  // ===== TIMA =====

  uint8_t tac = io[0x07];

  // timer disabled
  if (!(tac & 0x04))
    return;

  int freq = 1024;

  switch (tac & 0x03) {

  case 0:
    freq = 1024;
    break;
  case 1:
    freq = 16;
    break;
  case 2:
    freq = 64;
    break;
  case 3:
    freq = 256;
    break;
  }

  timerCounter += cycles;

  while (timerCounter >= freq) {

    timerCounter -= freq;

    uint8_t old = io[0x05];

    io[0x05]++;

    // overflow
    if (old == 0xFF) {

      io[0x05] = io[0x06];

      // request timer interrupt
      io[0x0F] |= 0x04;
    }
  }
}

uint8_t Memory::read(uint16_t addr) {

  // ROM
  if (addr <= 0x7FFF)
    return rom[addr];

  // VRAM
  else if (addr <= 0x9FFF)
    return vram[addr - 0x8000];

  // ERAM
  else if (addr <= 0xBFFF)
    return eram[addr - 0xA000];

  // WRAM
  else if (addr <= 0xDFFF)
    return wram[addr - 0xC000];

  // Echo RAM
  else if (addr <= 0xFDFF)
    return wram[addr - 0xE000];

  // OAM
  else if (addr <= 0xFE9F)
    return oam[addr - 0xFE00];

  // unusable
  else if (addr <= 0xFEFF)
    return 0xFF;

  // IO
  else if (addr >= 0xFF00 && addr <= 0xFF7F)
    return io[addr - 0xFF00];

  // HRAM
  else if (addr >= 0xFF80 && addr <= 0xFFFE)
    return hram[addr - 0xFF80];

  // IE
  else if (addr == 0xFFFF)
    return IE;

  return 0xFF;
}

void Memory::write(uint16_t addr, uint8_t value) {

  // ===== DIV reset =====

  if (addr == 0xFF04) {

    io[0x04] = 0;

    divCounter = 0;

    return;
  }

  // ===== DMA transfer =====

  if (addr == 0xFF46) {

    io[0x46] = value;

    uint16_t source = value << 8;

    for (int i = 0; i < 0xA0; i++) {

      oam[i] = read(source + i);
    }

    return;
  }

  // ROM
  if (addr <= 0x7FFF)
    return;

  // VRAM
  else if (addr <= 0x9FFF)
    vram[addr - 0x8000] = value;

  // ERAM
  else if (addr <= 0xBFFF)
    eram[addr - 0xA000] = value;

  // WRAM
  else if (addr <= 0xDFFF)
    wram[addr - 0xC000] = value;

  // Echo RAM
  else if (addr <= 0xFDFF)
    wram[addr - 0xE000] = value;

  // OAM
  else if (addr <= 0xFE9F)
    oam[addr - 0xFE00] = value;

  // unusable
  else if (addr <= 0xFEFF)
    return;

  // IO
  else if (addr >= 0xFF00 && addr <= 0xFF7F)
    io[addr - 0xFF00] = value;

  // HRAM
  else if (addr >= 0xFF80 && addr <= 0xFFFE)
    hram[addr - 0xFF80] = value;

  // IE
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
