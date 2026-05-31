#include "ppu.h"
#include <cstdio>

PPU::PPU(Memory *mem) { mmu = mem; }

void PPU::step(int cycles) {

  if (!(LCDC() & 0x80)) {

    modeClock = 0;

    setLY(0);

    setMode(HBLANK);

    return;
  }

  modeClock += cycles;

  switch (mode) {

  case OAM:

    if (modeClock >= 80) {

      modeClock -= 80;

      setMode(DRAW);
    }

    break;

  case DRAW:

    if (modeClock >= 172) {

      modeClock -= 172;

      renderScanline();

      setMode(HBLANK);
    }

    break;

  case HBLANK:

    if (modeClock >= 204) {

      modeClock -= 204;

      setLY(LY() + 1);

      if (LY() == 144) {

        setMode(VBLANK);

        requestInterrupt(0);

      } else {

        setMode(OAM);
      }
    }

    break;

  case VBLANK:

    if (modeClock >= 456) {

      modeClock -= 456;

      setLY(LY() + 1);

      if (LY() > 153) {

        setLY(0);

        setMode(OAM);
      }
    }

    break;
  }
}
uint8_t PPU::tilePixel(int tile, int x, int y, bool signedMode) {

  uint16_t tileAddr;

  if (!signedMode) {

    // LCDC bit4=1
    tileAddr = 0x8000 + tile * 16;
  } else {

    // LCDC bit4=0
    int8_t signedTile = (int8_t)tile;

    tileAddr = 0x9000 + signedTile * 16;
  }

  tileAddr += y * 2;

  uint8_t lo = mmu->read(tileAddr);

  uint8_t hi = mmu->read(tileAddr + 1);

  int bit = 7 - x;

  return (((hi >> bit) & 1) << 1) | ((lo >> bit) & 1);
}

void PPU::renderScanline() {

  uint8_t scy = mmu->read(0xFF42);

  uint8_t scx = mmu->read(0xFF43);

  uint8_t ly = LY();

  int y = (scy + ly) & 255;

  bool mapSelect = LCDC() & (1 << 3);

  uint16_t mapBase = mapSelect ? 0x9C00 : 0x9800;

  bool signedMode = !(LCDC() & (1 << 4));

  for (int x = 0; x < 160; x++) {

    int bgx = (scx + x) & 255;

    int tileX = bgx / 8;

    int tileY = y / 8;

    uint16_t mapAddr = mapBase + tileY * 32 + tileX;

    uint8_t tile = mmu->read(mapAddr);

    int pixelX = bgx % 8;

    int pixelY = y % 8;

    uint8_t color = tilePixel(tile, pixelX, pixelY, signedMode);
    color = applyPalette(color);
    framebuffer[ly * 160 + x] = color;
  }
  renderSprites();
}

void PPU::setMode(int m) {

  mode = m;

  uint8_t stat = mmu->read(0xFF41);

  stat &= ~0x3;

  stat |= m;

  mmu->write(0xFF41, stat);
}

uint8_t PPU::LY() { return mmu->read(0xFF44); }

void PPU::setLY(uint8_t v) { mmu->write(0xFF44, v); }

uint8_t PPU::LCDC() { return mmu->read(0xFF40); }

uint8_t PPU::STAT() { return mmu->read(0xFF41); }

void PPU::requestInterrupt(int bit) {

  uint8_t IF = mmu->read(0xFF0F);

  IF |= (1 << bit);

  mmu->write(0xFF0F, IF);
}

uint8_t PPU::applyPalette(uint8_t color) {

  uint8_t bgp = mmu->read(0xFF47);

  return (bgp >> (color * 2)) & 0x3;
}

void PPU::renderSprites() {

  uint8_t ly = LY();

  bool spriteSize = LCDC() & (1 << 2);

  int height = spriteSize ? 16 : 8;

  for (int i = 0; i < 40; i++) {

    uint16_t addr = 0xFE00 + i * 4;

    int y = mmu->read(addr) - 16;
    int x = mmu->read(addr + 1) - 8;

    uint8_t tile = mmu->read(addr + 2);

    uint8_t flags = mmu->read(addr + 3);

    bool flipX = flags & (1 << 5);
    bool flipY = flags & (1 << 6);

    // sprite not on this scanline
    if (ly < y || ly >= y + height)
      continue;

    int row = ly - y;

    if (flipY)
      row = height - 1 - row;

    uint16_t tileAddr = 0x8000 + tile * 16;

    tileAddr += row * 2;

    uint8_t lo = mmu->read(tileAddr);
    uint8_t hi = mmu->read(tileAddr + 1);

    for (int px = 0; px < 8; px++) {

      int bit = flipX ? px : (7 - px);

      uint8_t color = (((hi >> bit) & 1) << 1) | ((lo >> bit) & 1);

      // color 0 = transparent
      if (color == 0)
        continue;

      int screenX = x + px;

      if (screenX < 0 || screenX >= 160)
        continue;

      framebuffer[ly * 160 + screenX] = applyPalette(color);
    }
  }
}
