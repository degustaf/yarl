#pragma once

#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>

#include <filesystem>
#include <memory>
#include <unordered_map>

#include "console.hpp"

using WindowPtr = std::unique_ptr<SDL_Window, decltype(&SDL_DestroyWindow)>;
using RendererPtr =
    std::unique_ptr<SDL_Renderer, decltype(&SDL_DestroyRenderer)>;
using FontPtr = std::unique_ptr<TTF_Font, decltype(&TTF_CloseFont)>;
using SurfacePtr = std::unique_ptr<SDL_Surface, decltype(&SDL_DestroySurface)>;
using TexturePtr = std::unique_ptr<SDL_Texture, decltype(&SDL_DestroyTexture)>;

using TextEnginePtr =
    std::unique_ptr<TTF_TextEngine, decltype(&TTF_DestroyRendererTextEngine)>;
using TextPtr = std::unique_ptr<TTF_Text, decltype(&TTF_DestroyText)>;

struct SDLData {
public:
  SDLData() = delete;
  SDLData(int columns, int rows, float fontSize, const char *title,
          const std::filesystem::path &fontPath);
  SDLData(int columns, int rows, float fontSize, const char *title,
          const std::filesystem::path &fontPath,
          const std::filesystem::path &imgPath);

  Console new_console(int width, int height);
  void accumulate(const struct Console &console, bool img);
  struct SDL_Renderer *renderer() { return _renderer.get(); };
  void convert_event_coordinates(SDL_Event &event);
  void resetCacheConsole(void);

  struct hash {
    uint64_t operator()(int n) const { return (uint64_t)n; };
  };

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
  cursor_transform_for_console_viewport(SDL_Renderer *renderer,
                                        const std::array<int, 2> &dims,
                                        const std::array<int, 2> &consoleDims);

  std::array<int, 2> dims;
  WindowPtr window;
  RendererPtr _renderer;
  FontPtr font;
  TextEnginePtr engine;
  std::unique_ptr<Console> cache_console;
  TexturePtr cache_texture;
  TexturePtr cover_texture;
  std::unordered_map<int, TextPtr, hash> cachedGlyphs;
  uint32_t sdl_subsystems; // Which subsystems where initialzed by this context.
  Transform cursor_transform;
};
