#define SDL_MAIN_USE_CALLBACKS
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include <flecs.h>
#include <libtcod.hpp>

#include "defines.hpp"
#include "engine.hpp"
#include "input_handler.hpp"
#include "module.hpp"

SDL_AppResult SDL_AppInit(void **data, [[maybe_unused]] int argc,
                          [[maybe_unused]] char **argv) {
  int width = 80;
  int height = 50;

  // Configure the context.
  auto params = TCOD_ContextParams{};

  tcod::Tileset tileset = tcod::load_tilesheet("assets/dejavu10x10_gs_tc.png",
                                               {32, 8}, tcod::CHARMAP_TCOD);
  params.tcod_version = TCOD_COMPILEDVERSION; // This is required.
  params.columns = width;
  params.rows = height;
  params.tileset = tileset.get();
  params.window_title = "Yet Another Roguelike";
  params.sdl_window_flags = SDL_WINDOW_RESIZABLE;
  params.vsync = true;

  auto *ecs = new flecs::world();
  *data = ecs;
  ecs->import <module>();
  ecs->set<tcod::Context>(tcod::Context(params));
  ecs->set<tcod::Console>(ecs->get_mut<tcod::Context>().new_console());
  ecs->add<EventHandler>();

  return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void *appstate) {
  auto ecs = *static_cast<flecs::world *>(appstate);
  auto &console = ecs.get_mut<tcod::Console>();
  console.clear();
  auto &eventHandler = ecs.get_mut<EventHandler>();
  (eventHandler.*(eventHandler.on_render))(ecs, console);
  ecs.get_mut<tcod::Context>().present(console);
  return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event) {
  auto ecs = *static_cast<flecs::world *>(appstate);
  auto &eh = ecs.get_mut<EventHandler>();
  ecs.get_mut<tcod::Context>().convert_event_coordinates(*event);
  return (eh.*(eh.handle_action))(ecs, eh.dispatch(event, ecs));
}

static void delete_file(std::filesystem::path file) {
  std::remove(file.string().c_str());
}

void SDL_AppQuit(void *data, SDL_AppResult result) {
  auto ecs = static_cast<flecs::world *>(data);
  if (result == SDL_APP_FAILURE) {
    delete_file(saveFilename);
  } else {
    // TODO handle game not started.
    Engine::save_as(*ecs, saveFilename);
  }
  // ecs->release();
  // delete ecs;
}
