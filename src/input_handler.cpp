#include "input_handler.hpp"

#include <algorithm>
#include <cassert>
#include <optional>

#include <libtcod.hpp>

#include "color.hpp"
#include "console.hpp"
#include "consumable.hpp"
#include "defines.hpp"
#include "engine.hpp"
#include "game_map.hpp"
#include "inventory.hpp"
#include "level.hpp"
#include "message_log.hpp"
#include "render_functions.hpp"
#include "scent.hpp"

static inline void makeHistoryHandler(EventHandler &e, flecs::world ecs) {
  e.keyDown = &EventHandler::HistoryKeyDown;
  e.click = &EventHandler::EmptyClick;
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
  assert(e.mouse_loc[0] >= 0);
  assert(e.mouse_loc[1] >= 0);
}

static inline void makeLevelUp(EventHandler &e) {
  e.keyDown = &EventHandler::LevelUpKeyDown;
  e.click = &EventHandler::LevelUpClick;
  e.on_render = &EventHandler::LevelUpOnRender;
  e.handle_action = &EventHandler::AskUserHandleAction;
  e.item_selected = nullptr;
  e.loc_selected = nullptr;

  e.title = "Level Up";
}

static inline void makeCharacterScreen(EventHandler &e) {
  e.keyDown = &EventHandler::AskUserKeyDown;
  e.click = &EventHandler::AskUserClick;
  e.on_render = &EventHandler::CharacterScreenOnRender;
  e.handle_action = &EventHandler::AskUserHandleAction;
  e.item_selected = nullptr;
  e.loc_selected = nullptr;

  e.title = "Character Information";
}

std::unique_ptr<Action> EventHandler::dispatch(SDL_Event *event,
                                               flecs::world &ecs) {
  switch (event->type) {
  case SDL_EVENT_QUIT:
    return std::make_unique<ExitAction>();

  case SDL_EVENT_KEY_DOWN:
    return (this->*keyDown)(&event->key, ecs);

  case SDL_EVENT_MOUSE_MOTION:
    mouse_loc[0] = (int)event->motion.x;
    mouse_loc[1] = (int)event->motion.y;
    assert(mouse_loc[0] >= 0);
    assert(mouse_loc[1] >= 0);
    return nullptr;

  case SDL_EVENT_MOUSE_BUTTON_DOWN:
    return (this->*click)(&event->button, ecs);

  default:
    return nullptr;
  }
}

void EventHandler::restoreMainGame() {
  assert(mouse_loc[0] >= 0);
  assert(mouse_loc[1] >= 0);
  keyDown = &EventHandler::MainGameKeyDown;
  click = &EventHandler::MainGameClick;
  on_render = &EventHandler::MainGameOnRender;
  handle_action = &EventHandler::MainGameHandleAction;
  item_selected = nullptr;
  loc_selected = nullptr;
}

void EventHandler::jumpConfirm(bool useRope, flecs::entity item) {
  keyDown = &EventHandler::JumpKeyDown;
  click = &EventHandler::AskUserClick;
  on_render = &EventHandler::JumpOnRender;
  handle_action = &EventHandler::AskUserHandleAction;
  item_selected = nullptr;
  loc_selected = nullptr;

  this->useRope = useRope;
  this->item = item;
}

void EventHandler::mainMenu(void) {
  assert(mouse_loc[0] >= 0);
  assert(mouse_loc[1] >= 0);
  keyDown = &EventHandler::MainMenuKeyDown;
  click = &EventHandler::EmptyClick;
  on_render = &EventHandler::MainMenuOnRender;
  handle_action = &EventHandler::MainMenuHandleAction;
  item_selected = nullptr;
  loc_selected = nullptr;
}

void EventHandler::gameOver(void) {
  keyDown = &EventHandler::GameOverKeyDown;
  click = &EventHandler::EmptyClick;
  on_render = &EventHandler::MainGameOnRender;
  handle_action = &EventHandler::MainGameHandleAction;
  item_selected = nullptr;
  loc_selected = nullptr;
}

void EventHandler::winGame(void) {
  keyDown = &EventHandler::GameOverKeyDown;
  click = &EventHandler::EmptyClick;
  on_render = &EventHandler::WinOnRender;
  handle_action = &EventHandler::MainMenuHandleAction;
  item_selected = nullptr;
  loc_selected = nullptr;
}

static constexpr auto COMMAND_MENU_WIDTH = 70;
static constexpr auto COMMAND_MENU_HEIGHT = 28;

static Console buildCommandMenu(void) {
  auto con = Console(COMMAND_MENU_WIDTH, COMMAND_MENU_HEIGHT);
  con.clear();
  Console::draw_frame(con, {0, 0, COMMAND_MENU_WIDTH, COMMAND_MENU_HEIGHT},
                      DECORATION, color::menu_border, std::nullopt);

  Console::print(con, {COMMAND_MENU_WIDTH / 2 - 4, 1}, "Commands", color::white,
                 std::nullopt);

  // vim directions
  Console::print(con, {3, 3}, "y k u", color::white, std::nullopt);
  Console::print(con, {4, 4}, "\\|/", color::white, std::nullopt);
  Console::print(con, {3, 5}, "h-*-l", color::white, std::nullopt);
  Console::print(con, {4, 6}, "/|\\", color::white, std::nullopt);
  Console::print(con, {3, 7}, "b j n", color::white, std::nullopt);

  // numpad directions.
  Console::print(con, {10, 2}, "numpad", color::white, std::nullopt);
  Console::print(con, {10, 3}, "7 8 9", color::white, std::nullopt);
  Console::print(con, {11, 4}, "\\|/", color::white, std::nullopt);
  Console::print(con, {10, 5}, "4-*-6", color::white, std::nullopt);
  Console::print(con, {11, 6}, "/|\\", color::white, std::nullopt);
  Console::print(con, {10, 7}, "1 2 3", color::white, std::nullopt);

  auto y = 9;

  Console::print(con, {2, y++}, "Hold <shift> to move 2 spaces", color::white,
                 std::nullopt);
  Console::print(con, {2, y++}, "Hold <ctrl> to move 3 spaces", color::white,
                 std::nullopt);
  y++;
  Console::print(con, {2, y++}, "5: wait", color::white, std::nullopt);
  Console::print(con, {2, y++}, ".: wait", color::white, std::nullopt);
  Console::print(con, {2, y++}, ">: Take elevator", color::white, std::nullopt);
  Console::print(con, {2, y++}, "C: This menu", color::white, std::nullopt);
  Console::print(con, {2, y++}, "D: Drop an item", color::white, std::nullopt);
  Console::print(con, {2, y++}, "F: Fire a weapon", color::white, std::nullopt);
  Console::print(con, {2, y++}, "G: Pick up an item", color::white,
                 std::nullopt);
  Console::print(con, {2, y++}, "I: Inventory", color::white, std::nullopt);
  Console::print(con, {2, y++}, "V: View game log", color::white, std::nullopt);
  Console::print(con, {2, y++}, "X: View character information", color::white,
                 std::nullopt);
  Console::print(con, {2, y++}, "/: Look around map", color::white,
                 std::nullopt);
  Console::print(con, {2, y++}, "Esc: exit menu", color::white, std::nullopt);

  return con;
}

void EventHandler::commandsMenu(void) {
  static auto menuConsole = buildCommandMenu();
  makePopup([](auto e) { e->restoreMainGame(); },
            [](auto e, auto world, auto &c, auto ts) {
              e->MainGameOnRender(world, c, ts);
            },
            [](auto, Console &c) {
              Console::blit(c, menuConsole,
                            {(c.get_width() - COMMAND_MENU_WIDTH) / 2,
                             (c.get_height() - COMMAND_MENU_HEIGHT) / 2});
            });
}

std::unique_ptr<Action> EventHandler::MainGameKeyDown(SDL_KeyboardEvent *key,
                                                      flecs::world &ecs) {
  auto speed = 1;
  if (key->mod & SDL_KMOD_SHIFT)
    speed *= 2;
  if (key->mod & SDL_KMOD_CTRL)
    speed *= 3;
  switch (key->scancode) {
  case SDL_SCANCODE_UP:
  case SDL_SCANCODE_KP_8:
  case SDL_SCANCODE_K:
    return std::make_unique<BumpAction>(0, -1, speed);
  case SDL_SCANCODE_DOWN:
  case SDL_SCANCODE_KP_2:
  case SDL_SCANCODE_J:
    return std::make_unique<BumpAction>(0, 1, speed);
  case SDL_SCANCODE_LEFT:
  case SDL_SCANCODE_KP_4:
  case SDL_SCANCODE_H:
    return std::make_unique<BumpAction>(-1, 0, speed);
  case SDL_SCANCODE_RIGHT:
  case SDL_SCANCODE_KP_6:
  case SDL_SCANCODE_L:
    return std::make_unique<BumpAction>(1, 0, speed);
  case SDL_SCANCODE_HOME:
  case SDL_SCANCODE_KP_7:
  case SDL_SCANCODE_Y:
    return std::make_unique<BumpAction>(-1, -1, speed);
  case SDL_SCANCODE_END:
  case SDL_SCANCODE_KP_1:
  case SDL_SCANCODE_B:
    return std::make_unique<BumpAction>(-1, 1, speed);
  case SDL_SCANCODE_PAGEUP:
  case SDL_SCANCODE_KP_9:
  case SDL_SCANCODE_U:
    return std::make_unique<BumpAction>(1, -1, speed);
  case SDL_SCANCODE_PAGEDOWN:
  case SDL_SCANCODE_KP_3:
  case SDL_SCANCODE_N:
    return std::make_unique<BumpAction>(1, 1, speed);

  case SDL_SCANCODE_PERIOD:
    if (key->mod & SDL_KMOD_SHIFT) {
      return std::make_unique<TakeStairsAction>();
    }
    // Intentional fallthrough
  case SDL_SCANCODE_KP_5:
  case SDL_SCANCODE_CLEAR:
    return std::make_unique<WaitAction>();

  case SDL_SCANCODE_C:
    commandsMenu();
    return nullptr;
  case SDL_SCANCODE_D: {
    static constexpr char DROP_TITLE[] = "┤Select an item to drop├";
    makeInventoryHandler<DROP_TITLE, &EventHandler::DropItemSelected>(*this,
                                                                      ecs);
    return nullptr;
  }
  case SDL_SCANCODE_F:
    return std::make_unique<RangedTargetAction>();
  case SDL_SCANCODE_G:
    return std::make_unique<PickupAction>();
  case SDL_SCANCODE_I: {
    static constexpr char USE_TITLE[] = "┤Select an item to use├";
    makeInventoryHandler<USE_TITLE, &EventHandler::UseItemSelected>(*this, ecs);
    return nullptr;
  }
  case SDL_SCANCODE_O:
    return std::make_unique<DoorAction>();
  case SDL_SCANCODE_V:
    makeHistoryHandler(*this, ecs);
    return nullptr;
  case SDL_SCANCODE_X:
    makeCharacterScreen(*this);
    return nullptr;
  case SDL_SCANCODE_SLASH:
    makeLookHandler(*this, ecs);
    return nullptr;

  case SDL_SCANCODE_ESCAPE:
    Engine::save_as(ecs, data_dir / saveFilename);
    mainMenu();
    return nullptr;

  default:
    return nullptr;
  }
}

std::unique_ptr<Action> EventHandler::GameOverKeyDown(SDL_KeyboardEvent *key,
                                                      flecs::world &) {
  switch (key->scancode) {
  case SDL_SCANCODE_ESCAPE: {
    mainMenu();
    return nullptr;
  }

  default:
    return nullptr;
  }
}

std::unique_ptr<Action> EventHandler::HistoryKeyDown(SDL_KeyboardEvent *key,
                                                     flecs::world &) {
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
    restoreMainGame();
    return nullptr;
  }
}

std::unique_ptr<Action> EventHandler::AskUserKeyDown(SDL_KeyboardEvent *key,
                                                     flecs::world &) {
  switch (key->scancode) {
  case SDL_SCANCODE_LSHIFT:
  case SDL_SCANCODE_RSHIFT:
  case SDL_SCANCODE_LCTRL:
  case SDL_SCANCODE_RCTRL:
  case SDL_SCANCODE_LALT:
  case SDL_SCANCODE_RALT:
    return nullptr;
  default:
    restoreMainGame();
    return nullptr;
  }
}

std::unique_ptr<Action> EventHandler::InventoryKeyDown(SDL_KeyboardEvent *key,
                                                       flecs::world &ecs) {
  auto idx = key->scancode - SDL_SCANCODE_A;
  if (0 <= idx && idx < q.count()) {
    return (this->*item_selected)(q.page(idx, 1).first());
  }
  return AskUserKeyDown(key, ecs);
}

std::unique_ptr<Action> EventHandler::SelectKeyDown(SDL_KeyboardEvent *key,
                                                    flecs::world &ecs) {
  auto dxy = std::array<int, 2>{0, 0};

  switch (key->scancode) {
  case SDL_SCANCODE_LSHIFT:
  case SDL_SCANCODE_RSHIFT:
  case SDL_SCANCODE_LCTRL:
  case SDL_SCANCODE_RCTRL:
  case SDL_SCANCODE_LALT:
  case SDL_SCANCODE_RALT:
    return nullptr;

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

  case SDL_SCANCODE_RETURN:
  case SDL_SCANCODE_RETURN2:
  case SDL_SCANCODE_KP_ENTER:
    return (this->*loc_selected)(mouse_loc);

  case SDL_SCANCODE_F:
    if (useF) {
      return (this->*loc_selected)(mouse_loc);
    }
    // Intentional fallthrough

  default:
    restoreMainGame();
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

  auto currentMap = ecs.lookup("currentMap").target<CurrentMap>();
  auto &map = currentMap.get<GameMap>();
  mouse_loc[0] =
      std::clamp(mouse_loc[0] + dxy[0] * modifier, 0, map.getWidth());
  mouse_loc[1] =
      std::clamp(mouse_loc[1] + dxy[1] * modifier, 0, map.getHeight());
  return nullptr;
}

std::unique_ptr<Action> EventHandler::MainMenuKeyDown(SDL_KeyboardEvent *key,
                                                      flecs::world &ecs) {
  switch (key->scancode) {
  case SDL_SCANCODE_Q:
  case SDL_SCANCODE_ESCAPE:
    return std::make_unique<ExitAction>();
  case SDL_SCANCODE_C:
    if (Engine::load(ecs, data_dir / saveFilename, *this)) {
      restoreMainGame();
    }
    break;
  case SDL_SCANCODE_N: {
    assert(mouse_loc[0] >= 0);
    assert(mouse_loc[1] >= 0);
    Engine::clear_game_data(ecs);
    Engine::new_game(ecs, dim[0], dim[1] - HUD_HEIGHT - 2);
    restoreMainGame();
    break;
  }

  default:
    return nullptr;
  }

  return nullptr;
}

std::unique_ptr<Action> EventHandler::PopupKeyDown(SDL_KeyboardEvent *key,
                                                   flecs::world &) {
  switch (key->scancode) {
  case SDL_SCANCODE_ESCAPE:
  case SDL_SCANCODE_RETURN:
  case SDL_SCANCODE_RETURN2:
  case SDL_SCANCODE_KP_ENTER:
    parent(this);
    break;
  default:
    break;
  }
  return nullptr;
}

std::unique_ptr<Action> EventHandler::LevelUpKeyDown(SDL_KeyboardEvent *key,
                                                     flecs::world &ecs) {
  auto player = ecs.lookup("player");
  auto &level = player.get_mut<Level>();
  auto msg = "";
  switch (key->scancode) {
  case SDL_SCANCODE_LSHIFT:
  case SDL_SCANCODE_RSHIFT:
  case SDL_SCANCODE_LCTRL:
  case SDL_SCANCODE_RCTRL:
  case SDL_SCANCODE_LALT:
  case SDL_SCANCODE_RALT:
    return nullptr;
  case SDL_SCANCODE_A:
    msg = level.increase_max_hp(player);
    break;
  case SDL_SCANCODE_B:
    msg = level.increase_power(player);
    break;
  case SDL_SCANCODE_C:
    msg = level.increase_defense(player);
    break;
  default:
    return std::make_unique<MessageAction>("Invalid entry.", color::invalid);
  }
  restoreMainGame();
  return std::make_unique<MessageAction>(msg);
}

std::unique_ptr<Action> EventHandler::JumpKeyDown(SDL_KeyboardEvent *key,
                                                  flecs::world &ecs) {
  switch (key->scancode) {
  case SDL_SCANCODE_Y:
    if (useRope)
      item.destruct();
    return std::make_unique<JumpAction>(useRope);
  case SDL_SCANCODE_J:
    if (useRope) {
      return std::make_unique<JumpAction>(false);
    }
    break;
  default:
    break;
  }
  return AskUserKeyDown(key, ecs);
}

std::unique_ptr<Action> EventHandler::EmptyClick(SDL_MouseButtonEvent *,
                                                 flecs::world) {
  return nullptr;
}

std::unique_ptr<Action>
EventHandler::MainGameClick(SDL_MouseButtonEvent *button, flecs::world) {
  if (commandBox[0] <= (int)button->x &&
      (int)button->x < commandBox[0] + commandBox[2] &&
      commandBox[1] <= (int)button->y &&
      (int)button->y < commandBox[1] + commandBox[3]) {
    commandsMenu();
    return nullptr;
  }
  return nullptr;
}

std::unique_ptr<Action> EventHandler::AskUserClick(SDL_MouseButtonEvent *,
                                                   flecs::world) {
  restoreMainGame();
  return nullptr;
}

std::unique_ptr<Action> EventHandler::SelectClick(SDL_MouseButtonEvent *button,
                                                  flecs::world ecs) {
  auto currentMap = ecs.lookup("currentMap").target<CurrentMap>();
  auto &map = currentMap.get<GameMap>();
  if (map.inBounds((int)button->x, (int)button->y)) {
    if (button->button == SDL_BUTTON_LEFT) {
      return (this->*loc_selected)({(int)button->x, (int)button->y});
    }
  }
  return AskUserClick(button, ecs);
}

std::unique_ptr<Action> EventHandler::LevelUpClick(SDL_MouseButtonEvent *,
                                                   flecs::world) {
  return nullptr;
}

void EventHandler::MainGameOnRender(flecs::world ecs, Console &console,
                                    uint64_t) {

  auto map = ecs.lookup("currentMap").target<CurrentMap>();
  auto &gMap = map.get_mut<GameMap>();
  gMap.render(console);

  ecs.lookup("messageLog")
      .get<MessageLog>()
      .render(console, BAR_WIDTH + 1, dim[1] - HUD_HEIGHT,
              dim[0] - (BAR_WIDTH + 1) - (COMMAND_BUTTON_WIDTH + 1) - 1,
              HUD_HEIGHT);

  auto q =
      ecs.query_builder<const Position, const Renderable, const Openable *>(
             "module::renderable")
          .with(flecs::ChildOf, map)
          .order_by<const Renderable>([](auto, auto r1, auto, auto r2) {
            return static_cast<int>(r1->layer) - static_cast<int>(r2->layer);
          })
          .build();

  q.each([&](auto p, auto r, auto openable) {
    if (gMap.isInFov(p)) {
      r.render(console, p, true);
    } else if (gMap.isSensed(p) && openable) {
      r.render(console, p, true);
    } else if (gMap.isExplored(p)) {
      r.render(console, p, false);
    }
  });

  auto player = ecs.lookup("player");
  player.get<Renderable>().render(console, player.get<Position>(), true);

  auto fighter = player.get<Fighter>();
  renderBar(console, fighter.hp(), fighter.max_hp, 0, dim[1] - HUD_HEIGHT,
            BAR_WIDTH);
  renderSmell(console, player, 0, dim[1] - HUD_HEIGHT + 2, BAR_WIDTH);
  renderDungeonLevel(console, gMap.level, {0, dim[1] - HUD_HEIGHT + 4});
  assert(mouse_loc[0] >= 0);
  assert(mouse_loc[1] >= 0);
  renderNamesAtMouseLocation(console, {BAR_WIDTH + 1, dim[1] - HUD_HEIGHT - 1},
                             mouse_loc, map, gMap);
  renderCommandButton(console, commandBox);
}

void EventHandler::HistoryOnRender(flecs::world ecs, Console &console,
                                   uint64_t time) {
  MainGameOnRender(ecs, console, time);
  auto logConsole = Console(console.get_width() - 6, console.get_height() - 6);
  Console::draw_frame(logConsole,
                      {0, 0, logConsole.get_width(), logConsole.get_height()},
                      DECORATION, std::nullopt, std::nullopt);
  Console::print_rect(logConsole, {0, 0, logConsole.get_width(), 1},
                      "┤Message history├", std::nullopt, std::nullopt,
                      TCOD_CENTER);
  ecs.lookup("messageLog")
      .get<MessageLog>()
      .render(logConsole, 1, 1, logConsole.get_width() - 2,
              logConsole.get_height() - 2, cursor);
  Console::blit(console, logConsole, {3, 3});
}

static int menuXLocation(flecs::entity player) {
  return player.get<Position>().x <= 30 ? 40 : 0;
}

void EventHandler::InventoryOnRender(flecs::world ecs, Console &console,
                                     uint64_t time) {
  MainGameOnRender(ecs, console, time);
  auto count = q.count();
  auto x = menuXLocation(ecs.lookup("player"));

  Console::draw_frame(console,
                      {x, 0, (int)title.size(), std::max(count + 2, 3)},
                      DECORATION, color::white, color::black);
  Console::print_rect(console, {x, 0, (int)title.size(), 1}, title,
                      std::nullopt, std::nullopt, TCOD_CENTER);
  if (count > 0) {
    auto player = ecs.lookup("player");
    auto idx = 0;
    q.each([&](flecs::entity e, const auto &name) {
      auto msg = tcod::stringf("(%c) %s%s", 'a' + idx, name.name.c_str(),
                               isEquipped(player, e) ? " (E)" : "");
      Console::print(console, {x + 1, idx + 1}, msg, std::nullopt,
                     std::nullopt);
      idx++;
    });
  } else {
    Console::print(console, {x + 1, 1}, "(Empty)", std::nullopt, std::nullopt);
  }
}

void EventHandler::SelectOnRender(flecs::world ecs, Console &console,
                                  uint64_t time) {
  MainGameOnRender(ecs, console, time);
  auto &tile = console.at(mouse_loc);
  tile.bg = color::white;
}

void EventHandler::AreaTargetOnRender(flecs::world ecs, Console &console,
                                      uint64_t time) {
  SelectOnRender(ecs, console, time);
  Console::draw_frame(console,
                      {mouse_loc[0] - radius - 1, mouse_loc[1] - radius - 1,
                       radius * radius, radius * radius},
                      DECORATION, color::red, std::nullopt);
}

void EventHandler::MainMenuOnRender(flecs::world, Console &console, uint64_t) {
  assert(mouse_loc[0] >= 0);
  assert(mouse_loc[1] >= 0);
  static constexpr auto ImageWidth = 100;
  // static const auto background_image = TCODImage("assets/teeth.png");
  // assert(background_image.getSize()[0] == ImageWidth);
  // tcod::draw_quartergraphics(console, background_image);

  const auto printY = (ImageWidth / 2 + console.get_width()) / 2;
  Console::print(console, {printY, console.get_height() / 2 - 4},
                 "The Fiend in Facility 14", color::menu_title, std::nullopt,
                 TCOD_CENTER);
  Console::print(console, {printY, console.get_height() - 2}, "By degustaf",
                 color::menu_title, std::nullopt, TCOD_CENTER);

  static const auto choices =
      std::array{"[N] Play a new game     ", "[C] Continue last game  ",
                 "[Q] Quit                "};
  for (auto i = 0; i < (int)choices.size(); i++) {
    Console::print(console, {printY, console.get_height() / 2 - 2 + i},
                   choices[i], color::menu_text, color::black, TCOD_CENTER);
  }
}

void EventHandler::PopupOnRender(flecs::world ecs, Console &console,
                                 uint64_t time) {
  parentOnRender(this, ecs, console, time);
  for (auto &tile : console) {
    tile.fg /= 8;
    tile.bg /= 8;
  }

  childOnRender(ecs, console);
}

void EventHandler::LevelUpOnRender(flecs::world ecs, Console &console,
                                   uint64_t time) {
  MainGameOnRender(ecs, console, time);
  auto player = ecs.lookup("player");
  auto x = menuXLocation(player);
  Console::draw_frame(console, {x, 0, 35, 8}, DECORATION, color::white,
                      color::black);
  Console::print_rect(console, {x, 0, 35, 1}, title, std::nullopt, std::nullopt,
                      TCOD_CENTER);
  Console::print(console, {x + 1, 1}, "Congratulations! You level up!",
                 std::nullopt, std::nullopt);
  Console::print(console, {x + 1, 2}, "Select an attribute to increase.",
                 std::nullopt, std::nullopt);

  auto fighter = player.get<Fighter>();
  auto msg = tcod::stringf("a) Constitution (+20 HP, from %d)", fighter.max_hp);
  Console::print(console, {x + 1, 4}, msg, std::nullopt, std::nullopt);
  msg = tcod::stringf("b) Strength (+1 attack, from %d)",
                      fighter.power(player, false));
  Console::print(console, {x + 1, 5}, msg, std::nullopt, std::nullopt);
  msg = tcod::stringf("c) Agility (+1 defense, from %d)",
                      fighter.defense(player));
  Console::print(console, {x + 1, 6}, msg, std::nullopt, std::nullopt);
}

void EventHandler::CharacterScreenOnRender(flecs::world ecs, Console &console,
                                           uint64_t time) {
  MainGameOnRender(ecs, console, time);
  auto player = ecs.lookup("player");
  auto x = menuXLocation(player);
  Console::draw_frame(console, {x, 0, (int)title.size() + 4, 5}, DECORATION,
                      color::white, color::black);
  Console::print_rect(console, {x, 0, (int)title.size() + 4, 1}, title,
                      std::nullopt, std::nullopt, TCOD_CENTER);

  // auto level = player.get<Level>();
  // auto msg = tcod::stringf("Level: %d", level.current);
  // Console::print(console, {x + 1, 1}, msg, std::nullopt, std::nullopt);
  // msg = tcod::stringf("XP: %d", level.xp);
  // Console::print(console, {x + 1, 2}, msg, std::nullopt, std::nullopt);
  // msg = tcod::stringf("XP for next level: %d", level.xp_to_next_level());
  // Console::print(console, {x + 1, 3}, msg, std::nullopt, std::nullopt);

  auto fighter = player.get<Fighter>();
  auto msg = tcod::stringf("Attack: %d", fighter.power(player, false));
  Console::print(console, {x + 1, 1}, msg, std::nullopt, std::nullopt);
  msg = tcod::stringf("Ranged attack: %d", fighter.power(player, true));
  Console::print(console, {x + 1, 2}, msg, std::nullopt, std::nullopt);
  msg = tcod::stringf("Defense: %d", fighter.defense(player));
  Console::print(console, {x + 1, 3}, msg, std::nullopt, std::nullopt);
}

void EventHandler::JumpOnRender(flecs::world ecs, Console &console,
                                uint64_t time) {
  MainGameOnRender(ecs, console, time);
  for (auto &tile : console) {
    tile.fg /= 8;
    tile.bg /= 8;
  }

  if (useRope) {
    Console::print(console, {console.get_width() / 2, console.get_height() / 2},
                   "Are you sure you want to climb into the chasm?",
                   color::white, color::black, TCOD_CENTER);
    Console::print(
        console, {console.get_width() / 2, console.get_height() / 2 + 2},
        "(Y)es     (N)o     (J)ump", color::red, color::black, TCOD_CENTER);
  } else {
    Console::print(console, {console.get_width() / 2, console.get_height() / 2},
                   "Are you sure you want to jump into the chasm?",
                   color::white, color::black, TCOD_CENTER);
    Console::print(console,
                   {console.get_width() / 2, console.get_height() / 2 + 2},
                   "(Y)es     (N)o", color::red, color::black, TCOD_CENTER);
  }
}

void EventHandler::WinOnRender(flecs::world, Console &console, uint64_t time) {
  static auto start_time = time;
  static auto last_time = time;
  auto x = console.get_width() / 2;
  auto y = console.get_height() / 2;
  auto time_ms = time - start_time;
  auto ts = time - last_time;
  auto level = (uint8_t)((time_ms >> 2) & 0xff);

  switch (time_ms >> 10) {
  case 0:
    Console::print(console, {x, y - 1}, "The Fiend is defeated",
                   TCOD_ColorRGB{level, level, level}, std::nullopt,
                   TCOD_CENTER);
    break;
  case 1:
    Console::print(console, {x, y - 1}, "The Fiend is defeated", color::white,
                   std::nullopt, TCOD_CENTER);
    Console::print(console, {x - 4, y + 1}, "You Win.",
                   TCOD_ColorRGB{level, level, level}, std::nullopt);
    break;
  case 2:
    Console::print(console, {x, y - 1}, "The Fiend is defeated", color::white,
                   std::nullopt, TCOD_CENTER);
    Console::print(console, {x - 4, y + 1}, "You Win.", color::white,
                   std::nullopt);
    break;
  case 3:
  case 4:
    Console::print(console, {x, y - 1}, "The Fiend is defeated", color::white,
                   std::nullopt, TCOD_CENTER);
    Console::print(console, {x - 4, y + 1}, "You Win.", color::white,
                   std::nullopt);
    break;
  case 5: {
    Console::print(console, {x, y - 1}, "The Fiend is defeated", color::white,
                   std::nullopt, TCOD_CENTER);
    Console::print(console, {x - 4, y + 1}, "You Win.", color::white,
                   std::nullopt);
    auto &tile = console.at({x + 4, y + 1});
    tile.ch = '.';
    tile.fg = TCOD_ColorRGB{level, level, level};
    break;
  }
  case 6: {
    Console::print(console, {x, y - 1}, "The Fiend is defeated", color::white,
                   std::nullopt, TCOD_CENTER);
    Console::print(console, {x - 4, y + 1}, "You Win..", color::white,
                   std::nullopt);
    auto &tile = console.at({x + 5, y + 1});
    tile.ch = '.';
    tile.fg = TCOD_ColorRGB{level, level, level};
    break;
  }
  case 7: {
    Console::print(console, {x, y - 1}, "The Fiend is defeated", color::white,
                   std::nullopt, TCOD_CENTER);
    Console::print(console, {x - 4, y + 1}, "You Win...", color::white,
                   std::nullopt);
    Console::print(console, {x - 4, y + 3}, "for now",
                   TCOD_ColorRGB{level, level, level}, std::nullopt);
    break;
  }
  default: {
    auto rng = TCODRandom::getInstance();
    Console::print(console, {x, y - 1}, "The Fiend is defeated", color::white,
                   std::nullopt, TCOD_CENTER);
    if (rng->getDouble(0.0, 0.25) < (double)ts / 1000.0) {
      auto i = rng->getInt(0, 20);
      drops.push_back(BloodDrop(x - 10 + i));
    }
    for (auto &d : drops) {
      d.update(ts);
    }
    for (auto it = drops.begin(); it != drops.end();) {
      if (it->y() + y >= console.get_height()) {
        it = drops.erase(it);
      } else {
        it++;
      }
    }
    for (auto &d : drops) {
      d.render(console, y);
    }
    Console::print(console, {x - 4, y + 1}, "You Win...", color::white,
                   std::nullopt);
    Console::print(console, {x - 4, y + 3}, "for now", color::white,
                   std::nullopt);
    break;
  }
  }
  last_time = time;
}

ActionResult
EventHandler::MainGameHandleAction(flecs::world ecs,
                                   std::unique_ptr<Action> action) {
  if (action) {
    auto player = ecs.entity("player");
    auto ret = action->perform(player);
    auto &log = ecs.lookup("messageLog").get_mut<MessageLog>();
    if (ret.msg.size() > 0) {
      log.addMessage(ret.msg, ret.fg);
    }
    auto &scent = player.get_mut<Scent>();
    if (ret) {
      auto map = ecs.lookup("currentMap").target<CurrentMap>();
      auto &gameMap = map.get_mut<GameMap>();
      gameMap.update_fov(player);
      Engine::handle_enemy_turns(ecs);
      scent += {ScentType::player, ret.exertion};
      gameMap.update_scent(map);
      auto scentMessage = gameMap.detectScent(player);
      if (scentMessage.size() > 0) {
        log.addMessage(scentMessage);
      }
    }
    // if (player.has<TrackerConsumable>()) {
    //   auto &t = player.get_mut<TrackerConsumable>();
    //   t.turns--;
    //   if (t.turns < 0) {
    //     player.remove<TrackerConsumable>();
    //   }
    // }
    if (player.get<Level>().requires_level_up()) {
      makeLevelUp(*this);
    } else if (scent.power > 100) {
      auto &warning = player.get_mut<ScentWarning>();
      if (!warning.warned) {
        makePopup([](auto e) { e->restoreMainGame(); },
                  [](auto e, auto world, auto &c, auto ts) {
                    e->MainGameOnRender(world, c, ts);
                  },
                  [](auto, auto &c) {
                    Console::print(
                        c, {c.get_width() / 2, c.get_height() / 2 - 1},
                        "Be careful...", color::red, color::black, TCOD_CENTER);
                    Console::print(c,
                                   {c.get_width() / 2, c.get_height() / 2 + 1},
                                   "The Fiend can track you by your scent.",
                                   color::white, color::black, TCOD_CENTER);
                  });
        warning.warned = true;
      }
    }
    return ret;
  }
  return {ActionResultType::Failure, "", 0.0f};
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
  return {ActionResultType::Failure, "", 0.0f};
}

ActionResult EventHandler::AskUserHandleAction(flecs::world ecs,
                                               std::unique_ptr<Action> action) {
  auto ret = MainGameHandleAction(ecs, std::move(action));
  if (ret && handle_action == &EventHandler::AskUserHandleAction) {
    // if handle_action != AskUserHandleAction, we've already updated which
    // version of the event handler we're using. Don't override that.
    restoreMainGame();
  }
  return ret;
}

std::unique_ptr<Action> EventHandler::DropItemSelected(flecs::entity item) {
  return std::make_unique<DropItemAction>(item);
}

std::unique_ptr<Action> EventHandler::UseItemSelected(flecs::entity item) {
  if (isConsumable(item)) {
    return std::make_unique<ItemAction>(item);
  }
  if (item.has<Equippable>()) {
    return std::make_unique<EquipAction>(item);
  }
  return nullptr;
}

std::unique_ptr<Action> EventHandler::LookSelectedLoc(std::array<int, 2>) {
  restoreMainGame();
  return nullptr;
}

std::unique_ptr<Action>
EventHandler::SingleTargetSelectedLoc(std::array<int, 2> xy) {
  restoreMainGame();
  return callback(xy);
}
