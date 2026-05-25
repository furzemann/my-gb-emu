#include "cpu.h"
#include "display.h"
#include "memory.h"
#include "ppu.h"

#include <iostream>

int main() {

  Memory mem;

  if (!mem.loadROM("cpu_instrs.gb")) {

    std::cout << "failed rom\n";

    return 1;
  }

  CPU cpu(&mem);

  PPU ppu(&mem);

  Display display;

  bool running = true;

  while (running) {

    running = display.processEvents();

    int frameCycles = 0;

    while (frameCycles < 70224) {

      int cycles = cpu.step();

      ppu.step(cycles);

      frameCycles += cycles;
    }

    display.draw(ppu.getFrame());
  }

  return 0;
}
