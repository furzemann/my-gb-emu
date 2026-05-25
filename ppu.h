#pragma once

#include "memory.h"
#include <cstdint>

class PPU {
public:
  PPU(Memory *mem);

  void step(int cycles);

  // SDL will read framebuffer through this
  const uint8_t *getFrame() const { return framebuffer; }

private:
  Memory *mmu;

  enum { HBLANK = 0, VBLANK = 1, OAM = 2, DRAW = 3 };

  int mode = OAM;
  int modeClock = 0;

  uint8_t framebuffer[160 * 144];

  void renderScanline();

  uint8_t tilePixel(int tile, int x, int y, bool signedMode);

  void setMode(int m);

  uint8_t LY();
  void setLY(uint8_t v);

  uint8_t LCDC();
  uint8_t STAT();

  void requestInterrupt(int bit);
};
;
