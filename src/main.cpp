#include <SDL3/SDL_timer.h>
#define SDL_MAIN_USE_CALLBACKS
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

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

  tcod::Tileset tileset = tcod::load_tilesheet("assets/terminal16x16.png",
                                               {16, 16}, tcod::CHARMAP_CP437);
  params.tcod_version = TCOD_COMPILEDVERSION; // This is required.
  params.columns = width;
  params.rows = height;
  params.tileset = tileset.get();
  params.window_title = "Yet Another Roguelike";
  params.sdl_window_flags = SDL_WINDOW_RESIZABLE;
  params.vsync = true;

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
  ecs->set<tcod::Context>(tcod::Context(params));
  ecs->set<tcod::Console>(
      ecs->get_mut<tcod::Context>().new_console(width, height));
  ecs->set<std::unique_ptr<InputHandler>>(
      std::make_unique<MainMenuInputHandler>());

  if (std::filesystem::exists(data_dir / configName)) {
    Command::load(data_dir / configName);
  } else {
    Command::init();
    Command::save(data_dir / configName);
  }

#if !defined NDEBUG
  ecs->import <flecs::stats>();
  ecs->set<flecs::Rest>({});
#endif

  return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void *appstate) {
  auto ecs = *static_cast<flecs::world *>(appstate);
  auto &console = ecs.get_mut<tcod::Console>();
  console.clear({' ', color::text, color::background});

  auto &handler = ecs.get_mut<std::unique_ptr<InputHandler>>();
  handler->animate(ecs, SDL_GetTicks());
  handler->on_render(ecs, console);
  ecs.get_mut<tcod::Context>().present(console);
#if !defined NDEBUG
  ecs.progress();
#endif
  return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event) {
  auto &ecs = *static_cast<flecs::world *>(appstate);
  auto &handler = ecs.get_mut<std::unique_ptr<InputHandler>>();
  ecs.get_mut<tcod::Context>().convert_event_coordinates(*event);
  return handler->handle_action(ecs, handler->dispatch(event, ecs));
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
  // ecs->release();
  // delete ecs;
}
