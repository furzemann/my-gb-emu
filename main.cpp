#include "cpu.h"
#include "display.h"
#include "memory.h"
#include "ppu.h"

#include <SDL2/SDL.h>
#include <iostream>

int main() {

  Memory mem;

  if (!mem.loadROM("rom.gb")) {

    std::cout << "failed rom\n";

    return 1;
  }

  CPU cpu(&mem);

  PPU ppu(&mem);

  Display display;

  bool running = true;

  while (running) {

    uint32_t frameStart = SDL_GetTicks();

    running = display.processEvents();

    int frameCycles = 0;

    while (frameCycles < 70224) {

      int cycles = cpu.step();

      mem.tick(cycles);

      ppu.step(cycles);

      frameCycles += cycles;
    }

    display.draw(ppu.getFrame());

    uint32_t frameTime = SDL_GetTicks() - frameStart;

    if (frameTime < 16)
      SDL_Delay(16 - frameTime);
  }

  return 0;
}
