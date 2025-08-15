#define SDL_MAIN_USE_CALLBACKS
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include <flecs.h>
#include <libtcod.hpp>

#include "actor.hpp"
#include "engine.hpp"
#include "game_map.hpp"
#include "inventory.hpp"
#include "module.hpp"
#include "room_accretion.hpp"

SDL_AppResult SDL_AppInit(void **data, [[maybe_unused]] int argc,
                          [[maybe_unused]] char **argv) {
  int width = 80;
  int height = 50;

  int map_width = 80;
  int map_height = 43;

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
  ecs->set<Engine>(Engine());

  auto player = ecs->entity("player")
                    .set<Position>({width / 2, height / 2})
                    .set<Renderable>({'@', {255, 255, 255}, RenderOrder::Actor})
                    .set<Named>({"Player"})
                    .emplace<Fighter>(30, 2, 5)
                    .set<Inventory>({26});
  auto map = ecs->entity();
  map.emplace<GameMap>(generateDungeon(map, map_width, map_height, player));

  ecs->add<CurrentMap>(map);
  map.get_mut<GameMap>().update_fov(player);

  ecs->get_mut<Engine>().messageLog.addMessage(
      "Hello and welcome, adventurer, to yet another dungeon!",
      color::welcomeText);

  return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void *appstate) {
  auto ecs = *static_cast<flecs::world *>(appstate);
  auto &console = ecs.get_mut<tcod::Console>();
  console.clear();
  auto eventHandler = ecs.get<Engine>().eventHandler;
  (eventHandler.*(eventHandler.on_render))(ecs, console);
  ecs.get_mut<tcod::Context>().present(console);
  return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event) {
  auto ecs = *static_cast<flecs::world *>(appstate);
  auto &eh = ecs.get_mut<Engine>().eventHandler;
  ecs.get_mut<tcod::Context>().convert_event_coordinates(*event);
  return (eh.*(eh.handle_action))(ecs, eh.dispatch(event, ecs));
}

void SDL_AppQuit(void * /*data*/, SDL_AppResult) {
  // auto ecs = static_cast<flecs::world *>(data);
  // ecs->release();
  // delete ecs;
}
