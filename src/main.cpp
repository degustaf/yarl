#include <optional>
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

struct A {
  std::vector<int> c;
  std::vector<int> d;
  std::optional<int> a;
};

template <typename Elem, typename Vector = std::vector<Elem>>
flecs::opaque<Vector, Elem> std_vector_support(flecs::world &world) {
  return flecs::opaque<Vector, Elem>()
      .as_type(world.vector<Elem>())

      // Forward elements of std::vector value to serializer
      .serialize([](const flecs::serializer *s, const Vector *data) {
        for (const auto &el : *data) {
          s->value(el);
        }
        return 0;
      })

      // Return vector count
      .count([](const Vector *data) { return data->size(); })

      // Resize contents of vector
      .resize([](Vector *data, size_t size) { data->resize(size); })

      // Ensure element exists, return pointer
      .ensure_element([](Vector *data, size_t elem) {
        if (data->size() <= elem) {
          data->resize(elem + 1);
        }

        return &data->data()[elem];
      });
}

template <typename Elem>
flecs::opaque<std::optional<Elem>, Elem>
std_optional_support(flecs::world &world) {
  return flecs::opaque<std::optional<Elem>, Elem>()
      .as_type(world.vector<Elem>())
      .serialize(
          [](const flecs::serializer *s, const std::optional<Elem> *data) {
            if (*data) {
              s->value(**data);
            }
            return 0;
          })
      .count([](const std::optional<Elem> *data) -> size_t {
        return *data ? 1 : 0;
      })
      .resize([](std::optional<Elem> *data, size_t size) {
        switch (size) {
        case 0:
          *data = std::nullopt;
          break;
        case 1:
          *data = Elem();
          break;
        default:
          assert(false);
        }
      })
      .ensure_element(
          [](std::optional<Elem> *data, size_t) { return &**data; });
}

const auto code = "prefab b {\n"
                  "  M.A: {\n"
                  "    c: []"
                  "    d: []"
                  "    a: []"
                  "  }"
                  "}";

struct M {
  M(flecs::world ecs) {
    ecs.module<M>("M");
    ecs.component<std::vector<int>>().opaque(std_vector_support<int>);
    ecs.component<std::optional<int>>().opaque(std_optional_support<int>);
    ecs.component<A>()
        .member<std::vector<int>>("c")
        .member<std::vector<int>>("d")
        .member<std::optional<int>>("a");

    ecs.script_run("crash", code);
  };
};

void crash(void) {
  auto ecs = flecs::world();
  ecs.import <M>();
}

SDL_AppResult SDL_AppInit(void **data, [[maybe_unused]] int argc,
                          [[maybe_unused]] char **argv) {
  crash();
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
  ecs->add<EventHandler>();

#if !defined NDEBUG
  ecs->import <flecs::stats>();
  ecs->set<flecs::Rest>({});
#endif

  return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void *appstate) {
  auto ecs = *static_cast<flecs::world *>(appstate);
  auto &console = ecs.get_mut<tcod::Console>();
  console.clear();
  auto &eventHandler = ecs.get_mut<EventHandler>();
  (eventHandler.*(eventHandler.on_render))(ecs, console, SDL_GetTicks());
  ecs.get_mut<tcod::Context>().present(console);
#if !defined NDEBUG
  ecs.progress();
#endif
  return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event) {
  auto &ecs = *static_cast<flecs::world *>(appstate);
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
    delete_file(data_dir / saveFilename);
  } else {
    // TODO handle game not started.
    Engine::save_as(*ecs, data_dir / saveFilename);
  }
  // ecs->release();
  // delete ecs;
}
