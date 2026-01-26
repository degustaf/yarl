#include "input_handler.hpp"

#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <memory>
#include <optional>

#include "action.hpp"
#include "actor.hpp"
#include "blood.hpp"
#include "color.hpp"
#include "console.hpp"
#include "consumable.hpp"
#include "defines.hpp"
#include "engine.hpp"
#include "inventory.hpp"
#include "level.hpp"
#include "message_log.hpp"
#include "render_functions.hpp"
#include "textBox.hpp"

static constexpr auto COMMAND_MENU_WIDTH = 70;
static constexpr auto COMMAND_MENU_HEIGHT = 28;

static char key(SDL_Scancode c) {
  return *SDL_GetKeyName(SDL_GetKeyFromScancode(c, SDL_KMOD_NONE, true));
}

static Console buildCommandMenu(void) {
  auto con = Console(COMMAND_MENU_WIDTH, COMMAND_MENU_HEIGHT);
  con.clear();
  con.draw_frame({0, 0, COMMAND_MENU_WIDTH, COMMAND_MENU_HEIGHT}, DECORATION,
                 color::menu_border, std::nullopt);

  con.print({COMMAND_MENU_WIDTH / 2 - 4, 1}, "Commands", color::white,
            std::nullopt);

  // vim directions
  auto y = 3;
  con.print({3, y}, "y k u", color::white, std::nullopt);
  con.at({3, y}).ch = key(SDL_SCANCODE_Y);
  con.at({5, y}).ch = key(SDL_SCANCODE_K);
  con.at({7, y}).ch = key(SDL_SCANCODE_U);
  y++;
  con.print({4, y}, "\\|/", color::white, std::nullopt);
  y++;
  con.print({3, y}, "h-*-l", color::white, std::nullopt);
  con.at({3, y}).ch = key(SDL_SCANCODE_H);
  con.at({7, y}).ch = key(SDL_SCANCODE_L);
  y++;
  con.print({4, y}, "/|\\", color::white, std::nullopt);
  y++;
  con.print({3, y}, "b j n", color::white, std::nullopt);
  con.at({3, y}).ch = key(SDL_SCANCODE_B);
  con.at({5, y}).ch = key(SDL_SCANCODE_J);
  con.at({7, y}).ch = key(SDL_SCANCODE_N);

  // numpad directions.
  con.print({10, 2}, "numpad", color::white, std::nullopt);
  con.print({10, 3}, "7 8 9", color::white, std::nullopt);
  con.print({11, 4}, "\\|/", color::white, std::nullopt);
  con.print({10, 5}, "4-*-6", color::white, std::nullopt);
  con.print({11, 6}, "/|\\", color::white, std::nullopt);
  con.print({10, 7}, "1 2 3", color::white, std::nullopt);

  y = 9;

  con.print({2, y++}, "Hold <shift> to move 2 spaces", color::white,
            std::nullopt);
  con.print({2, y++}, "Hold <ctrl> to move 3 spaces", color::white,
            std::nullopt);
  y++;
  con.print({2, y++}, "5: wait", color::white, std::nullopt);
  con.print({2, y}, ".: wait", color::white, std::nullopt);
  con.at({2, y++}).ch = key(SDL_SCANCODE_PERIOD);
  con.print({2, y}, ">: Take elevator", color::white, std::nullopt);
  con.at({2, y++}).ch = key(SDL_SCANCODE_COMMA);
  con.print({2, y}, "C: This menu", color::white, std::nullopt);
  con.at({2, y++}).ch = key(SDL_SCANCODE_C);
  con.print({2, y}, "D: Drop an item", color::white, std::nullopt);
  con.at({2, y++}).ch = key(SDL_SCANCODE_D);
  con.print({2, y}, "F: Fire a weapon", color::white, std::nullopt);
  con.at({2, y++}).ch = key(SDL_SCANCODE_F);
  con.print({2, y}, "G: Pick up an item", color::white, std::nullopt);
  con.at({2, y++}).ch = key(SDL_SCANCODE_G);
  con.print({2, y}, "I: Inventory", color::white, std::nullopt);
  con.at({2, y++}).ch = key(SDL_SCANCODE_I);
  con.print({2, y}, "V: View game log", color::white, std::nullopt);
  con.at({2, y++}).ch = key(SDL_SCANCODE_V);
  con.print({2, y}, "X: View character information", color::white,
            std::nullopt);
  con.at({2, y++}).ch = key(SDL_SCANCODE_X);
  con.print({2, y}, "/: Look around map", color::white, std::nullopt);
  con.at({2, y++}).ch = key(SDL_SCANCODE_SLASH);
  auto str =
      stringf("%s: Exit menu", SDL_GetKeyName(SDL_GetKeyFromScancode(
                                   SDL_SCANCODE_ESCAPE, SDL_KMOD_NONE, true)));
  con.print({2, y}, str, color::white, std::nullopt);

  return con;
}

static void commandsMenu(flecs::world ecs, InputHandler &handler) {
  static auto menuConsole = buildCommandMenu();
  auto f = [](auto, Console &c) {
    c.blit(menuConsole, {(c.get_width() - COMMAND_MENU_WIDTH) / 2,
                         (c.get_height() - COMMAND_MENU_HEIGHT) / 2});
  };

  makePopup<decltype(f)>(ecs, f, handler);
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
    assert(mouse_loc[0] >= 0);
    assert(mouse_loc[1] >= 0);
    Engine::clear_game_data(ecs);
    Engine::new_game(ecs, dim[0], dim[1] - HUD_HEIGHT - 2);
    make<MainGameInputHandler>(ecs);
    break;
  }

  default:
    return nullptr;
  }

  return nullptr;
}

void MainMenuInputHandler::on_render(flecs::world, Console &console) {
  static constexpr auto ImageWidth = 100;
  // static const auto background_image = TCODImage("assets/teeth.png");
  // assert(background_image.getSize()[0] == ImageWidth);
  // tcod::draw_quartergraphics(console, background_image);

  const auto printY = (ImageWidth / 2 + console.get_width()) / 2;
  console.print({printY, console.get_height() / 2 - 4},
                "The Fiend in Facility 14", color::menu_title, std::nullopt,
                Console::Alignment::CENTER);
  console.print({printY, console.get_height() - 2}, "By degustaf",
                color::menu_title, std::nullopt, Console::Alignment::CENTER);

  static const auto choices =
      std::array{"[%c] Play a new game     ", "[%c] Continue last game  ",
                 "[%c] Quit                "};
  static const auto keys =
      std::array{key(SDL_SCANCODE_N), key(SDL_SCANCODE_C), key(SDL_SCANCODE_Q)};
  for (auto i = 0; i < (int)choices.size(); i++) {
    auto str = tcod::stringf(choices[i], keys[i]);
    console.print({printY, console.get_height() / 2 - 2 + i}, str,
                  color::menu_text, color::black, Console::Alignment::CENTER);
  }
}

void MainHandler::on_render(flecs::world ecs, Console &console) {
  auto map = ecs.lookup("currentMap").target<CurrentMap>();
  auto &gMap = map.get_mut<GameMap>();
  gMap.render(console);

  ecs.lookup("messageLog")
      .get<MessageLog>()
      .render(console, BAR_WIDTH + 1, dim[1] - HUD_HEIGHT,
              dim[0] - (BAR_WIDTH + 1) - (COMMAND_BUTTON_WIDTH + 1) - 1,
              HUD_HEIGHT);

  auto q =
      ecs.query_builder<const Position, const MoveAnimation *, const Renderable,
                        const Openable *>("module::renderable")
          .with(flecs::ChildOf, map)
          .order_by<const Renderable>([](auto, auto r1, auto, auto r2) {
            return static_cast<int>(r1->layer) - static_cast<int>(r2->layer);
          })
          .build();

  q.each([&](auto p, auto ma, auto r, auto openable) {
    if (gMap.isInFov(p)) {
      if (ma) {
        r.render(console, *ma, true);
      } else {
        r.render(console, p, true);
      }
    } else if (gMap.isSensed(p) && openable) {
      assert(ma == nullptr);
      r.render(console, p, true);
    } else if (gMap.isExplored(p)) {
      if (ma) {
        r.render(console, *ma, false);
      } else {
        r.render(console, p, false);
      }
    }
  });

  auto player = ecs.lookup("player");
  if (player.has<MoveAnimation>()) {
    player.get<Renderable>().render(console, player.get<MoveAnimation>(), true);
  } else {
    player.get<Renderable>().render(console, player.get<Position>(), true);
  }

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
          c.print({c.get_width() / 2, c.get_height() / 2 - 1}, "Be careful...",
                  color::red, color::black, Console::Alignment::CENTER);
          c.print({c.get_width() / 2, c.get_height() / 2 + 1},
                  "The Fiend can track you by your scent.", color::white,
                  color::black, Console::Alignment::CENTER);
        };
        makePopup<decltype(f)>(ecs, f, *this);
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
  if (commandBox[0] <= (int)button.x &&
      (int)button.x < commandBox[0] + commandBox[2] &&
      commandBox[1] <= (int)button.y &&
      (int)button.y < commandBox[1] + commandBox[3]) {
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

void MainGameInputHandler::animate(flecs::world ecs, uint64_t t) {
  auto q = ecs.query<const Position, MoveAnimation>();
  auto dt = (float)(t - time);
  assert(!ecs.is_deferred());
  ecs.defer_begin();
  q.each([dt](flecs::entity e, const Position &p, MoveAnimation &am) {
    am.x += ((float)p.x - am.x) * (1 - std::exp(-am.speed * dt));
    am.y += ((float)p.y - am.y) * (1 - std::exp(-am.speed * dt));
    if (am.x == (float)p.x && am.y == (float)p.y) {
      e.remove<MoveAnimation>();
    }
  });
  ecs.defer_end();

  MainHandler::animate(ecs, t);
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

void InventoryInputHandler::on_render(flecs::world ecs, Console &console) {
  AskUserInputHandler::on_render(ecs, console);
  auto count = q.count();
  auto x = menuXLocation(ecs.lookup("player"));

  console.draw_frame({x, 0, (int)title.size(), std::max(count + 2, 3)},
                     DECORATION, color::white, color::black);
  console.print_rect({x, 0, (int)title.size(), 1}, title);
  if (count > 0) {
    auto player = ecs.lookup("player");
    auto idx = 0;
    q.each([&](flecs::entity e, const auto &name) {
      auto msg = stringf("(%c) %s%s", 'a' + idx, name.name.c_str(),
                         isEquipped(player, e) ? " (E)" : "");
      console.print({x + 1, idx + 1}, msg, std::nullopt, std::nullopt);
      idx++;
    });
  } else {
    console.print({x + 1, 1}, "(Empty)", std::nullopt, std::nullopt);
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

void LevelupHandler::on_render(flecs::world ecs, Console &console) {
  AskUserInputHandler::on_render(ecs, console);
  auto player = ecs.lookup("player");
  auto x = menuXLocation(player);
  console.draw_frame({x, 0, 35, 8}, DECORATION, color::white, color::black);
  console.print_rect({x, 0, 35, 1}, "Level Up");
  console.print({x + 1, 1}, "Congratulations! You level up!", std::nullopt,
                std::nullopt);
  console.print({x + 1, 2}, "Select an attribute to increase.", std::nullopt,
                std::nullopt);

  auto fighter = player.get<Fighter>();
  auto msg = stringf("%c) Constitution (+20 HP, from %d)", key(SDL_SCANCODE_A),
                     fighter.max_hp);
  console.print({x + 1, 4}, msg, std::nullopt, std::nullopt);
  msg = stringf("%c) Strength (+1 attack, from %d)", key(SDL_SCANCODE_B),
                fighter.power(player, false));
  console.print({x + 1, 5}, msg, std::nullopt, std::nullopt);
  msg = stringf("%c) Agility (+1 defense, from %d)", key(SDL_SCANCODE_C),
                fighter.defense(player));
  console.print({x + 1, 6}, msg, std::nullopt, std::nullopt);
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

void HistoryInputHandler::on_render(flecs::world ecs, Console &console) {
  MainHandler::on_render(ecs, console);
  auto logConsole = Console(console.get_width() - 6, console.get_height() - 6);
  logConsole.draw_frame({0, 0, logConsole.get_width(), logConsole.get_height()},
                        DECORATION, std::nullopt, std::nullopt);
  logConsole.print_rect({0, 0, logConsole.get_width(), 1}, "┤Message history├");
  ecs.lookup("messageLog")
      .get<MessageLog>()
      .render(logConsole, 1, 1, logConsole.get_width() - 2,
              logConsole.get_height() - 2, cursor);
  console.blit(logConsole, {3, 3});
}

void CharacterScreenInputHandler::on_render(flecs::world ecs,
                                            Console &console) {
  AskUserInputHandler::on_render(ecs, console);
  auto player = ecs.lookup("player");
  auto x = menuXLocation(player);
  auto title = std::string{"Character Information"};
  console.draw_frame({x, 0, (int)title.size() + 4, 5}, DECORATION, color::white,
                     color::black);
  console.print_rect({x, 0, (int)title.size() + 4, 1}, title);

  // auto level = player.get<Level>();
  // auto msg = stringf("Level: %d", level.current);
  // console.print({x + 1, 1}, msg, std::nullopt, std::nullopt);
  // msg = stringf("XP: %d", level.xp);
  // console.print({x + 1, 2}, msg, std::nullopt, std::nullopt);
  // msg = stringf("XP for next level: %d", level.xp_to_next_level());
  // console.print({x + 1, 3}, msg, std::nullopt, std::nullopt);

  auto fighter = player.get<Fighter>();
  auto msg = stringf("Attack: %d", fighter.power(player, false));
  console.print({x + 1, 1}, msg, std::nullopt, std::nullopt);
  msg = stringf("Ranged attack: %d", fighter.power(player, true));
  console.print({x + 1, 2}, msg, std::nullopt, std::nullopt);
  msg = stringf("Defense: %d", fighter.defense(player));
  console.print({x + 1, 3}, msg, std::nullopt, std::nullopt);
}

std::unique_ptr<Action> LookHandler::loc_selected(flecs::world ecs,
                                                  std::array<int, 2>) {
  make<MainGameInputHandler>(ecs);
  return nullptr;
}

void AreaTargetSelector::on_render(flecs::world ecs, Console &console) {
  TargetSelector<true>::on_render(ecs, console);
  console.draw_frame({mouse_loc[0] - radius - 1, mouse_loc[1] - radius - 1,
                      radius * radius, radius * radius},
                     DECORATION, color::red, std::nullopt);
}

void PathFinder::on_render(flecs::world ecs, Console &console) {
  if (path->isEmpty()) {
    MainHandler::on_render(ecs, console);
    make<MainGameInputHandler>(ecs);
    return;
  }
  int x, y;
  if (!path->walk(&x, &y, true)) {
    make<MainGameInputHandler>(ecs);
    MainHandler::on_render(ecs, console);
    return;
  }
  auto player = ecs.entity("player");
  auto pos = player.get<Position>();
  std::unique_ptr<Action> act =
      std::make_unique<BumpAction>(x - pos.x, y - pos.y, 1);
  auto ret = handle_action(ecs, std::move(act));
  if (!ret) {
    assert(ret.type == ActionResultType::Failure);
    MainHandler::on_render(ecs, console);
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

  MainHandler::on_render(ecs, console);
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

void WinScreen::animate(flecs::world ecs, uint64_t t) {
  auto time_ms = t - start_time;
  auto level = (uint8_t)((time_ms >> 2) & 0xff);
  auto ts = t - time;

  switch (time_ms >> 10) {
  case 0: {
    auto e = ecs.lookup("defeated");
    if (e) {
      e.get_mut<CenterTextBox>().fg = {level, level, level};
    } else {
      e = ecs.entity("defeated");
      e.set<CenterTextBox>({{0, -1},
                            "The Fiend is defeated",
                            {level, level, level},
                            Console::Alignment::CENTER});
    }
    break;
  }
  case 1: {
    ecs.lookup("defeated").get_mut<CenterTextBox>().fg = color::white;

    auto e = ecs.lookup("win");
    if (e) {
      e.get_mut<CenterTextBox>().fg = {level, level, level};
    } else {
      e = ecs.entity("win");
      e.set<CenterTextBox>({{-4, 1},
                            "You Win.",
                            {level, level, level},
                            Console::Alignment::LEFT});
    }
    break;
  }
  case 2:
    ecs.lookup("win").get_mut<CenterTextBox>().fg = color::white;
    break;
  case 5: {
    auto &tb = ecs.lookup("win").get_mut<CenterTextBox>();
    if (tb.text.size() == 8) {
      tb.text.push_back('.');
    }
    break;
  }
  case 6: {
    auto &tb = ecs.lookup("win").get_mut<CenterTextBox>();
    if (tb.text.size() == 9) {
      tb.text.push_back('.');
    }
    break;
  }
  case 7: {
    auto e = ecs.lookup("for now");
    if (e) {
      e.get_mut<CenterTextBox>().fg = {level, level, level};
    } else {
      e = ecs.entity("for now");
      e.set<CenterTextBox>({{-4, 3},
                            "for now",
                            {level, level, level},
                            Console::Alignment::CENTER});
    }
    break;
  }
  case 8:
    ecs.lookup("for now").get_mut<CenterTextBox>().fg = color::white;
    break;
  default:
    auto rng = TCODRandom::getInstance();
    if (rng->getDouble(0.0, 0.25) < (double)ts / 1000.0) {
      auto i = rng->getInt(0, 20);
      ecs.entity().set<BloodDrop>({i - 10});
    }
    ecs.query<BloodDrop>().each([ts](auto &d) { d.update(ts); });
    break;
  }

  GameOver::animate(ecs, t);
}

void WinScreen::on_render(flecs::world ecs, Console &console) {
  auto x = console.get_width() / 2;
  auto y = console.get_height() / 2;

  ecs.query<BloodDrop>().each([y, &console](auto e, auto &d) {
    if (d.y() >= y) {
      e.destruct();
    }
    d.render(console, y);
  });

  ecs.query<CenterTextBox>().each([&](auto &b) {
    console.print({x + b.offset[0], y + b.offset[1]}, b.text, b.fg,
                  std::nullopt, b.alignment);
  });

  GameOver::on_render(ecs, console);
}
