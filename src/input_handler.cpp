#include "input_handler.hpp"

#include <algorithm>
#include <array>
#include <cassert>
#include <memory>
#include <optional>

#include "action.hpp"
#include "color.hpp"
#include "consumable.hpp"
#include "defines.hpp"
#include "engine.hpp"
#include "inventory.hpp"
#include "level.hpp"
#include "message_log.hpp"
#include "render_functions.hpp"

static constexpr auto COMMAND_MENU_WIDTH = 50;
static constexpr auto COMMAND_MENU_HEIGHT = 28;

static char key(SDL_Scancode c) {
  return *SDL_GetKeyName(SDL_GetKeyFromScancode(c, SDL_KMOD_NONE, true));
}

static tcod::Console buildCommandMenu(void) {
  auto con = tcod::Console(COMMAND_MENU_WIDTH, COMMAND_MENU_HEIGHT);
  con.clear();
  tcod::draw_frame(con, {0, 0, COMMAND_MENU_WIDTH, COMMAND_MENU_HEIGHT},
                   DECORATION, color::menu_border, std::nullopt);

  tcod::print(con, {COMMAND_MENU_WIDTH / 2 - 4, 1}, "Commands", color::white,
              std::nullopt);

  // vim directions
  auto y = 3;
  tcod::print(con, {3, y}, "y k u", color::white, std::nullopt);
  con.at({3, y}).ch = key(SDL_SCANCODE_Y);
  con.at({5, y}).ch = key(SDL_SCANCODE_K);
  con.at({7, y}).ch = key(SDL_SCANCODE_U);
  y++;
  tcod::print(con, {4, y}, "\\|/", color::white, std::nullopt);
  y++;
  tcod::print(con, {3, y}, "h-*-l", color::white, std::nullopt);
  con.at({3, y}).ch = key(SDL_SCANCODE_H);
  con.at({7, y}).ch = key(SDL_SCANCODE_L);
  y++;
  tcod::print(con, {4, y}, "/|\\", color::white, std::nullopt);
  y++;
  tcod::print(con, {3, y}, "b j n", color::white, std::nullopt);
  con.at({3, y}).ch = key(SDL_SCANCODE_B);
  con.at({5, y}).ch = key(SDL_SCANCODE_J);
  con.at({7, y}).ch = key(SDL_SCANCODE_N);

  // numpad directions.
  tcod::print(con, {10, 2}, "numpad", color::white, std::nullopt);
  tcod::print(con, {10, 3}, "7 8 9", color::white, std::nullopt);
  tcod::print(con, {11, 4}, "\\|/", color::white, std::nullopt);
  tcod::print(con, {10, 5}, "4-*-6", color::white, std::nullopt);
  tcod::print(con, {11, 6}, "/|\\", color::white, std::nullopt);
  tcod::print(con, {10, 7}, "1 2 3", color::white, std::nullopt);

  y = 9;

  tcod::print(con, {2, y++}, "Hold <shift> to move 2 spaces", color::white,
              std::nullopt);
  tcod::print(con, {2, y++}, "Hold <ctrl> to move 3 spaces", color::white,
              std::nullopt);
  y++;
  tcod::print(con, {2, y++}, "5: wait", color::white, std::nullopt);
  tcod::print(con, {2, y}, ".: wait", color::white, std::nullopt);
  con.at({2, y++}).ch = key(SDL_SCANCODE_PERIOD);
  tcod::print(con, {2, y}, ">: Take elevator", color::white, std::nullopt);
  con.at({2, y++}).ch = key(SDL_SCANCODE_COMMA);
  tcod::print(con, {2, y}, "C: This menu", color::white, std::nullopt);
  con.at({2, y++}).ch = key(SDL_SCANCODE_C);
  tcod::print(con, {2, y}, "D: Drop an item", color::white, std::nullopt);
  con.at({2, y++}).ch = key(SDL_SCANCODE_D);
  tcod::print(con, {2, y}, "F: Fire a weapon", color::white, std::nullopt);
  con.at({2, y++}).ch = key(SDL_SCANCODE_F);
  tcod::print(con, {2, y}, "G: Pick up an item", color::white, std::nullopt);
  con.at({2, y++}).ch = key(SDL_SCANCODE_G);
  tcod::print(con, {2, y}, "I: Inventory", color::white, std::nullopt);
  con.at({2, y++}).ch = key(SDL_SCANCODE_I);
  tcod::print(con, {2, y}, "V: View game log", color::white, std::nullopt);
  con.at({2, y++}).ch = key(SDL_SCANCODE_V);
  tcod::print(con, {2, y}, "X: View character information", color::white,
              std::nullopt);
  con.at({2, y++}).ch = key(SDL_SCANCODE_X);
  tcod::print(con, {2, y}, "/: Look around map", color::white, std::nullopt);
  con.at({2, y++}).ch = key(SDL_SCANCODE_SLASH);
  auto str = tcod::stringf("%s: Exit menu",
                           SDL_GetKeyName(SDL_GetKeyFromScancode(
                               SDL_SCANCODE_ESCAPE, SDL_KMOD_NONE, true)));
  tcod::print(con, {2, y}, str, color::white, std::nullopt);

  return con;
}

static void commandsMenu(flecs::world ecs, InputHandler &handler) {
  static auto menuConsole = buildCommandMenu();
  auto f = [](auto, tcod::Console &c) {
    tcod::blit(c, menuConsole,
               {(c.get_width() - COMMAND_MENU_WIDTH) / 2,
                (c.get_height() - COMMAND_MENU_HEIGHT) / 2});
  };

  make<PopupInputHandler<MainGameInputHandler, decltype(f)>>(ecs, handler, f);
}

std::unique_ptr<Action> InputHandler::dispatch(SDL_Event *event,
                                               flecs::world &ecs) {
  switch (event->type) {
  case SDL_EVENT_QUIT:
    return std::make_unique<ExitAction>();

  case SDL_EVENT_KEY_DOWN:
    return keyDown(event->key, ecs);

  case SDL_EVENT_MOUSE_MOTION:
    mouse_loc = {(int)event->motion.x, (int)event->motion.y};
    return nullptr;

  case SDL_EVENT_MOUSE_BUTTON_DOWN:
    return click(event->button, ecs);

  default:
    return nullptr;
  }
}

ActionResult InputHandler::handle_action(flecs::world ecs,
                                         std::unique_ptr<Action> action) {
  if (action) {
    auto ret = action->perform(ecs.entity());
    assert(ret.msg.size() == 0);
    assert(!ret);
    return ret;
  }
  return {ActionResultType::Failure, "", 0.0f};
}

std::unique_ptr<Action> MainMenuInputHandler::keyDown(SDL_KeyboardEvent &key,
                                                      flecs::world ecs) {
  switch (key.scancode) {
  case SDL_SCANCODE_Q:
  case SDL_SCANCODE_ESCAPE:
    return std::make_unique<ExitAction>();
  case SDL_SCANCODE_C:
    // This could cause the unique_ptr managing this's lifetime to deallocate
    if (Engine::load(ecs, data_dir / saveFilename, *this)) {
      make<MainGameInputHandler>(ecs);
    }
    break;
  case SDL_SCANCODE_N: {
    Engine::clear_game_data(ecs);
    Engine::new_game(ecs);
    make<MainGameInputHandler>(ecs);
    break;
  }

  default:
    return nullptr;
  }

  return nullptr;
}

void MainMenuInputHandler::on_render(flecs::world, tcod::Console &console,
                                     uint64_t) {
  static constexpr auto ImageWidth = 100;
  // static const auto background_image = TCODImage("assets/teeth.png");
  // assert(background_image.getSize()[0] == ImageWidth);
  // tcod::draw_quartergraphics(console, background_image);

  const auto printY = (ImageWidth / 2 + console.get_width()) / 2;
  tcod::print(console, {printY, console.get_height() / 2 - 4},
              "The Fiend in Facility 14", color::menu_title, std::nullopt,
              TCOD_CENTER);
  tcod::print(console, {printY, console.get_height() - 2}, "By degustaf",
              color::menu_title, std::nullopt, TCOD_CENTER);

  static const auto choices =
      std::array{"[%c] Play a new game     ", "[%c] Continue last game  ",
                 "[%c] Quit                "};
  static const auto keys =
      std::array{key(SDL_SCANCODE_N), key(SDL_SCANCODE_C), key(SDL_SCANCODE_Q)};
  for (auto i = 0; i < (int)choices.size(); i++) {
    auto str = tcod::stringf(choices[i], keys[i]);
    tcod::print(console, {printY, console.get_height() / 2 - 2 + i}, str,
                color::menu_text, color::black, TCOD_CENTER);
  }
}

static auto constexpr commandBox = std::array{62, 45, 13, 3};

void MainHandler::on_render(flecs::world ecs, tcod::Console &console,
                            uint64_t) {
  auto map = ecs.lookup("currentMap").target<CurrentMap>();
  auto &gMap = map.get_mut<GameMap>();
  gMap.render(console);

  ecs.lookup("messageLog").get<MessageLog>().render(console, 21, 45, 40, 5);

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
  renderBar(console, fighter.hp(), fighter.max_hp, 20);
  renderSmell(console, player, 20);
  renderDungeonLevel(console, gMap.level, {0, 49});
  renderNamesAtMouseLocation(console, {21, 44}, mouse_loc, map, gMap);
  renderCommandButton(console, commandBox);
}

ActionResult MainHandler::handle_action(flecs::world ecs,
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
      make<LevelupHandler>(ecs);
    } else if (scent.power > 100) {
      auto &warning = player.get_mut<ScentWarning>();
      if (!warning.warned) {
        auto f = [](auto, auto &c) {
          tcod::print(c, {c.get_width() / 2, c.get_height() / 2 - 1},
                      "Be careful...", color::red, color::black, TCOD_CENTER);
          tcod::print(c, {c.get_width() / 2, c.get_height() / 2 + 1},
                      "The Fiend can track you by your scent.", color::white,
                      color::black, TCOD_CENTER);
        };
        make<PopupInputHandler<MainGameInputHandler, decltype(f)>>(ecs, *this,
                                                                   f);
        warning.warned = true;
      }
    }
    return ret;
  }
  return {ActionResultType::Failure, "", 0.0f};
}

std::unique_ptr<Action> MainGameInputHandler::keyDown(SDL_KeyboardEvent &key,
                                                      flecs::world ecs) {
  auto speed = 1;
  if (key.mod & SDL_KMOD_SHIFT)
    speed *= 2;
  if (key.mod & SDL_KMOD_CTRL)
    speed *= 3;
  switch (key.scancode) {
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

  case SDL_SCANCODE_COMMA:
    return std::make_unique<TakeStairsAction>();

  case SDL_SCANCODE_PERIOD:
  case SDL_SCANCODE_KP_5:
  case SDL_SCANCODE_CLEAR:
    return std::make_unique<WaitAction>();

  case SDL_SCANCODE_C:
    commandsMenu(ecs, *this);
    return nullptr;
  case SDL_SCANCODE_D: {
    make<DropItemInputHandler>(ecs, "┤Select an item to drop├", ecs);
    return nullptr;
  }
  case SDL_SCANCODE_F:
    return std::make_unique<RangedTargetAction>();
  case SDL_SCANCODE_G:
    return std::make_unique<PickupAction>();
  case SDL_SCANCODE_I: {
    make<DropItemInputHandler>(ecs, "┤Select an item to use├", ecs);
    return nullptr;
  }
  case SDL_SCANCODE_O:
    return std::make_unique<DoorAction>();
  case SDL_SCANCODE_V:
    make<HistoryInputHandler>(ecs, ecs);
    return nullptr;
  case SDL_SCANCODE_X:
    make<CharacterScreenInputHandler>(ecs);
    return nullptr;
  case SDL_SCANCODE_SLASH:
    make<LookHandler>(ecs, ecs);
    return nullptr;

  case SDL_SCANCODE_ESCAPE:
    make<MainMenuInputHandler>(ecs);
    return nullptr;

  default:
    return nullptr;
  }
}

std::unique_ptr<Action>
MainGameInputHandler::click(SDL_MouseButtonEvent &button, flecs::world ecs) {
  auto currentMap = ecs.lookup("currentMap").target<CurrentMap>();
  auto &map = currentMap.get<GameMap>();
  if (commandBox[0] <= button.x && button.x < commandBox[0] + commandBox[2] &&
      commandBox[1] <= button.y && button.y < commandBox[1] + commandBox[3]) {
    commandsMenu(ecs, *this);
    return nullptr;
  } else if (map.inBounds((int)button.x, (int)button.y)) {
    auto pos = ecs.lookup("player").get<Position>();
    if (map.isExplored((int)button.x, (int)button.y)) {
      if (pos.distanceSquared({(int)button.x, (int)button.y}) <= 2) {
        return std::make_unique<BumpAction>((int)button.x - pos.x,
                                            (int)button.y - pos.y, 1);
      } else {
        make<PathFinder>(ecs, currentMap, pos,
                         std::array{(int)button.x, (int)button.y});
        return nullptr;
      }
    }
  }
  return nullptr;
}

std::unique_ptr<Action> AskUserInputHandler::keyDown(SDL_KeyboardEvent &key,
                                                     flecs::world ecs) {
  switch (key.scancode) {
  case SDL_SCANCODE_LSHIFT:
  case SDL_SCANCODE_RSHIFT:
  case SDL_SCANCODE_LCTRL:
  case SDL_SCANCODE_RCTRL:
  case SDL_SCANCODE_LALT:
  case SDL_SCANCODE_RALT:
    return nullptr;
  default:
    make<MainGameInputHandler>(ecs);
    return nullptr;
  }
}

std::unique_ptr<Action> AskUserInputHandler::click(SDL_MouseButtonEvent &,
                                                   flecs::world ecs) {
  make<MainGameInputHandler>(ecs);
  return nullptr;
}

ActionResult
AskUserInputHandler::handle_action(flecs::world ecs,
                                   std::unique_ptr<Action> action) {
  auto ret = MainHandler::handle_action(ecs, std::move(action));
  if (ret) {
    if (ecs.get<std::unique_ptr<InputHandler>>().get() == this) {
      // If not, we've already updated the input handler we're using.
      // Don't override that.
      make<MainGameInputHandler>(ecs);
    }
  }
  return ret;
}

std::unique_ptr<Action> InventoryInputHandler::keyDown(SDL_KeyboardEvent &key,
                                                       flecs::world ecs) {
  auto idx = key.key - SDLK_A;
  if (0 <= (int)idx && (int)idx < q.count()) {
    return item_selected(q.page(idx, 1).first());
  }
  return AskUserInputHandler::keyDown(key, ecs);
}

static int menuXLocation(flecs::entity player) {
  return player.get<Position>().x <= 30 ? 40 : 0;
}

void InventoryInputHandler::on_render(flecs::world ecs, tcod::Console &console,
                                      uint64_t time) {
  MainHandler::on_render(ecs, console, time);
  auto count = q.count();
  auto x = menuXLocation(ecs.lookup("player"));

  tcod::draw_frame(console, {x, 0, (int)title.size(), std::max(count + 2, 3)},
                   DECORATION, color::white, color::black);
  tcod::print_rect(console, {x, 0, (int)title.size(), 1}, title, std::nullopt,
                   std::nullopt, TCOD_CENTER);
  if (count > 0) {
    auto player = ecs.lookup("player");
    auto idx = 0;
    q.each([&](flecs::entity e, const auto &name) {
      auto msg = tcod::stringf("(%c) %s%s", 'a' + idx, name.name.c_str(),
                               isEquipped(player, e) ? " (E)" : "");
      tcod::print(console, {x + 1, idx + 1}, msg, std::nullopt, std::nullopt);
      idx++;
    });
  } else {
    tcod::print(console, {x + 1, 1}, "(Empty)", std::nullopt, std::nullopt);
  }
}

std::unique_ptr<Action>
DropItemInputHandler::item_selected(flecs::entity item) {
  return std::make_unique<DropItemAction>(item);
}

std::unique_ptr<Action> UseItemInputHandler::item_selected(flecs::entity item) {
  if (isConsumable(item)) {
    return std::make_unique<ItemAction>(item);
  }
  if (item.has<Equippable>()) {
    return std::make_unique<EquipAction>(item);
  }
  return nullptr;
}

std::unique_ptr<Action> LevelupHandler::keyDown(SDL_KeyboardEvent &key,
                                                flecs::world ecs) {
  auto player = ecs.lookup("player");
  auto &level = player.get_mut<Level>();
  auto msg = "";
  switch (key.scancode) {
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
  make<MainGameInputHandler>(ecs);
  return std::make_unique<MessageAction>(msg);
}

std::unique_ptr<Action> LevelupHandler::click(SDL_MouseButtonEvent &,
                                              flecs::world) {
  return nullptr;
}

void LevelupHandler::on_render(flecs::world ecs, tcod::Console &console,
                               uint64_t time) {
  MainHandler::on_render(ecs, console, time);
  auto player = ecs.lookup("player");
  auto x = menuXLocation(player);
  tcod::draw_frame(console, {x, 0, 35, 8}, DECORATION, color::white,
                   color::black);
  tcod::print_rect(console, {x, 0, 35, 1}, "Level Up", std::nullopt,
                   std::nullopt, TCOD_CENTER);
  tcod::print(console, {x + 1, 1}, "Congratulations! You level up!",
              std::nullopt, std::nullopt);
  tcod::print(console, {x + 1, 2}, "Select an attribute to increase.",
              std::nullopt, std::nullopt);

  auto fighter = player.get<Fighter>();
  auto msg = tcod::stringf("%c) Constitution (+20 HP, from %d)",
                           key(SDL_SCANCODE_A), fighter.max_hp);
  tcod::print(console, {x + 1, 4}, msg, std::nullopt, std::nullopt);
  msg = tcod::stringf("%c) Strength (+1 attack, from %d)", key(SDL_SCANCODE_B),
                      fighter.power(player, false));
  tcod::print(console, {x + 1, 5}, msg, std::nullopt, std::nullopt);
  msg = tcod::stringf("%c) Agility (+1 defense, from %d)", key(SDL_SCANCODE_C),
                      fighter.defense(player));
  tcod::print(console, {x + 1, 6}, msg, std::nullopt, std::nullopt);
}

std::unique_ptr<Action> HistoryInputHandler::keyDown(SDL_KeyboardEvent &key,
                                                     flecs::world ecs) {
  switch (key.scancode) {
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
    make<MainGameInputHandler>(ecs);
    return nullptr;
  }
}

void HistoryInputHandler::on_render(flecs::world, tcod::Console &, uint64_t) {}

void CharacterScreenInputHandler::on_render(flecs::world ecs,
                                            tcod::Console &console,
                                            uint64_t time) {
  MainHandler::on_render(ecs, console, time);
  auto player = ecs.lookup("player");
  auto x = menuXLocation(player);
  auto title = std::string{"Character Information"};
  tcod::draw_frame(console, {x, 0, (int)title.size() + 4, 5}, DECORATION,
                   color::white, color::black);
  tcod::print_rect(console, {x, 0, (int)title.size() + 4, 1}, title,
                   std::nullopt, std::nullopt, TCOD_CENTER);

  // auto level = player.get<Level>();
  // auto msg = tcod::stringf("Level: %d", level.current);
  // tcod::print(console, {x + 1, 1}, msg, std::nullopt, std::nullopt);
  // msg = tcod::stringf("XP: %d", level.xp);
  // tcod::print(console, {x + 1, 2}, msg, std::nullopt, std::nullopt);
  // msg = tcod::stringf("XP for next level: %d", level.xp_to_next_level());
  // tcod::print(console, {x + 1, 3}, msg, std::nullopt, std::nullopt);

  auto fighter = player.get<Fighter>();
  auto msg = tcod::stringf("Attack: %d", fighter.power(player, false));
  tcod::print(console, {x + 1, 1}, msg, std::nullopt, std::nullopt);
  msg = tcod::stringf("Ranged attack: %d", fighter.power(player, true));
  tcod::print(console, {x + 1, 2}, msg, std::nullopt, std::nullopt);
  msg = tcod::stringf("Defense: %d", fighter.defense(player));
  tcod::print(console, {x + 1, 3}, msg, std::nullopt, std::nullopt);
}
std::unique_ptr<Action> LookHandler::loc_selected(flecs::world ecs,
                                                  std::array<int, 2>) {
  make<MainGameInputHandler>(ecs);
  return nullptr;
}

void AreaTargetSelector::on_render(flecs::world ecs, tcod::Console &console,
                                   uint64_t time) {
  SelectInputHandler<true>::on_render(ecs, console, time);
  tcod::draw_frame(console,
                   {mouse_loc[0] - radius - 1, mouse_loc[1] - radius - 1,
                    radius * radius, radius * radius},
                   DECORATION, color::red, std::nullopt);
}

void PathFinder::on_render(flecs::world ecs, tcod::Console &console,
                           uint64_t time) {
  if (path->isEmpty()) {
    MainHandler::on_render(ecs, console, time);
    make<MainGameInputHandler>(ecs);
    return;
  }
  int x, y;
  if (!path->walk(&x, &y, true)) {
    make<MainGameInputHandler>(ecs);
    MainHandler::on_render(ecs, console, time);
    return;
  }
  auto player = ecs.entity("player");
  auto pos = player.get<Position>();
  std::unique_ptr<Action> act =
      std::make_unique<BumpAction>(x - pos.x, y - pos.y, 1);
  auto ret = handle_action(ecs, std::move(act));
  if (!ret) {
    assert(ret.type == ActionResultType::Failure);
    MainHandler::on_render(ecs, console, time);
    return;
  }

  auto map = ecs.lookup("currentMap").target<CurrentMap>();
  auto &gm = map.get<GameMap>();
  auto q = ecs.query_builder<const Position>()
               .with<Fighter>()
               .with(flecs::ChildOf, map)
               .build();
  auto seen = false;
  q.each([&](auto &p) { seen |= gm.isInFov(p); });
  if (seen) {
    make<MainGameInputHandler>(ecs);
  }

  MainHandler::on_render(ecs, console, time);
}

std::unique_ptr<Action> GameOver::keyDown(SDL_KeyboardEvent &key,
                                          flecs::world ecs) {
  switch (key.scancode) {
  case SDL_SCANCODE_ESCAPE: {
    make<MainMenuInputHandler>(ecs);
    return nullptr;
  }

  default:
    return nullptr;
  }
}

void WinScreen::on_render(flecs::world, tcod::Console &console, uint64_t time) {
  static auto start_time = time;
  static auto last_time = time;
  auto x = console.get_width() / 2;
  auto y = console.get_height() / 2;
  auto time_ms = time - start_time;
  auto ts = time - last_time;
  auto level = (uint8_t)((time_ms >> 2) & 0xff);

  switch (time_ms >> 10) {
  case 0:
    tcod::print(console, {x, y - 1}, "The Fiend is defeated",
                TCOD_ColorRGB{level, level, level}, std::nullopt, TCOD_CENTER);
    break;
  case 1:
    tcod::print(console, {x, y - 1}, "The Fiend is defeated", color::white,
                std::nullopt, TCOD_CENTER);
    tcod::print(console, {x - 4, y + 1}, "You Win.",
                TCOD_ColorRGB{level, level, level}, std::nullopt);
    break;
  case 2:
    tcod::print(console, {x, y - 1}, "The Fiend is defeated", color::white,
                std::nullopt, TCOD_CENTER);
    tcod::print(console, {x - 4, y + 1}, "You Win.", color::white,
                std::nullopt);
    break;
  case 3:
  case 4:
    tcod::print(console, {x, y - 1}, "The Fiend is defeated", color::white,
                std::nullopt, TCOD_CENTER);
    tcod::print(console, {x - 4, y + 1}, "You Win.", color::white,
                std::nullopt);
    break;
  case 5: {
    tcod::print(console, {x, y - 1}, "The Fiend is defeated", color::white,
                std::nullopt, TCOD_CENTER);
    tcod::print(console, {x - 4, y + 1}, "You Win.", color::white,
                std::nullopt);
    auto &tile = console.at(x + 4, y + 1);
    tile.ch = '.';
    tile.fg = TCOD_ColorRGB{level, level, level};
    break;
  }
  case 6: {
    tcod::print(console, {x, y - 1}, "The Fiend is defeated", color::white,
                std::nullopt, TCOD_CENTER);
    tcod::print(console, {x - 4, y + 1}, "You Win..", color::white,
                std::nullopt);
    auto &tile = console.at(x + 5, y + 1);
    tile.ch = '.';
    tile.fg = TCOD_ColorRGB{level, level, level};
    break;
  }
  case 7: {
    tcod::print(console, {x, y - 1}, "The Fiend is defeated", color::white,
                std::nullopt, TCOD_CENTER);
    tcod::print(console, {x - 4, y + 1}, "You Win...", color::white,
                std::nullopt);
    tcod::print(console, {x - 4, y + 3}, "for now",
                TCOD_ColorRGB{level, level, level}, std::nullopt);
    break;
  }
  default: {
    auto rng = TCODRandom::getInstance();
    tcod::print(console, {x, y - 1}, "The Fiend is defeated", color::white,
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
    tcod::print(console, {x - 4, y + 1}, "You Win...", color::white,
                std::nullopt);
    tcod::print(console, {x - 4, y + 3}, "for now", color::white, std::nullopt);
    break;
  }
  }
  last_time = time;
}
