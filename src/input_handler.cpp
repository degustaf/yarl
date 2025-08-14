#include "input_handler.hpp"

#include <algorithm>
#include <memory>
#include <optional>

#include <libtcod.hpp>

#include "action.hpp"
#include "engine.hpp"
#include "game_map.hpp"
#include "render_functions.hpp"

std::unique_ptr<Action> EventHandler::dispatch(SDL_Event *event,
                                               Engine &engine) {
  switch (event->type) {
  case SDL_EVENT_QUIT:
    return std::make_unique<ExitAction>();

  case SDL_EVENT_KEY_DOWN:
    return (this->*keyDown)(&event->key, engine);

  case SDL_EVENT_MOUSE_MOTION:
    mouse_loc[0] = (int)event->motion.x;
    mouse_loc[1] = (int)event->motion.y;
    return nullptr;

  default:
    return nullptr;
  }
}

std::unique_ptr<Action> EventHandler::MainGameKeyDown(SDL_KeyboardEvent *key,
                                                      Engine &engine) {
  switch (key->scancode) {
  case SDL_SCANCODE_UP:
  case SDL_SCANCODE_KP_8:
  case SDL_SCANCODE_K:
    return std::make_unique<BumpAction>(0, -1);
  case SDL_SCANCODE_DOWN:
  case SDL_SCANCODE_KP_2:
  case SDL_SCANCODE_J:
    return std::make_unique<BumpAction>(0, 1);
  case SDL_SCANCODE_LEFT:
  case SDL_SCANCODE_KP_4:
  case SDL_SCANCODE_H:
    return std::make_unique<BumpAction>(-1, 0);
  case SDL_SCANCODE_RIGHT:
  case SDL_SCANCODE_KP_6:
  case SDL_SCANCODE_L:
    return std::make_unique<BumpAction>(1, 0);
  case SDL_SCANCODE_HOME:
  case SDL_SCANCODE_KP_7:
  case SDL_SCANCODE_Y:
    return std::make_unique<BumpAction>(-1, -1);
  case SDL_SCANCODE_END:
  case SDL_SCANCODE_KP_1:
  case SDL_SCANCODE_B:
    return std::make_unique<BumpAction>(-1, 1);
  case SDL_SCANCODE_PAGEUP:
  case SDL_SCANCODE_KP_9:
  case SDL_SCANCODE_U:
    return std::make_unique<BumpAction>(1, -1);
  case SDL_SCANCODE_PAGEDOWN:
  case SDL_SCANCODE_KP_3:
  case SDL_SCANCODE_N:
    return std::make_unique<BumpAction>(1, 1);

  case SDL_SCANCODE_PERIOD:
  case SDL_SCANCODE_KP_5:
  case SDL_SCANCODE_CLEAR:
    return std::make_unique<WaitAction>();
  case SDL_SCANCODE_G:
    return std::make_unique<PickupAction>();

  case SDL_SCANCODE_V:
    keyDown = &EventHandler::HistoryKeyDown;
    on_render = &EventHandler::HistoryOnRender;
    log_length = engine.messageLog.size();
    cursor = log_length - 1;
    return nullptr;

  case SDL_SCANCODE_ESCAPE:
    return std::make_unique<ExitAction>();

  default:
    return nullptr;
  }
}

std::unique_ptr<Action> EventHandler::GameOverKeyDown(SDL_KeyboardEvent *key,
                                                      Engine &) {
  switch (key->scancode) {
  case SDL_SCANCODE_ESCAPE:
    return std::make_unique<ExitAction>();

  default:
    return nullptr;
  }
}

std::unique_ptr<Action> EventHandler::HistoryKeyDown(SDL_KeyboardEvent *key,
                                                     Engine &) {
  switch (key->scancode) {
  case SDL_SCANCODE_UP:
    if (cursor == 0) {
      cursor = log_length - 1;
    } else {
      cursor--;
    }
    return nullptr;
  case SDL_SCANCODE_DOWN:
    if (cursor == log_length - 1) {
      cursor = 0;
    } else {
      cursor++;
    }
    return nullptr;
  case SDL_SCANCODE_PAGEUP:
    cursor = cursor < 10 ? 0 : cursor - 10;
    return nullptr;
  case SDL_SCANCODE_PAGEDOWN:
    cursor = std::min(cursor + 10, log_length - 1);
    return nullptr;
  case SDL_SCANCODE_HOME:
    cursor = 0;
    return nullptr;
  case SDL_SCANCODE_END:
    cursor = log_length - 1;
    return nullptr;

  default:
    keyDown = &EventHandler::MainGameKeyDown;
    on_render = &EventHandler::MainGameOnRender;
    return nullptr;
  }
}

void EventHandler::MainGameOnRender(flecs::world ecs) {
  auto &console = ecs.get_mut<tcod::Console>();

  auto map = ecs.target<CurrentMap>();
  auto &gMap = map.get_mut<GameMap>();
  gMap.render(console);

  const auto &engine = ecs.get<Engine>();
  engine.messageLog.render(console, 21, 45, 40, 5);

  auto q = ecs.query_builder<const Position, const Renderable>()
               .with(flecs::ChildOf, map)
               .order_by<const Renderable>([](auto, auto r1, auto, auto r2) {
                 return (int)(r1->layer - r2->layer);
               })
               .build();

  q.each([&](auto p, auto r) {
    if (gMap.isInFov(p.x, p.y)) {
      r.render(console, p);
    }
  });

  auto player = ecs.lookup("player");
  player.get<Renderable>().render(console, player.get<Position>());

  auto fighter = player.get<Fighter>();
  renderBar(console, fighter.hp(), fighter.max_hp, 20);
  renderNamesAtMouseLocation(console, {21, 44}, mouse_loc, map);
}

static constexpr auto DECORATION = std::array<int, 9>{
    0x250c, 0x2500, 0x2510, 0x2502, 0, 0x2502, 0x2514, 0x2500, 0x2518};

void EventHandler::HistoryOnRender(flecs::world ecs) {
  MainGameOnRender(ecs);
  auto &console = ecs.get_mut<tcod::Console>();
  auto logConsole =
      tcod::Console(console.get_width() - 6, console.get_height() - 6);
  tcod::draw_frame(logConsole,
                   {0, 0, logConsole.get_width(), logConsole.get_height()},
                   DECORATION, std::nullopt, std::nullopt);
  tcod::print_rect(logConsole, {0, 0, logConsole.get_width(), 1},
                   "┤Message history├", std::nullopt, std::nullopt,
                   TCOD_CENTER);
  ecs.get<Engine>().messageLog.render(logConsole, 1, 1,
                                      logConsole.get_width() - 2,
                                      logConsole.get_height() - 2, cursor);
  tcod::blit(console, logConsole, {3, 3});
}
