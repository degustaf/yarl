#pragma once

#include <SDL3/SDL.h>
#include <libtcod.h>

#include <memory>

#include "console.hpp"

struct SDLData {
public:
  SDLData() = delete;
  SDLData(int columns, int rows, const char *title, TCOD_Tileset *tileset);

  Console new_console(int width, int height);
  void accumulate(const struct Console &console);
  struct SDL_Renderer *renderer() { return _renderer.get(); };
  void convert_event_coordinates(SDL_Event &event);
  void resetCacheConsole(void);

private:
  void pixel_to_tile(float &x, float &y);
  void pixel_to_tile(double &x, double &y);
  void pixel_to_tile(int &x, int &y);

  struct Transform {
    float offset_x;
    float offset_y;
    float scale_x;
    float scale_y;
  };
  static Transform
  cursor_transform_for_console_viewport(const TCOD_TilesetAtlasSDL2 *atlas,
                                        const Console &console);

  std::unique_ptr<SDL_Window, decltype(&SDL_DestroyWindow)> window;
  std::unique_ptr<SDL_Renderer, decltype(&SDL_DestroyRenderer)> _renderer;
  std::unique_ptr<TCOD_TilesetAtlasSDL2, decltype(&TCOD_sdl2_atlas_delete)>
      atlas;
  Console *cache_console; // Tracks the data from the last console presented.
  SDL_Texture *cache_texture; // Cached console rendering output.
  uint32_t sdl_subsystems; // Which subsystems where initialzed by this context.
  Transform cursor_transform;
};
