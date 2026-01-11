#include "renderer.hpp"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <memory>
#include <stdexcept>

#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>

static constexpr auto integer_scaling = false;
static constexpr auto keep_aspect = false;
static constexpr auto align_x = 0.5f;
static constexpr auto align_y = 0.5f;

static SDL_ScaleMode get_sdl2_scale_mode_hint() {
  static constexpr SDL_ScaleMode DEFAULT_SCALE_MODE = SDL_SCALEMODE_NEAREST;
  const auto scale_mode_hint = SDL_GetHint("SDL_RENDER_SCALE_QUALITY");
  if (!scale_mode_hint)
    return DEFAULT_SCALE_MODE;
  if (strcmp(scale_mode_hint, "0") == 0 ||
      strcmp(scale_mode_hint, "nearest") == 0)
    return SDL_SCALEMODE_NEAREST;
  return SDL_SCALEMODE_LINEAR;
}

static void setup_cache_console(const Console &console,
                                std::unique_ptr<Console> &cache) {
  if (cache) {
    assert(cache->get_width() != console.get_width());
    assert(cache->get_height() != console.get_height());
  }

  if (!cache) {
    cache =
        std::make_unique<Console>(console.get_width(), console.get_height());
    for (auto &tile : *cache)
      tile.ch = -1;
  }
}

static void render_texture_setup(SDL_Renderer *renderer,
                                 const std::array<int, 2> &dims,
                                 const Console &console,
                                 std::unique_ptr<Console> &cache,
                                 TexturePtr &target) {
  SDL_ScaleMode scale_mode = SDL_SCALEMODE_INVALID;

  if (target) {
    SDL_GetTextureScaleMode(target.get(), &scale_mode);
    float tex_width;
    float tex_height;
    SDL_GetTextureSize(target.get(), &tex_width, &tex_height);
    if ((int)tex_width != dims[0] * console.get_width() ||
        (int)tex_height != dims[1] * console.get_height()) {
      target = nullptr;
      if (cache) {
        cache = nullptr;
      }
    }
  }

  if (!target) {
    target = TexturePtr(SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA32,
                                          SDL_TEXTUREACCESS_TARGET,
                                          dims[0] * console.get_width(),
                                          dims[1] * console.get_height()),
                        SDL_DestroyTexture);
    if (!target) {
      throw std::runtime_error(
          std::string("Failed to create a new target texture. ") +
          SDL_GetError());
    }
    if (scale_mode == SDL_SCALEMODE_INVALID)
      scale_mode = get_sdl2_scale_mode_hint();
    if (scale_mode != SDL_SCALEMODE_INVALID)
      SDL_SetTextureScaleMode(target.get(), scale_mode);
  }

  if (cache) {
    setup_cache_console(console, cache);
  }
}

static Console::Tile normalize_tile_for_drawing(Console::Tile tile) {
  if (tile.ch < 0) {
    tile.ch = 0; // Tile character is out-of-bounds.
  }
  if (tile.fg.a == 0)
    tile.ch = 0; // No foreground alpha.
  if (tile.bg.r == tile.fg.r && tile.bg.g == tile.fg.g &&
      tile.bg.b == tile.fg.b && tile.bg.a == 255 && tile.fg.a == 255) {
    tile.ch = 0; // Foreground and background color match, so the foreground
                 // glyph would be invisible.
  }
  if (tile.ch == 0) {
    tile.fg.r = tile.fg.g = tile.fg.b = tile.fg.a =
        0; // Clear foreground color if the foreground glyph is skipped.
  }
  return tile;
}

static void render(const FontPtr &font, SDL_Renderer *renderer,
                   const Console &console, std::unique_ptr<Console> &cache) {
  if (cache) {
    assert(cache->get_width() == console.get_width());
    assert(cache->get_height() == console.get_height());
  }

  int target_width, target_height;
  SDL_GetRenderOutputSize(renderer, &target_width, &target_height);
  auto cell_width = target_width / console.get_width();
  auto cell_height = target_height / console.get_height();

  for (auto y = 0; y < console.get_height(); y++) {
    for (auto x = 0; x < console.get_width(); x++) {
      const auto tile = normalize_tile_for_drawing(console.at({x, y}));
      SDL_Color fg = {tile.fg.r, tile.fg.g, tile.fg.b, tile.fg.a};
      SDL_Color bg = {tile.bg.r, tile.bg.g, tile.bg.b, tile.bg.a};
      auto surface =
          TTF_RenderGlyph_Shaded(font.get(), (unsigned int)tile.ch, fg, bg);
      auto texture = SDL_CreateTextureFromSurface(renderer, surface);
      SDL_DestroySurface(surface);
      SDL_FRect dstRect{(float)(x * cell_width), (float)(y * cell_height),
                        (float)cell_width, (float)cell_height}; // x, y, w, h
      SDL_RenderTexture(renderer, texture, NULL, &dstRect);
      SDL_DestroyTexture(texture);
    }
  }
}

static void render_texture(const FontPtr &font, SDL_Renderer *renderer,
                           const Console &console,
                           std::unique_ptr<Console> &cache,
                           struct SDL_Texture *target) {
  if (!target) {
    return render(font, renderer, console, cache);
  }
  auto old_target = SDL_GetRenderTarget(renderer);
  SDL_SetRenderTarget(renderer, target);
  render(font, renderer, console, cache);
  SDL_SetRenderTarget(renderer, old_target);
}

static bool handle_event(void *userdata, SDL_Event *event) {
  SDLData *data = (SDLData *)userdata;
  switch (event->type) {
  case SDL_EVENT_RENDER_TARGETS_RESET:
    data->resetCacheConsole();
    break;
  }
  return 0;
}

SDLData::SDLData(int columns, int rows, float fontSize, const char *title,
                 const std::filesystem::path &fontPath)
    : dims({0, 0}), window(nullptr, nullptr), _renderer(nullptr, nullptr),
      font(nullptr, nullptr), cache_console(nullptr),
      cache_texture(nullptr, nullptr), cursor_transform({0, 0, 1, 1}) {

  assert(columns > 0);
  assert(rows > 0);
  assert(fontSize > 0);

  TTF_Init();
  font =
      FontPtr(TTF_OpenFont(fontPath.string().c_str(), fontSize), TTF_CloseFont);
  if (!font) {
    throw ::std::runtime_error(std::string("Could not open font ") +
                               fontPath.string() + " :\n" + SDL_GetError());
  }

  int w, h;
  TTF_GetStringSize(font.get(), "@", 1, &w, &h);
  dims = {w, h};

  SDL_PropertiesID window_props = SDL_CreateProperties();
  SDL_SetStringProperty(window_props, SDL_PROP_WINDOW_CREATE_TITLE_STRING,
                        title);
  SDL_SetNumberProperty(window_props, SDL_PROP_WINDOW_CREATE_X_NUMBER,
                        SDL_WINDOWPOS_UNDEFINED);
  SDL_SetNumberProperty(window_props, SDL_PROP_WINDOW_CREATE_Y_NUMBER,
                        SDL_WINDOWPOS_UNDEFINED);
  SDL_SetNumberProperty(window_props, SDL_PROP_WINDOW_CREATE_WIDTH_NUMBER,
                        columns * (int)w);
  SDL_SetNumberProperty(window_props, SDL_PROP_WINDOW_CREATE_HEIGHT_NUMBER,
                        rows * (int)h);
  SDL_SetNumberProperty(window_props, SDL_PROP_WINDOW_CREATE_FLAGS_NUMBER,
                        SDL_WINDOW_RESIZABLE);

  sdl_subsystems = SDL_INIT_VIDEO;
  SDL_AddEventWatch(handle_event, this);
  window = decltype(window)(SDL_CreateWindowWithProperties(window_props),
                            SDL_DestroyWindow);
  if (!window) {
    throw std::runtime_error(std::string("Could not create SDL window:\n") +
                             SDL_GetError());
  }

  SDL_PropertiesID renderer_props = SDL_CreateProperties();
  SDL_SetNumberProperty(renderer_props,
                        SDL_PROP_RENDERER_CREATE_PRESENT_VSYNC_NUMBER, 1);
  SDL_SetPointerProperty(renderer_props,
                         SDL_PROP_RENDERER_CREATE_WINDOW_POINTER, window.get());
  _renderer = decltype(_renderer)(
      SDL_CreateRendererWithProperties(renderer_props), SDL_DestroyRenderer);
  SDL_SetPointerProperty(renderer_props,
                         SDL_PROP_RENDERER_CREATE_WINDOW_POINTER, NULL);
  if (!_renderer) {
    throw ::std::runtime_error(std::string("Could not create SDL renderer:\n") +
                               SDL_GetError());
  }

  SDL_DestroyProperties(renderer_props);
  SDL_DestroyProperties(window_props);
}

Console SDLData::new_console(int width, int height) {
  int w;
  int h;
  if (!SDL_GetCurrentRenderOutputSize(_renderer.get(), &w, &h)) {
    throw ::std::runtime_error(std::string("SDL Error: ") + SDL_GetError());
  }
  int columns = (dims[0] == 0) ? 0 : (int)(w / dims[0]);
  int rows = (dims[1] == 0) ? 0 : (int)(h / dims[1]);
  return Console{std::max(columns, width), std::max(rows, height)};
};

static SDL_FRect get_destination_rect(struct SDL_Renderer *renderer, int width,
                                      int height) {
  float output_w;
  float output_h;
  SDL_Texture *render_target = SDL_GetRenderTarget(renderer);
  if (render_target) {
    SDL_GetTextureSize(render_target, &output_w, &output_h);
  } else {
    int out_w_int;
    int out_h_int;
    SDL_GetCurrentRenderOutputSize(renderer, &out_w_int, &out_h_int);
    output_w = (float)out_w_int;
    output_h = (float)out_h_int;
  }
  SDL_FRect out = {0, 0, output_w, output_h};
  float scale_w = (float)output_w / (float)width;
  float scale_h = (float)output_h / (float)height;
  if constexpr (integer_scaling) {
    scale_w = scale_w <= 1.0f ? scale_w : std::floor(scale_w);
    scale_h = scale_h <= 1.0f ? scale_h : std::floor(scale_h);
  }
  if (keep_aspect) {
    scale_w = scale_h = std::min(scale_w, scale_h);
  }
  out.w = (float)width * scale_w;
  out.h = (float)height * scale_h;
  out.x = (output_w - out.w) * std::clamp(align_x, 0.0f, 1.0f);
  out.y = (output_h - out.h) * std::clamp(align_y, 0.0f, 1.0f);
  return out;
}

static SDL_FRect
get_destination_rect_for_console(SDL_Renderer *renderer,
                                 const std::array<int, 2> &dims,
                                 const Console &console) {
  return get_destination_rect(renderer, console.get_width() * dims[0],
                              console.get_height() * dims[1]);
}

SDLData::Transform
SDLData::cursor_transform_for_console_viewport(SDL_Renderer *renderer,
                                               const std::array<int, 2> &dims,
                                               const Console &console) {
  auto dest = get_destination_rect_for_console(renderer, dims, console);
  return {
      dest.x,
      dest.y,
      (float)console.get_width() / dest.w,
      (float)console.get_height() / dest.h,
  };
}

void SDLData::accumulate(const Console &console) {
  render_texture_setup(_renderer.get(), dims, console, cache_console,
                       cache_texture);
  render_texture(font, _renderer.get(), console, cache_console,
                 cache_texture.get());

  auto dest = get_destination_rect_for_console(_renderer.get(), dims, console);
  cursor_transform =
      cursor_transform_for_console_viewport(_renderer.get(), dims, console);
  SDL_RenderTexture(_renderer.get(), cache_texture.get(), nullptr, &dest);
}

void SDLData::resetCacheConsole(void) {
  if (cache_console) {
    for (auto &tile : *cache_console) {
      tile = Console::Tile{-1, {}, {}};
    }
  }
}

void SDLData::convert_event_coordinates(SDL_Event &event) {
  switch (event.type) {
  case SDL_EVENT_MOUSE_MOTION: {
    int tile_x = (int)event.motion.x;
    int tile_y = (int)event.motion.y;
    int previous_tile_x = (int)(event.motion.x - event.motion.xrel);
    int previous_tile_y = (int)(event.motion.y - event.motion.yrel);
    pixel_to_tile(tile_x, tile_y);
    pixel_to_tile(previous_tile_x, previous_tile_y);
    event.motion.x = (float)tile_x;
    event.motion.y = (float)tile_y;
    event.motion.xrel = (float)(tile_x - previous_tile_x);
    event.motion.yrel = (float)(tile_y - previous_tile_y);
  } break;
  case SDL_EVENT_MOUSE_BUTTON_DOWN:
  case SDL_EVENT_MOUSE_BUTTON_UP: {
    int x = (int)event.button.x;
    int y = (int)event.button.y;
    pixel_to_tile(x, y);
    event.button.x = (float)x;
    event.button.y = (float)y;
  } break;
  default:
    break;
  }
};

void SDLData::pixel_to_tile(float &x, float &y) {
  SDL_RenderCoordinatesFromWindow(_renderer.get(), x, y, &x, &y);
  x = (float)((x - cursor_transform.offset_x) * cursor_transform.scale_x);
  y = (float)((y - cursor_transform.offset_y) * cursor_transform.scale_y);
}

void SDLData::pixel_to_tile(double &x, double &y) {
  auto fx = static_cast<float>(x);
  auto fy = static_cast<float>(y);
  SDL_RenderCoordinatesFromWindow(_renderer.get(), fx, fy, &fx, &fy);
  x = (fx - cursor_transform.offset_x) * cursor_transform.scale_x;
  y = (fy - cursor_transform.offset_y) * cursor_transform.scale_y;
}

void SDLData::pixel_to_tile(int &x, int &y) {
  auto dx = static_cast<double>(x);
  auto dy = static_cast<double>(y);
  pixel_to_tile(dx, dy);
  x = static_cast<int>(std::floor(dx));
  y = static_cast<int>(std::floor(dy));
}
