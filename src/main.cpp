#include <SDL3/SDL_init.h>
#include <libtcod/color.hpp>
#define SDL_MAIN_USE_CALLBACKS
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include <flecs.h>
#include <libtcod.hpp>

#include "action.hpp"

int player_x = 0;
int player_y = 0;

SDL_AppResult SDL_AppInit(void **data, [[maybe_unused]] int argc,
                          [[maybe_unused]] char **argv) {
  int width = 100;
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
  ecs->set<tcod::Context>(tcod::Context(params));
  ecs->set<tcod::Console>(ecs->get_mut<tcod::Context>().new_console());

  player_x = width / 2;
  player_y = height / 2;

  return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void *appstate) {
  auto ecs = *static_cast<flecs::world *>(appstate);
  auto context = ecs.try_get_mut<tcod::Context>();
  auto console = ecs.try_get_mut<tcod::Console>();
  console->clear();

  console->at(player_x, player_y) = {'@', tcod::ColorRGB{255, 255, 255},
                                     tcod::ColorRGB{0, 0, 0}};

  context->present(*console);

  return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent([[maybe_unused]] void *appstate, SDL_Event *event) {
  auto action = Action::get(event);
  switch (action.type) {
  case ActionType::ESCAPE:
  case ActionType::QUIT:
    return SDL_APP_SUCCESS;
    break;
  case ActionType::MOVE:
    player_x += action.xy[0];
    player_y += action.xy[1];
    break;

  case ActionType::NONE:
    // This intentionally does nothing.
    break;
  }
  return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void *data, SDL_AppResult) {
  auto ecs = static_cast<flecs::world *>(data);
  delete ecs;
}
