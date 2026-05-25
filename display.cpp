#include "display.h"

Display::Display() {

  SDL_Init(SDL_INIT_VIDEO);

  window = SDL_CreateWindow("GameBoy", SDL_WINDOWPOS_CENTERED,
                            SDL_WINDOWPOS_CENTERED, 160 * 4, 144 * 4,
                            SDL_WINDOW_SHOWN);

  renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

  texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888,
                              SDL_TEXTUREACCESS_STREAMING, 160, 144);
}

Display::~Display() {

  SDL_DestroyTexture(texture);

  SDL_DestroyRenderer(renderer);

  SDL_DestroyWindow(window);

  SDL_Quit();
}

bool Display::processEvents() {

  SDL_Event e;

  while (SDL_PollEvent(&e)) {

    if (e.type == SDL_QUIT)
      return false;
  }

  return true;
}

void Display::draw(const uint8_t *framebuffer) {

  for (int i = 0; i < 160 * 144; i++) {

    switch (framebuffer[i]) {

    case 0:

      pixels[i] = 0xFFFFFFFF;
      break;

    case 1:

      pixels[i] = 0xAAAAAAFF;
      break;

    case 2:

      pixels[i] = 0x555555FF;
      break;

    case 3:

      pixels[i] = 0x000000FF;
      break;

    default:

      pixels[i] = 0xFF0000FF;
    }
  }

  SDL_UpdateTexture(texture, nullptr, pixels, 160 * sizeof(uint32_t));

  SDL_RenderClear(renderer);

  SDL_RenderCopy(renderer, texture, nullptr, nullptr);

  SDL_RenderPresent(renderer);
}
