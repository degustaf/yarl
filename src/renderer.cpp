#include "renderer.hpp"

#include <cassert>
#include <cmath>
#include <cstdint>
#include <libtcod/renderer_sdl2.h>
#include <memory>
#include <stdexcept>

#include <SDL3/SDL.h>

static constexpr auto integer_scaling = false;
static constexpr auto keep_aspect = false;
static constexpr auto align_x = 0.5f;
static constexpr auto align_y = 0.5f;

#define BUFFER_TILES_MAX (65536 / 6)
// Max number of tiles to buffer to fit indices in a uint16_t type.

typedef struct VertexElement {
  VertexElement() : x(0.0f), y(0.0f), rgba({0.0f, 0.0f, 0.0f, 0.0f}) {};
  float x;
  float y;
  SDL_FColor rgba;
} VertexElement;

typedef struct VertexUV {
  VertexUV() : u(0.0f), v(0.0f) {};
  float u;
  float v;
} VertexUV;

static SDL_Rect get_atlas_tile(const TCOD_TilesetAtlasSDL2 &atlas,
                               int tile_id) {
  auto [y, x] = std::div(tile_id, atlas.texture_columns);
  return {x * atlas.tileset->tile_width, y * atlas.tileset->tile_height,
          atlas.tileset->tile_width, atlas.tileset->tile_height};
}

struct VertexBuffer {
  VertexBuffer() : index(0), indices_initialized(0) {
    for (auto &i : indices)
      i = 0;
    for (auto &v : vertex)
      v = VertexElement();
    for (auto &v : vertex_uv)
      v = VertexUV();
  };
  int16_t index; // Next tile to assign to.  Groups indicies in sets of 6
                 // and vertices in sets of 4.
  int16_t indices_initialized; // Sets of indicies initialized.
  std::array<uint16_t, BUFFER_TILES_MAX * 6>
      indices; // Vertex indices.  Vertex quads are
               // assigned as: 0 1 2, 2 1 3.
  std::array<VertexElement, BUFFER_TILES_MAX * 4> vertex;
  std::array<VertexUV, BUFFER_TILES_MAX * 4> vertex_uv;

  void push_bg(int x, int y, TCOD_ConsoleTile tile,
               const TCOD_TilesetAtlasSDL2 &atlas) {
    if (index == BUFFER_TILES_MAX)
      flush_bg(atlas);
    assert(index < BUFFER_TILES_MAX);
    set_tile_pos(index, x, y, *atlas.tileset);
    set_color(index, tile.bg);
    ++index;
  }

  void flush_bg(const TCOD_TilesetAtlasSDL2 &atlas) {
    sync_indices();
    SDL_SetRenderDrawBlendMode(atlas.renderer, SDL_BLENDMODE_NONE);
    SDL_RenderGeometryRaw(atlas.renderer,
                          NULL, // No texture, this renders solid colors.
                          &vertex[0].x, sizeof(vertex[0]), &vertex[0].rgba,
                          sizeof(vertex[0]), NULL, 0, index * 4, indices.data(),
                          index * 6, 2);
    index = 0;
  }

  void push_fg(int x, int y, TCOD_ConsoleTile tile,
               const TCOD_TilesetAtlasSDL2 &atlas, float u_multiply,
               float v_multiply) {
    if (index == BUFFER_TILES_MAX)
      flush_fg(atlas);
    assert(index < BUFFER_TILES_MAX);
    set_tile_pos(index, x, y, *atlas.tileset);
    set_color(index, tile.fg);
    const auto tile_id = atlas.tileset->character_map[tile.ch];
    const SDL_Rect src = get_atlas_tile(atlas, tile_id);
    vertex_uv[(uint16_t)index * 4 + 0].u = (float)(src.x) * u_multiply;
    vertex_uv[(uint16_t)index * 4 + 0].v = (float)(src.y) * v_multiply;
    vertex_uv[(uint16_t)index * 4 + 1].u = (float)(src.x) * u_multiply;
    vertex_uv[(uint16_t)index * 4 + 1].v = (float)(src.y + src.h) * v_multiply;
    vertex_uv[(uint16_t)index * 4 + 2].u = (float)(src.x + src.w) * u_multiply;
    vertex_uv[(uint16_t)index * 4 + 2].v = (float)(src.y) * v_multiply;
    vertex_uv[(uint16_t)index * 4 + 3].u = (float)(src.x + src.w) * u_multiply;
    vertex_uv[(uint16_t)index * 4 + 3].v = (float)(src.y + src.h) * v_multiply;
    ++index;
  }

  void flush_fg(const TCOD_TilesetAtlasSDL2 &atlas) {
    sync_indices();
    SDL_SetTextureBlendMode(atlas.texture, SDL_BLENDMODE_BLEND);
    SDL_RenderGeometryRaw(atlas.renderer, atlas.texture, &vertex[0].x,
                          sizeof(vertex[0]), &vertex[0].rgba, sizeof(vertex[0]),
                          (float *)vertex_uv.data(), sizeof(vertex_uv[0]),
                          index * 4, indices.data(), index * 6, 2);
    index = 0;
  }

private:
  void sync_indices() {
    for (; indices_initialized < index; ++indices_initialized) {
      indices[indices_initialized * 6 + 0] =
          (uint16_t)(indices_initialized * 4);
      indices[indices_initialized * 6 + 1] =
          (uint16_t)(indices_initialized * 4 + 1);
      indices[indices_initialized * 6 + 2] =
          (uint16_t)(indices_initialized * 4 + 2);
      indices[indices_initialized * 6 + 3] =
          (uint16_t)(indices_initialized * 4 + 2);
      indices[indices_initialized * 6 + 4] =
          (uint16_t)(indices_initialized * 4 + 1);
      indices[indices_initialized * 6 + 5] =
          (uint16_t)(indices_initialized * 4 + 3);
    }
  }

  void set_tile_pos(int idx, int x, int y, const TCOD_Tileset &tileset) {
    vertex[idx * 4 + 0].x = (float)(x * tileset.tile_width);
    vertex[idx * 4 + 0].y = (float)(y * tileset.tile_height);
    vertex[idx * 4 + 1].x = (float)(x * tileset.tile_width);
    vertex[idx * 4 + 1].y = (float)((y + 1) * tileset.tile_height);
    vertex[idx * 4 + 2].x = (float)((x + 1) * tileset.tile_width);
    vertex[idx * 4 + 2].y = (float)(y * tileset.tile_height);
    vertex[idx * 4 + 3].x = (float)((x + 1) * tileset.tile_width);
    vertex[idx * 4 + 3].y = (float)((y + 1) * tileset.tile_height);
  }

  void set_color(int idx, TCOD_ColorRGBA rgba) {
    SDL_FColor new_color = {
        (float)rgba.r * (1.0f / 255.0f), (float)rgba.g * (1.0f / 255.0f),
        (float)rgba.b * (1.0f / 255.0f), (float)rgba.a * (1.0f / 255.0f)};
    vertex[idx * 4 + 0].rgba = new_color;
    vertex[idx * 4 + 1].rgba = new_color;
    vertex[idx * 4 + 2].rgba = new_color;
    vertex[idx * 4 + 3].rgba = new_color;
  }
};

static SDL_ScaleMode get_sdl2_scale_mode_hint() {
  static const SDL_ScaleMode DEFAULT_SCALE_MODE = SDL_SCALEMODE_NEAREST;
  const auto scale_mode_hint = SDL_GetHint("SDL_RENDER_SCALE_QUALITY");
  if (!scale_mode_hint)
    return DEFAULT_SCALE_MODE;
  if (strcmp(scale_mode_hint, "0") == 0 ||
      strcmp(scale_mode_hint, "nearest") == 0)
    return SDL_SCALEMODE_NEAREST;
  return SDL_SCALEMODE_LINEAR;
}

static int cache_console_update(TCOD_TilesetObserver *observer, int tile_id) {
  auto console = (TCOD_Console *)observer->userdata;
  for (int c = 0; c < observer->tileset->character_map_length; ++c) {
    // Find codepoints that point to the tile_id.
    if (observer->tileset->character_map[c] != tile_id) {
      continue;
    }
    for (int i = 0; i < console->elements; ++i) {
      // Compare matched codepoints to the cache console characters.
      if (console->tiles[i].ch != c) {
        continue;
      }
      console->tiles[i].ch = -1;
    }
  }
  return 0;
}

static void cache_console_on_delete(TCOD_Console *console) { delete console; }

static void cache_console_observer_delete(TCOD_TilesetObserver *observer) {
  ((TCOD_Console *)observer->userdata)->userdata = NULL;
}

static void setup_cache_console(const TCOD_TilesetAtlasSDL2 &atlas,
                                const Console &console, Console *&cache) {
  if (cache) {
    assert(cache->get_width() != console.get_width());
    assert(cache->get_height() != console.get_height());
  }

  if (!cache) {
    cache = new Console(console.get_width(), console.get_height());
    auto observer = TCOD_tileset_observer_new(atlas.tileset);
    observer->userdata = cache;
    cache->userdata() = observer;
    observer->on_tile_changed = cache_console_update;
    cache->on_delete() = cache_console_on_delete;
    observer->on_observer_delete = cache_console_observer_delete;
    for (auto &tile : *cache)
      tile.ch = -1;
  }
}

static void render_texture_setup(const TCOD_TilesetAtlasSDL2 &atlas,
                                 const Console &console, Console *&cache,
                                 SDL_Texture *&target) {
  SDL_ScaleMode scale_mode = SDL_SCALEMODE_INVALID;

  if (target) {
    SDL_GetTextureScaleMode(target, &scale_mode);
    float tex_width;
    float tex_height;
    SDL_GetTextureSize(target, &tex_width, &tex_height);
    if ((int)tex_width != atlas.tileset->tile_width * console.get_width() ||
        (int)tex_height != atlas.tileset->tile_height * console.get_height()) {
      SDL_DestroyTexture(target);
      target = nullptr;
      if (cache) {
        delete cache;
        cache = NULL;
      }
    }
  }

  if (!target) {
    target = SDL_CreateTexture(
        atlas.renderer, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_TARGET,
        atlas.tileset->tile_width * console.get_width(),
        atlas.tileset->tile_height * console.get_height());
    if (!target) {
      throw std::runtime_error(
          std::string("Failed to create a new target texture. ") +
          SDL_GetError());
    }
    if (scale_mode == SDL_SCALEMODE_INVALID)
      scale_mode = get_sdl2_scale_mode_hint();
    if (scale_mode != SDL_SCALEMODE_INVALID)
      SDL_SetTextureScaleMode(target, scale_mode);
  }

  if (cache) {
    setup_cache_console(atlas, console, cache);
  }
}

TCOD_ConsoleTile normalize_tile_for_drawing(TCOD_ConsoleTile tile,
                                            const TCOD_Tileset &tileset) {
  if (tile.ch == 0x20)
    tile.ch = 0; // Tile is the space character.
  if (tile.ch < 0 || tile.ch >= tileset.character_map_length) {
    tile.ch = 0; // Tile character is out-of-bounds.
  } else if (tileset.character_map[tile.ch] == 0) {
    tile.ch = 0; // Ignore characters not defined in the tileset.
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

static void render(const TCOD_TilesetAtlasSDL2 &atlas, const Console &console,
                   Console *cache) {
  if (cache) {
    assert(cache->get_width() == console.get_width());
    assert(cache->get_height() != console.get_height());
  }

  auto buffer = new VertexBuffer;

  // The background rendering pass.
  for (int y = 0; y < console.get_height(); ++y) {
    for (int x = 0; x < console.get_width(); ++x) {
      const auto tile =
          normalize_tile_for_drawing(console.at({x, y}), *atlas.tileset);
      if (cache) {
        auto &cached = cache->at({x, y});
        // True if there are changes to the BG color.
        const auto bg_changed =
            tile.bg.r != cached.bg.r || tile.bg.g != cached.bg.g ||
            tile.bg.b != cached.bg.b || tile.bg.a != cached.bg.a;
        // True if there are changes to the FG glyph.
        const auto fg_changed =
            cached.ch && (tile.ch != cached.ch || tile.fg.r != cached.fg.r ||
                          tile.fg.g != cached.fg.g ||
                          tile.fg.b != cached.fg.b || tile.fg.a != cached.fg.a);
        if (!(bg_changed || fg_changed)) {
          continue; // If no changes exist then this tile can be skipped
                    // entirely.
        }
        // Cache the BG and unset the FG data, this will tell the FG pass if it
        // needs to draw the glyph.
        cached = TCOD_ConsoleTile{0, {0, 0, 0, 0}, tile.bg};
      }
      // Data is pushed onto the buffer, this is flushed automatically if it
      // would otherwise overflow.
      buffer->push_bg(x, y, tile, atlas);
    }
  }
  buffer->flush_bg(atlas);

  // The foreground rendering pass.
  float tex_width;
  float tex_height;
  SDL_GetTextureSize(atlas.texture, &tex_width, &tex_height);
  const float u_multiply =
      1.0f / (float)(tex_width); // Used to transform texture pixel coordinates
                                 // to UV coords.
  const float v_multiply = 1.0f / (float)(tex_height);
  for (int y = 0; y < console.get_height(); ++y) {
    for (int x = 0; x < console.get_width(); ++x) {
      const auto tile =
          normalize_tile_for_drawing(console.at({x, y}), *atlas.tileset);
      if (tile.ch == 0)
        continue; // No FG glyph to draw.
      if (cache) {
        auto &cached = cache->at({x, y});
        if (tile.ch == cached.ch) {
          continue; // The glyph has not changed since the last render.
        }
        // Cache the foreground glyph.
        cached.ch = tile.ch;
        cached.fg = tile.fg;
      }
      buffer->push_fg(x, y, tile, atlas, u_multiply, v_multiply);
    }
  }
  buffer->flush_fg(atlas);

  delete buffer;
}

static void render_texture(const TCOD_TilesetAtlasSDL2 &atlas,
                           const Console &console, Console *cache,
                           struct SDL_Texture *target) {
  if (!target) {
    return render(atlas, console, cache);
  }
  auto old_target = SDL_GetRenderTarget(atlas.renderer);
  SDL_SetRenderTarget(atlas.renderer, target);
  render(atlas, console, cache);
  SDL_SetRenderTarget(atlas.renderer, old_target);
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

SDLData::SDLData(int columns, int rows, const char *title,
                 TCOD_Tileset *tileset)
    : window(nullptr, nullptr), _renderer(nullptr, nullptr),
      atlas(nullptr, nullptr), cache_console(nullptr), cache_texture(nullptr) {

  SDL_PropertiesID window_props = SDL_CreateProperties();
  SDL_SetStringProperty(window_props, SDL_PROP_WINDOW_CREATE_TITLE_STRING,
                        title);
  SDL_SetNumberProperty(window_props, SDL_PROP_WINDOW_CREATE_X_NUMBER,
                        SDL_WINDOWPOS_UNDEFINED);
  SDL_SetNumberProperty(window_props, SDL_PROP_WINDOW_CREATE_Y_NUMBER,
                        SDL_WINDOWPOS_UNDEFINED);
  SDL_SetNumberProperty(window_props, SDL_PROP_WINDOW_CREATE_WIDTH_NUMBER,
                        columns * tileset->tile_width);
  SDL_SetNumberProperty(window_props, SDL_PROP_WINDOW_CREATE_HEIGHT_NUMBER,
                        rows * tileset->tile_height);
  SDL_SetNumberProperty(window_props, SDL_PROP_WINDOW_CREATE_FLAGS_NUMBER,
                        SDL_WINDOW_RESIZABLE);

  SDL_PropertiesID renderer_props = SDL_CreateProperties();
  SDL_SetNumberProperty(renderer_props,
                        SDL_PROP_RENDERER_CREATE_PRESENT_VSYNC_NUMBER, 1);

  sdl_subsystems = SDL_INIT_VIDEO;
  SDL_AddEventWatch(handle_event, this);
  window = decltype(window)(SDL_CreateWindowWithProperties(window_props),
                            SDL_DestroyWindow);

  if (!window) {
    throw std::runtime_error(std::string("Could not create SDL window:\n") +
                             SDL_GetError());
  }
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
  atlas =
      std::unique_ptr<TCOD_TilesetAtlasSDL2, decltype(&TCOD_sdl2_atlas_delete)>(
          TCOD_sdl2_atlas_new(_renderer.get(), tileset),
          TCOD_sdl2_atlas_delete);

  SDL_DestroyProperties(renderer_props);
  SDL_DestroyProperties(window_props);
}

Console SDLData::new_console(int width, int height) {
  int columns;
  int rows;
  int w;
  int h;
  if (!SDL_GetCurrentRenderOutputSize(_renderer.get(), &w, &h)) {
    throw ::std::runtime_error(std::string("SDL Error: ") + SDL_GetError());
  }
  if (atlas->tileset->tile_width != 0) {
    columns = (int)(w / atlas->tileset->tile_width);
  }
  if (atlas->tileset->tile_height != 0) {
    rows = (int)(h / atlas->tileset->tile_height);
  }
  return Console{std::max(columns, width), std::max(rows, height)};
};

static SDL_FRect get_destination_rect(const TCOD_TilesetAtlasSDL2 *atlas,
                                      int width, int height) {
  float output_w;
  float output_h;
  SDL_Texture *render_target = SDL_GetRenderTarget(atlas->renderer);
  if (render_target) {
    SDL_GetTextureSize(render_target, &output_w, &output_h);
  } else {
    int out_w_int;
    int out_h_int;
    SDL_GetCurrentRenderOutputSize(atlas->renderer, &out_w_int, &out_h_int);
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
get_destination_rect_for_console(const TCOD_TilesetAtlasSDL2 *atlas,
                                 const Console &console) {
  const auto tile_width = atlas->tileset->tile_width;
  const auto tile_height = atlas->tileset->tile_height;
  return get_destination_rect(atlas, console.get_width() * tile_width,
                              console.get_height() * tile_height);
}

SDLData::Transform SDLData::cursor_transform_for_console_viewport(
    const TCOD_TilesetAtlasSDL2 *atlas, const Console &console) {
  auto dest = get_destination_rect_for_console(atlas, console);
  return {
      dest.x,
      dest.y,
      (float)console.get_width() / dest.w,
      (float)console.get_height() / dest.h,
  };
}

void SDLData::accumulate(const Console &console) {
  render_texture_setup(*atlas, console, cache_console, cache_texture);
  render_texture(*atlas, console, cache_console, cache_texture);

  auto dest = get_destination_rect_for_console(atlas.get(), console);
  cursor_transform =
      cursor_transform_for_console_viewport(atlas.get(), console);
  SDL_RenderTexture(_renderer.get(), cache_texture, nullptr, &dest);
}

void SDLData::resetCacheConsole(void) {
  if (cache_console) {
    for (auto &tile : *cache_console) {
      tile = TCOD_ConsoleTile{-1, {}, {}};
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
