#include "input_handler.hpp"

#include <algorithm>
#include <cassert>
#include <optional>

#include <libtcod.hpp>

#include "action.hpp"
#include "color.hpp"
#include "defines.hpp"
#include "engine.hpp"
#include "game_map.hpp"
#include "inventory.hpp"
#include "message_log.hpp"
#include "render_functions.hpp"

static inline void restoreMainGame(EventHandler &e) {
  e.keyDown = &EventHandler::MainGameKeyDown;
  e.click = &EventHandler::MainGameClick;
  e.on_render = &EventHandler::MainGameOnRender;
  e.handle_action = &EventHandler::MainGameHandleAction;
  e.item_selected = nullptr;
  e.loc_selected = nullptr;
}

static inline void makeHistoryHandler(EventHandler &e, flecs::world ecs) {
  e.keyDown = &EventHandler::HistoryKeyDown;
  e.click = &EventHandler::MainGameClick;
  e.on_render = &EventHandler::HistoryOnRender;
  e.handle_action = &EventHandler::MainGameHandleAction;
  e.item_selected = nullptr;
  e.loc_selected = nullptr;

  e.log_length = ecs.lookup("messageLog").get<MessageLog>().size();
  e.cursor = e.log_length - 1;
}

template <char const *TITLE,
          std::unique_ptr<Action> (EventHandler::*f)(flecs::entity)>
static inline void makeInventoryHandler(EventHandler &e, flecs::world ecs) {
  e.keyDown = &EventHandler::InventoryKeyDown;
  e.click = &EventHandler::AskUserClick;
  e.on_render = &EventHandler::InventoryOnRender;
  e.handle_action = &EventHandler::AskUserHandleAction;
  e.item_selected = f;
  e.loc_selected = nullptr;

  e.title = TITLE;
  e.q = ecs.query_builder<const Named>("module::playerItem")
            .with<ContainedBy>(ecs.lookup("player"))
            .with<Item>()
            .cached()
            .build();
}

static void makeLookHandler(EventHandler &e, flecs::world ecs) {
  e.keyDown = &EventHandler::SelectKeyDown;
  e.click = &EventHandler::SelectClick;
  e.on_render = &EventHandler::SelectOnRender;
  e.handle_action = &EventHandler::AskUserHandleAction;
  e.item_selected = nullptr;
  e.loc_selected = &EventHandler::LookSelectedLoc;

  e.mouse_loc = ecs.lookup("player").get<Position>();
}

std::unique_ptr<Action> EventHandler::dispatch(SDL_Event *event,
                                               flecs::world ecs) {
  switch (event->type) {
  case SDL_EVENT_QUIT:
    return std::make_unique<ExitAction>();

  case SDL_EVENT_KEY_DOWN:
    return (this->*keyDown)(&event->key, ecs);

  case SDL_EVENT_MOUSE_MOTION:
    mouse_loc[0] = (int)event->motion.x;
    mouse_loc[1] = (int)event->motion.y;
    return nullptr;

  case SDL_EVENT_MOUSE_BUTTON_DOWN:
    return (this->*click)(&event->button, ecs);

  default:
    return nullptr;
  }
}

void EventHandler::mainMenu(void) {
  keyDown = &EventHandler::MainMenuKeyDown;
  click = &EventHandler::MainGameClick;
  on_render = &EventHandler::MainMenuOnRender;
  handle_action = &EventHandler::MainMenuHandleAction;
  item_selected = nullptr;
  loc_selected = nullptr;
}

void EventHandler::gameOver(void) {
  keyDown = &EventHandler::GameOverKeyDown;
  click = &EventHandler::MainGameClick;
  on_render = &EventHandler::MainGameOnRender;
  handle_action = &EventHandler::MainGameHandleAction;
  item_selected = nullptr;
  loc_selected = nullptr;
}

std::unique_ptr<Action> EventHandler::MainGameKeyDown(SDL_KeyboardEvent *key,
                                                      flecs::world ecs) {
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
    if (key->mod & SDL_KMOD_SHIFT) {
      return std::make_unique<TakeStairsAction>();
    }
    // Intentional fallthrough
  case SDL_SCANCODE_KP_5:
  case SDL_SCANCODE_CLEAR:
    return std::make_unique<WaitAction>();
  case SDL_SCANCODE_G:
    return std::make_unique<PickupAction>();
  case SDL_SCANCODE_D:
    static constexpr char DROP_TITLE[] = "┤Select an item to drop├";
    makeInventoryHandler<DROP_TITLE, &EventHandler::DropItemSelected>(*this,
                                                                      ecs);
    return nullptr;
  case SDL_SCANCODE_I:
    static constexpr char USE_TITLE[] = "┤Select an item to use├";
    makeInventoryHandler<USE_TITLE, &EventHandler::UseItemSelected>(*this, ecs);
    return nullptr;
  case SDL_SCANCODE_V:
    makeHistoryHandler(*this, ecs);
    return nullptr;
  case SDL_SCANCODE_SLASH:
    makeLookHandler(*this, ecs);
    return nullptr;

  case SDL_SCANCODE_ESCAPE:
    return std::make_unique<ExitAction>();

  default:
    return nullptr;
  }
}

std::unique_ptr<Action> EventHandler::GameOverKeyDown(SDL_KeyboardEvent *key,
                                                      flecs::world) {
  switch (key->scancode) {
  case SDL_SCANCODE_ESCAPE:
    return std::make_unique<QuitWithoutSavingAction>();

  default:
    return nullptr;
  }
}

std::unique_ptr<Action> EventHandler::HistoryKeyDown(SDL_KeyboardEvent *key,
                                                     flecs::world) {
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
    restoreMainGame(*this);
    return nullptr;
  }
}

std::unique_ptr<Action> EventHandler::AskUserKeyDown(SDL_KeyboardEvent *key,
                                                     flecs::world) {
  switch (key->scancode) {
  case SDL_SCANCODE_LSHIFT:
  case SDL_SCANCODE_RSHIFT:
  case SDL_SCANCODE_LCTRL:
  case SDL_SCANCODE_RCTRL:
  case SDL_SCANCODE_LALT:
  case SDL_SCANCODE_RALT:
    return nullptr;
  default:
    restoreMainGame(*this);
    return nullptr;
  }
}

std::unique_ptr<Action> EventHandler::InventoryKeyDown(SDL_KeyboardEvent *key,
                                                       flecs::world ecs) {
  auto idx = key->scancode - SDL_SCANCODE_A;
  if (0 <= idx && idx < q.count()) {
    return (this->*item_selected)(q.page(idx, 1).first());
  }
  return AskUserKeyDown(key, ecs);
}

std::unique_ptr<Action> EventHandler::SelectKeyDown(SDL_KeyboardEvent *key,
                                                    flecs::world ecs) {
  auto dxy = std::array<int, 2>{0, 0};

  switch (key->scancode) {
  case SDL_SCANCODE_LSHIFT:
  case SDL_SCANCODE_RSHIFT:
  case SDL_SCANCODE_LCTRL:
  case SDL_SCANCODE_RCTRL:
  case SDL_SCANCODE_LALT:
  case SDL_SCANCODE_RALT:
    return nullptr;

  case SDL_SCANCODE_RETURN:
  case SDL_SCANCODE_RETURN2:
  case SDL_SCANCODE_KP_ENTER:
    return (this->*loc_selected)(mouse_loc);

  case SDL_SCANCODE_UP:
  case SDL_SCANCODE_KP_8:
  case SDL_SCANCODE_K:
    dxy = {0, -1};
    break;
  case SDL_SCANCODE_DOWN:
  case SDL_SCANCODE_KP_2:
  case SDL_SCANCODE_J:
    dxy = {0, 1};
    break;
  case SDL_SCANCODE_LEFT:
  case SDL_SCANCODE_KP_4:
  case SDL_SCANCODE_H:
    dxy = {-1, 0};
    break;
  case SDL_SCANCODE_RIGHT:
  case SDL_SCANCODE_KP_6:
  case SDL_SCANCODE_L:
    dxy = {1, 0};
    break;
  case SDL_SCANCODE_HOME:
  case SDL_SCANCODE_KP_7:
  case SDL_SCANCODE_Y:
    dxy = {-1, -1};
    break;
  case SDL_SCANCODE_END:
  case SDL_SCANCODE_KP_1:
  case SDL_SCANCODE_B:
    dxy = {-1, 1};
    break;
  case SDL_SCANCODE_PAGEUP:
  case SDL_SCANCODE_KP_9:
  case SDL_SCANCODE_U:
    dxy = {1, -1};
    break;
  case SDL_SCANCODE_PAGEDOWN:
  case SDL_SCANCODE_KP_3:
  case SDL_SCANCODE_N:
    dxy = {1, 1};
    break;

  default:
    restoreMainGame(*this);
    return nullptr;
  }

  auto modifier = 1;
  if (key->mod & SDL_KMOD_SHIFT) {
    modifier *= 5;
  }
  if (key->mod & SDL_KMOD_CTRL) {
    modifier *= 10;
  }
  if (key->mod & SDL_KMOD_ALT) {
    modifier *= 20;
  }

  auto &map = ecs.lookup("currentMap").target<CurrentMap>().get<GameMap>();
  mouse_loc[0] =
      std::clamp(mouse_loc[0] + dxy[0] * modifier, 0, map.getWidth());
  mouse_loc[1] =
      std::clamp(mouse_loc[1] + dxy[1] * modifier, 0, map.getHeight());
  return nullptr;
}

std::unique_ptr<Action> EventHandler::MainMenuKeyDown(SDL_KeyboardEvent *key,
                                                      flecs::world ecs) {
  switch (key->scancode) {
  case SDL_SCANCODE_Q:
  case SDL_SCANCODE_ESCAPE:
    return std::make_unique<ExitAction>();
  case SDL_SCANCODE_C:
    if (Engine::load(ecs, saveFilename, *this)) {
      restoreMainGame(*this);
    }
    break;
  case SDL_SCANCODE_N:
    Engine::new_game(ecs);
    restoreMainGame(*this);
    break;

  default:
    return nullptr;
  }

  return nullptr;
}

std::unique_ptr<Action> EventHandler::PopupKeyDown(SDL_KeyboardEvent *,
                                                   flecs::world) {
  parent(this);
  return nullptr;
}

std::unique_ptr<Action> EventHandler::MainGameClick(SDL_MouseButtonEvent *,
                                                    flecs::world) {
  return nullptr;
}

std::unique_ptr<Action> EventHandler::AskUserClick(SDL_MouseButtonEvent *,
                                                   flecs::world) {
  restoreMainGame(*this);
  return nullptr;
}

std::unique_ptr<Action> EventHandler::SelectClick(SDL_MouseButtonEvent *button,
                                                  flecs::world ecs) {
  auto &map = ecs.lookup("currentMap").target<CurrentMap>().get<GameMap>();
  if (map.inBounds((int)button->x, (int)button->y)) {
    if (button->button == SDL_BUTTON_LEFT) {
      return (this->*loc_selected)({(int)button->x, (int)button->y});
    }
  }
  return AskUserClick(button, ecs);
}

void EventHandler::MainGameOnRender(flecs::world ecs, tcod::Console &console) {
  auto map = ecs.lookup("currentMap").target<CurrentMap>();
  auto &gMap = map.get_mut<GameMap>();
  gMap.render(console);

  ecs.lookup("messageLog").get<MessageLog>().render(console, 21, 45, 40, 5);

  auto q =
      ecs.query_builder<const Position, const Renderable>("module::renderable")
          .with(flecs::ChildOf, map)
          .order_by<const Renderable>([](auto, auto r1, auto, auto r2) {
            return static_cast<int>(r1->layer) - static_cast<int>(r2->layer);
          })
          .build();

  q.each([&](auto p, auto r) {
    if (gMap.isInFov(p)) {
      r.render(console, p);
    }
  });

  auto player = ecs.lookup("player");
  player.get<Renderable>().render(console, player.get<Position>());

  auto fighter = player.get<Fighter>();
  renderBar(console, fighter.hp(), fighter.max_hp, 20);
  renderDungeonLevel(console, gMap.level, {0, 47});
  renderNamesAtMouseLocation(console, {21, 44}, mouse_loc, map);
}

static constexpr auto DECORATION = std::array<int, 9>{
    0x250c, 0x2500, 0x2510, 0x2502, 0, 0x2502, 0x2514, 0x2500, 0x2518};

void EventHandler::HistoryOnRender(flecs::world ecs, tcod::Console &console) {
  MainGameOnRender(ecs, console);
  auto logConsole =
      tcod::Console(console.get_width() - 6, console.get_height() - 6);
  tcod::draw_frame(logConsole,
                   {0, 0, logConsole.get_width(), logConsole.get_height()},
                   DECORATION, std::nullopt, std::nullopt);
  tcod::print_rect(logConsole, {0, 0, logConsole.get_width(), 1},
                   "┤Message history├", std::nullopt, std::nullopt,
                   TCOD_CENTER);
  ecs.lookup("messageLog")
      .get<MessageLog>()
      .render(logConsole, 1, 1, logConsole.get_width() - 2,
              logConsole.get_height() - 2, cursor);
  tcod::blit(console, logConsole, {3, 3});
}

void EventHandler::InventoryOnRender(flecs::world ecs, tcod::Console &console) {
  MainGameOnRender(ecs, console);
  auto count = q.count();
  auto &player = ecs.lookup("player").get<Position>();
  auto x = player.x <= 30 ? 40 : 0;

  tcod::draw_frame(console, {x, 0, (int)title.size(), std::max(count + 2, 3)},
                   DECORATION, color::white, color::black);
  tcod::print_rect(console, {x, 0, (int)title.size(), 1}, title, std::nullopt,
                   std::nullopt, TCOD_CENTER);
  if (count > 0) {
    auto idx = 0;
    q.each([&](const auto &name) {
      auto msg = tcod::stringf("(%c) %s", 'a' + idx, name.name.c_str());
      tcod::print(console, {x + 1, idx + 1}, msg, std::nullopt, std::nullopt);
      idx++;
    });
  } else {
    tcod::print(console, {x + 1, 1}, "(Empty)", std::nullopt, std::nullopt);
  }
}

void EventHandler::SelectOnRender(flecs::world ecs, tcod::Console &console) {
  MainGameOnRender(ecs, console);
  auto &tile = console.at(mouse_loc);
  tile.bg = color::white;
  tile.fg = color::black;
}

void EventHandler::AreaTargetOnRender(flecs::world ecs,
                                      tcod::Console &console) {
  SelectOnRender(ecs, console);
  tcod::draw_frame(console,
                   {mouse_loc[0] - radius - 1, mouse_loc[1] - radius - 1,
                    radius * radius, radius * radius},
                   DECORATION, color::red, std::nullopt);
}

void EventHandler::MainMenuOnRender(flecs::world, tcod::Console &console) {
  static auto background_image = TCODImage("assets/menu_background.png");
  tcod::draw_quartergraphics(console, background_image);
  tcod::print(console, {console.get_width() / 2, console.get_height() / 2 - 4},
              "Yet Another Roguelike", color::menu_title, std::nullopt,
              TCOD_CENTER);
  tcod::print(console, {console.get_width() / 2, console.get_height() - 2},
              "By degustaf", color::menu_title, std::nullopt, TCOD_CENTER);

  static const auto choices =
      std::array{"[N] Play a new game     ", "[C] Continue last game  ",
                 "[Q] Quit                "};
  for (auto i = 0; i < (int)choices.size(); i++) {
    tcod::print(console,
                {console.get_width() / 2, console.get_height() / 2 - 2 + i},
                choices[i], color::menu_text, color::black, TCOD_CENTER);
  }
}

void EventHandler::PopupOnRender(flecs::world ecs, tcod::Console &console) {
  parentOnRender(this, ecs, console);
  for (auto &tile : console) {
    tile.fg /= 8;
    tile.bg /= 8;
  }

  tcod::print(console, {console.get_width() / 2, console.get_height() / 2},
              text, color::white, color::black, TCOD_CENTER);
}

ActionResult
EventHandler::MainGameHandleAction(flecs::world ecs,
                                   std::unique_ptr<Action> action) {
  if (action) {
    auto player = ecs.entity("player");
    auto ret = action->perform(player);
    if (ret.msg.size() > 0) {
      ecs.lookup("messageLog")
          .get_mut<MessageLog>()
          .addMessage(ret.msg, ret.fg);
    }
    if (ret) {
      ecs.lookup("currentMap")
          .target<CurrentMap>()
          .get_mut<GameMap>()
          .update_fov(player);
      Engine::handle_enemy_turns(ecs);
    }
    return ret;
  }
  return {ActionResultType::Failure, ""};
}

ActionResult
EventHandler::MainMenuHandleAction(flecs::world ecs,
                                   std::unique_ptr<Action> action) {
  if (action) {
    auto ret = action->perform(ecs.entity());
    assert(ret.msg.size() == 0);
    assert(!ret);
    return ret;
  }
  return {ActionResultType::Failure, ""};
}

ActionResult EventHandler::AskUserHandleAction(flecs::world ecs,
                                               std::unique_ptr<Action> action) {
  auto ret = MainGameHandleAction(ecs, std::move(action));
  if (ret) {
    restoreMainGame(*this);
  }
  return ret;
}

std::unique_ptr<Action> EventHandler::DropItemSelected(flecs::entity item) {
  return std::make_unique<DropItemAction>(item);
}

std::unique_ptr<Action> EventHandler::UseItemSelected(flecs::entity item) {
  return std::make_unique<ItemAction>(item);
}

std::unique_ptr<Action> EventHandler::LookSelectedLoc(std::array<int, 2>) {
  restoreMainGame(*this);
  return nullptr;
}

std::unique_ptr<Action>
EventHandler::SingleTargetSelectedLoc(std::array<int, 2> xy) {
  restoreMainGame(*this);
  return callback(xy);
}
