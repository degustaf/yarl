#define SDL_MAIN_USE_CALLBACKS
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <SDL3_ttf/SDL_ttf.h>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

#include <flecs.h>
#include <libtcod.hpp>

#include "defines.hpp"
#include "engine.hpp"
#include "input_handler.hpp"
#include "module.hpp"
#include "renderer.hpp"

static constexpr auto clear_color = TCOD_ColorRGBA{0, 0, 0, 255};

SDL_AppResult SDL_AppInit(void **data, [[maybe_unused]] int argc,
                          [[maybe_unused]] char **argv) {
  static constexpr auto width = 160;
  static constexpr auto height = 50;

#ifdef __EMSCRIPTEN__
  EM_ASM(const save_dir = UTF8ToString($0); FS.mkdir(save_dir);
         FS.mount(IDBFS, {autoPersist : true}, save_dir);
         FS.syncfs(
             true,
             function(err) {
               assert(!err);
               console.log("finished initiating IDBFS");
             }),
         data_dir.c_str());
#endif

  auto *ecs = new flecs::world();
  *data = ecs;
  ecs->import <module>();
  ecs->emplace<SDLData>(width, height, 15.0f, "Yet Another Roguelike",
                        "assets/CodeNewRoman.ttf");
  ecs->set<Console>(ecs->get_mut<SDLData>().new_console(width, height));
  ecs->emplace<EventHandler>(std::array{width, height});

#if !defined NDEBUG
  ecs->import <flecs::stats>();
  ecs->set<flecs::Rest>({});
#endif

  if (!std::filesystem::exists(data_dir) ||
      !std::filesystem::is_directory(data_dir)) {
    std::filesystem::create_directory(data_dir);
  }

  assert(ecs->get<EventHandler>().mouse_loc[0] >= 0);
  assert(ecs->get<EventHandler>().mouse_loc[1] >= 0);

  return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void *appstate) {
  auto ecs = *static_cast<flecs::world *>(appstate);
  auto &console = ecs.get_mut<Console>();
  console.clear();
  auto &eventHandler = ecs.get_mut<EventHandler>();
  assert(eventHandler.mouse_loc[0] >= 0);
  assert(eventHandler.mouse_loc[1] >= 0);
  (eventHandler.*(eventHandler.on_render))(ecs, console, SDL_GetTicks());
  auto &data = ecs.get_mut<SDLData>();
  auto renderer = data.renderer();
  SDL_SetRenderTarget(renderer, nullptr);
  SDL_SetRenderDrawColor(renderer, clear_color.r, clear_color.g, clear_color.b,
                         clear_color.a);
  SDL_RenderClear(renderer);
  data.accumulate(console);
  SDL_RenderPresent(renderer);

#if !defined NDEBUG
  ecs.progress();
#endif
  return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event) {
  auto &ecs = *static_cast<flecs::world *>(appstate);
  auto &eh = ecs.get_mut<EventHandler>();
  ecs.get_mut<SDLData>().convert_event_coordinates(*event);
  return (eh.*(eh.handle_action))(ecs, eh.dispatch(event, ecs));
}

static void delete_file(std::filesystem::path file) {
  std::remove(file.string().c_str());
}

void SDL_AppQuit(void *data, SDL_AppResult result) {
  auto ecs = static_cast<flecs::world *>(data);
  if (result == SDL_APP_FAILURE) {
    delete_file(data_dir / saveFilename);
  } else {
    // TODO handle game not started.
    Engine::save_as(*ecs, data_dir / saveFilename);
  }
  ecs->release();
  delete ecs;
  TTF_Quit();
  SDL_Quit();
}
