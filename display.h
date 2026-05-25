#pragma once

#include <SDL2/SDL.h>
#include <cstdint>

class Display {
public:
  Display();
  ~Display();

  bool processEvents();

  void draw(const uint8_t *framebuffer);

private:
  SDL_Window *window;

  SDL_Renderer *renderer;

  SDL_Texture *texture;

  uint32_t pixels[160 * 144];
};
