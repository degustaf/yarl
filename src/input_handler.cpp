#include "input_handler.hpp"

#include <algorithm>
#include <array>
#include <cassert>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <optional>

#include "action.hpp"
#include "blood.hpp"
#include "color.hpp"
#include "command.hpp"
#include "consumable.hpp"
#include "defines.hpp"
#include "engine.hpp"
#include "game_map.hpp"
#include "inventory.hpp"
#include "level.hpp"
#include "message_log.hpp"
#include "pathfinding.hpp"
#include "render_functions.hpp"
#include "textBox.hpp"

static constexpr auto COMMAND_MENU_WIDTH = 50;
static constexpr auto COMMAND_MENU_HEIGHT = 28;

static tcod::Console buildCommandMenu(void) {
  auto con = tcod::Console(COMMAND_MENU_WIDTH, COMMAND_MENU_HEIGHT);
  con.clear({' ', color::text, color::background});
  tcod::draw_frame(con, {0, 0, COMMAND_MENU_WIDTH, COMMAND_MENU_HEIGHT},
                   DECORATION, color::menu_border, std::nullopt);

  tcod::print(con, {COMMAND_MENU_WIDTH / 2 - 4, 1}, "Commands", color::text,
              std::nullopt);

  // vim directions
  tcod::print(con, {2, 3}, "y k u", color::text, std::nullopt);
  tcod::print(con, {2, 4}, " \\|/ ", color::text, std::nullopt);
  tcod::print(con, {2, 5}, "h-*-l", color::text, std::nullopt);
  tcod::print(con, {2, 6}, " /|\\ ", color::text, std::nullopt);
  tcod::print(con, {2, 7}, "b j n", color::text, std::nullopt);

  // numpad directions.
  tcod::print(con, {10, 2}, "numpad", color::text, std::nullopt);
  tcod::print(con, {10, 3}, "7 8 9", color::text, std::nullopt);
  tcod::print(con, {10, 4}, " \\|/", color::text, std::nullopt);
  tcod::print(con, {10, 5}, "4-*-6", color::text, std::nullopt);
  tcod::print(con, {10, 6}, " /|\\", color::text, std::nullopt);
  tcod::print(con, {10, 7}, "1 2 3", color::text, std::nullopt);

  auto y = 9;

  // tcod::print(con, {2, y++}, "Hold <shift> to move 2 spaces", color::text,
  // std::nullopt);
  // tcod::print(con, {2, y++}, "Hold <ctrl> to move 3 spaces", color::text,
  // std::nullopt);
  // y++;

  // Remappable commands
  for (auto &cmd : Command::mapping) {
    switch (cmd.second) {
    case CommandType::UL: {
      auto buffer = tcod::stringf("%s", SDL_GetKeyName(SDL_GetKeyFromScancode(
                                            cmd.first, SDL_KMOD_NONE, true)));
      tcod::print(con, {2, 3}, buffer, color::text, std::nullopt);
      break;
    }
    case CommandType::UP: {
      auto buffer = tcod::stringf("%s", SDL_GetKeyName(SDL_GetKeyFromScancode(
                                            cmd.first, SDL_KMOD_NONE, true)));
      tcod::print(con, {4, 3}, buffer, color::text, std::nullopt);
      break;
    }
    case CommandType::UR: {
      auto buffer = tcod::stringf("%s", SDL_GetKeyName(SDL_GetKeyFromScancode(
                                            cmd.first, SDL_KMOD_NONE, true)));
      tcod::print(con, {6, 3}, buffer, color::text, std::nullopt);
      break;
    }

    case CommandType::LEFT: {
      auto buffer = tcod::stringf("%s", SDL_GetKeyName(SDL_GetKeyFromScancode(
                                            cmd.first, SDL_KMOD_NONE, true)));
      tcod::print(con, {2, 5}, buffer, color::text, std::nullopt);
      break;
    }
    case CommandType::RIGHT: {
      auto buffer = tcod::stringf("%s", SDL_GetKeyName(SDL_GetKeyFromScancode(
                                            cmd.first, SDL_KMOD_NONE, true)));
      tcod::print(con, {6, 5}, buffer, color::text, std::nullopt);
      break;
    }

    case CommandType::DL: {
      auto buffer = tcod::stringf("%s", SDL_GetKeyName(SDL_GetKeyFromScancode(
                                            cmd.first, SDL_KMOD_NONE, true)));
      tcod::print(con, {2, 7}, buffer, color::text, std::nullopt);
      break;
    }
    case CommandType::DOWN: {
      auto buffer = tcod::stringf("%s", SDL_GetKeyName(SDL_GetKeyFromScancode(
                                            cmd.first, SDL_KMOD_NONE, true)));
      tcod::print(con, {4, 7}, buffer, color::text, std::nullopt);
      break;
    }
    case CommandType::DR: {
      auto buffer = tcod::stringf("%s", SDL_GetKeyName(SDL_GetKeyFromScancode(
                                            cmd.first, SDL_KMOD_NONE, true)));
      tcod::print(con, {6, 7}, buffer, color::text, std::nullopt);
      break;
    }

    default: {
      auto buffer = tcod::stringf("%s: %s",
                                  SDL_GetKeyName(SDL_GetKeyFromScancode(
                                      cmd.first, SDL_KMOD_NONE, true)),
                                  CommandTypeDescription(cmd.second));
      tcod::print(con, {2, y}, buffer, color::text, std::nullopt);
      y++;
    }
    }
  }

  return con;
}

static void commandsMenu(flecs::world ecs, InputHandler &handler) {
  static auto menuConsole = buildCommandMenu();
  auto f = [](auto, tcod::Console &c) {
    tcod::blit(c, menuConsole,
               {(c.get_width() - COMMAND_MENU_WIDTH) / 2,
                (c.get_height() - COMMAND_MENU_HEIGHT) / 2});
  };

  ecs.set<std::unique_ptr<InputHandler>>(
      std::make_unique<PopupInputHandler<MainGameInputHandler, decltype(f)>>(
          f, handler));
}

std::unique_ptr<Action> InputHandler::dispatch(SDL_Event *event,
                                               flecs::world &ecs) {
  switch (event->type) {
  case SDL_EVENT_QUIT:
    return std::make_unique<ExitAction>();

  case SDL_EVENT_KEY_DOWN:
    return keyDown(Command::get(event->key), ecs);

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

std::unique_ptr<Action> MainMenuInputHandler::keyDown(Command cmd,
                                                      flecs::world ecs) {
  switch (cmd.type) {
  case CommandType::ESCAPE:
    return std::make_unique<ExitAction>();

  case CommandType::UP:
    idx--;
    if (idx == -1)
      idx = choices.size() - 1;
    break;
  case CommandType::DOWN:
    idx++;
    if (idx == choices.size())
      idx = 0;
    break;

  case CommandType::ENTER:
    return processChoice(idx, ecs);
    break;

  default:
    return nullptr;
  }

  return nullptr;
}

std::unique_ptr<Action>
MainMenuInputHandler::click(SDL_MouseButtonEvent &button, flecs::world ecs) {
  auto &console = ecs.get<tcod::Console>();
  const auto printX = (ImageWidth / 2.0f + (float)console.get_width()) / 2.0f;
  for (auto i = 0; i < (int)choices.size(); i++) {
    auto printY = (float)console.get_height() / 2.0f - 2 + (float)i;
    if (button.x >= (float)(printX - (float)strlen(choices[i]) / 2) &&
        printY <= button.y && button.y < printY + 1) {
      return processChoice(i, ecs);
    }
  }
  return nullptr;
}

void MainMenuInputHandler::on_render(flecs::world, tcod::Console &console) {
  // static const auto background_image = TCODImage("assets/teeth.png");
  // assert(background_image.getSize()[0] == ImageWidth);
  // tcod::draw_quartergraphics(console, background_image);

  const auto printX = (ImageWidth / 2 + console.get_width()) / 2;
  tcod::print(console, {printX, console.get_height() / 2 - 4},
              "The Fiend in Facility 14", color::menu_title, std::nullopt,
              TCOD_CENTER);
  tcod::print(console, {printX, console.get_height() - 2}, "By degustaf",
              color::menu_title, std::nullopt, TCOD_CENTER);

  for (auto i = 0; i < (int)choices.size(); i++) {
    auto printY = console.get_height() / 2 - 2 + i;
    if (mouse_loc[0] >= printX - (int)strlen(choices[i]) / 2 &&
        mouse_loc[1] == printY) {
      idx = i;
    }
    auto str = i == idx ? tcod::stringf("\u25BA %s", choices[i]) : choices[i];
    tcod::print(console, {printX, printY}, str, color::menu_text,
                color::background, TCOD_CENTER);
  }
}

std::unique_ptr<Action> MainMenuInputHandler::processChoice(int idx,
                                                            flecs::world ecs) {
  switch (idx) {
  case 0:
    Engine::clear_game_data(ecs);
    Engine::new_game(ecs);
    make<MainGameInputHandler>(ecs);
    return nullptr;
  case 1:
    if (Engine::load(ecs, data_dir / saveFilename, *this)) {
      make<MainGameInputHandler>(ecs);
      return nullptr;
    }
    break;
  case 2:
    make<KeybindMenu>(ecs);
    return nullptr;
  case 3:
    return std::make_unique<ExitAction>();
  default:
    assert(false);
    break;
  }
  return nullptr;
}

std::unique_ptr<Action> KeybindMenu::keyDown(Command cmd, flecs::world ecs) {
  switch (cmd.type) {
  case CommandType::ESCAPE:
    make<MainMenuInputHandler>(ecs);
    return nullptr;
  case CommandType::UP:
    idx--;
    if (idx == -1)
      idx = (int)keys.size() - 1;
    break;
  case CommandType::DOWN:
    idx++;
    if (idx == (int)keys.size())
      idx = 0;
    break;
  case CommandType::ENTER:
    ecs.set<std::unique_ptr<InputHandler>>(
        std::make_unique<KeyBinding>(*this, Command::mapping[keys[idx]]));
    break;

  default:
    break;
  }

  return nullptr;
}

void KeybindMenu::on_render(flecs::world ecs, tcod::Console &console) {
  MainMenuInputHandler::on_render(ecs, console);
  for (auto &tile : console) {
    tile.fg /= 8;
    tile.bg /= 8;
  }

  auto frameX = (console.get_width() - COMMAND_MENU_WIDTH) / 2;
  auto frameY = (console.get_height() - COMMAND_MENU_HEIGHT) / 2;

  tcod::draw_frame(console,
                   {frameX, frameY, COMMAND_MENU_WIDTH, COMMAND_MENU_HEIGHT},
                   DECORATION, color::menu_border, std::nullopt);

  auto y = 0;
  for (auto &key : keys) {
    auto buffer = tcod::stringf(
        "%-25s: %s", CommandTypeDescription(Command::mapping[key]),
        SDL_GetKeyName(SDL_GetKeyFromScancode(key, SDL_KMOD_NONE, true)));
    tcod::print(console, {frameX + 2, frameY + y + 2}, buffer,
                y == idx ? color::background : color::text,
                y == idx ? std::optional(color::menu_border) : std::nullopt);
    y++;
  }
}

std::unique_ptr<Action> KeyBinding::keyDown(Command cmd, flecs::world ecs) {
  switch (cmd.type) {
  case CommandType::ESCAPE:
    ecs.set<std::unique_ptr<InputHandler>>(
        std::make_unique<KeybindMenu>(*this));
    return nullptr;
  case CommandType::NONE: {
    SDL_Keymod mod = SDL_KMOD_NONE;
    auto scan = SDL_GetScancodeFromKey(cmd.ch, &mod);
    Command::set(scan, c);
    keys[idx] = scan;
    ecs.set<std::unique_ptr<InputHandler>>(
        std::make_unique<KeybindMenu>(*this));
    return nullptr;
  }
  default:
    return nullptr;
  }
}

void KeyBinding::on_render(flecs::world ecs, tcod::Console &console) {
  KeybindMenu::on_render(ecs, console);
  for (auto &tile : console) {
    tile.fg /= 8;
    tile.bg /= 8;
  }

  auto str = tcod::stringf("Press a key for %s", CommandTypeDescription(c));

  auto frameX = (int)(console.get_width() - str.length() - 2) / 2;
  auto frameY = (int)(console.get_height() - 3) / 2;

  tcod::draw_frame(console, {frameX, frameY, (int)str.length() + 2, 3},
                   DECORATION, color::menu_border, std::nullopt);
  tcod::print(console, {frameX + 1, frameY + 1}, str, color::menu_text,
              std::nullopt);
}

static auto constexpr commandBox = std::array{62, 45, 13, 3};

void MainHandler::on_render(flecs::world ecs, tcod::Console &console) {
  auto map = ecs.lookup("currentMap").target<CurrentMap>();
  auto &gMap = map.get_mut<GameMap>();
  gMap.render(console, time);

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
      gameMap.update_fov(map, player);
      Engine::handle_enemy_turns(ecs);
      scent += {ScentType::player, ret.exertion};
      gameMap.update_scent(map);
      auto scentMessage = gameMap.detectScent(player);
      if (scentMessage.size() > 0) {
        log.addMessage(scentMessage);
      }
      ecs.lookup("turn").get_mut<Turn>().turn++;
    }
    // if (player.has<TrackerConsumable>()) {
    //   auto &t = player.get_mut<TrackerConsumable>();
    //   t.turns--;
    //   if (t.turns < 0) {
    //     player.remove<TrackerConsumable>();
    //   }
    // }
    if (player.get<Level>().requires_level_up()) {
      ::make<LevelupHandler>(ecs);
    } else if (scent.power > 100) {
      auto &warning = player.get_mut<ScentWarning>();
      if (!warning.warned) {
        auto f = [](auto, auto &c) {
          tcod::print(c, {c.get_width() / 2, c.get_height() / 2 - 1},
                      "Be careful...", color::blood, color::background,
                      TCOD_CENTER);
          tcod::print(c, {c.get_width() / 2, c.get_height() / 2 + 1},
                      "The Fiend can track you by your scent.", color::text,
                      color::background, TCOD_CENTER);
        };
        ecs.set<std::unique_ptr<InputHandler>>(
            std::make_unique<
                PopupInputHandler<MainGameInputHandler, decltype(f)>>(f,
                                                                      *this));
        warning.warned = true;
      }
    }
    return ret;
  }
  return {ActionResultType::Failure, "", 0.0f};
}

std::unique_ptr<Action> MainGameInputHandler::keyDown(Command cmd,
                                                      flecs::world ecs) {
  auto speed = 1;
  // if (key.mod & SDL_KMOD_SHIFT)
  //   speed *= 2;
  // if (key.mod & SDL_KMOD_CTRL)
  //   speed *= 3;
  switch (cmd.type) {
  case CommandType::UP:
    return std::make_unique<BumpAction>(0, -1, speed);
  case CommandType::DOWN:
    return std::make_unique<BumpAction>(0, 1, speed);
  case CommandType::LEFT:
    return std::make_unique<BumpAction>(-1, 0, speed);
  case CommandType::RIGHT:
    return std::make_unique<BumpAction>(1, 0, speed);
  case CommandType::UL:
    return std::make_unique<BumpAction>(-1, -1, speed);
  case CommandType::DL:
    return std::make_unique<BumpAction>(-1, 1, speed);
  case CommandType::UR:
    return std::make_unique<BumpAction>(1, -1, speed);
  case CommandType::DR:
    return std::make_unique<BumpAction>(1, 1, speed);

  case CommandType::STAIRS:
    return std::make_unique<TakeStairsAction>();

  case CommandType::WAIT:
    return std::make_unique<WaitAction>();

  case CommandType::AUTO:
    make<AutoExplore>(ecs, ecs.lookup("currentMap").target<CurrentMap>());
    return nullptr;
  case CommandType::COMMANDS:
    commandsMenu(ecs, *this);
    return nullptr;
  case CommandType::DROP:
    make<DropItemInputHandler>(ecs, "┤Select an item to drop├", ecs);
    return nullptr;
  case CommandType::SHOOT:
    return std::make_unique<RangedTargetAction>();
  case CommandType::GRAB:
    return std::make_unique<PickupAction>();
  case CommandType::INVENTORY:
    make<UseItemInputHandler>(ecs, "┤Select an item to use├", ecs);
    return nullptr;
  case CommandType::OPEN:
    return std::make_unique<DoorAction>();
  case CommandType::MESSAGES:
    make<HistoryInputHandler>(ecs, ecs);
    return nullptr;
  case CommandType::CHARACTER:
    make<CharacterScreenInputHandler>(ecs);
    return nullptr;
  case CommandType::PEEK:
    make<LookHandler>(ecs);
    return nullptr;
  case CommandType::TURN:
    return std::make_unique<SeedAction>();

  case CommandType::ESCAPE:
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

std::unique_ptr<Action> AskUserInputHandler::keyDown(Command cmd,
                                                     flecs::world ecs) {
  switch (cmd.type) {
  case CommandType::SHIFT:
  case CommandType::CTRL:
  case CommandType::ALT:
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

std::unique_ptr<Action> InventoryInputHandler::keyDown(Command cmd,
                                                       flecs::world ecs) {
  auto idx = cmd.ch - SDLK_A;
  if (0 <= (int)idx && (int)idx < q.count()) {
    return item_selected(q.page(idx, 1).first());
  }
  return AskUserInputHandler::keyDown(cmd, ecs);
}

static int menuXLocation(flecs::entity player) {
  return player.get<Position>().x <= 30 ? 40 : 0;
}

void InventoryInputHandler::on_render(flecs::world ecs,
                                      tcod::Console &console) {
  MainHandler::on_render(ecs, console);
  auto count = q.count();
  auto x = menuXLocation(ecs.lookup("player"));

  tcod::draw_frame(console, {x, 0, (int)title.size(), std::max(count + 2, 3)},
                   DECORATION, color::text, color::background);
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

std::unique_ptr<Action> LevelupHandler::keyDown(Command cmd, flecs::world ecs) {
  auto player = ecs.lookup("player");
  auto &level = player.get_mut<Level>();
  auto msg = "";
  switch (cmd.ch) {
  case 'A':
  case 'a':
    msg = level.increase_max_hp(player);
    break;
  case 'B':
  case 'b':
    msg = level.increase_power(player);
    break;
  case 'C':
  case 'c':
    msg = level.increase_defense(player);
    break;
  default:
    return nullptr;
  }
  make<MainGameInputHandler>(ecs);
  return std::make_unique<MessageAction>(msg);
}

std::unique_ptr<Action> LevelupHandler::click(SDL_MouseButtonEvent &,
                                              flecs::world) {
  return nullptr;
}

void LevelupHandler::on_render(flecs::world ecs, tcod::Console &console) {
  MainHandler::on_render(ecs, console);
  auto player = ecs.lookup("player");
  auto x = menuXLocation(player);
  tcod::draw_frame(console, {x, 0, 35, 8}, DECORATION, color::text,
                   color::background);
  tcod::print_rect(console, {x, 0, 35, 1}, "Level Up", std::nullopt,
                   std::nullopt, TCOD_CENTER);
  tcod::print(console, {x + 1, 1}, "Congratulations! You level up!",
              std::nullopt, std::nullopt);
  tcod::print(console, {x + 1, 2}, "Select an attribute to increase.",
              std::nullopt, std::nullopt);

  auto fighter = player.get<Fighter>();
  auto msg = tcod::stringf("%s) Constitution (+20 HP, from %d)",
                           SDL_GetKeyName(SDLK_A), fighter.max_hp);
  tcod::print(console, {x + 1, 4}, msg, std::nullopt, std::nullopt);
  msg = tcod::stringf("%s) Strength (+1 attack, from %d)",
                      SDL_GetKeyName(SDLK_B), fighter.power(player, false));
  tcod::print(console, {x + 1, 5}, msg, std::nullopt, std::nullopt);
  msg = tcod::stringf("%s) Agility (+1 defense, from %d)",
                      SDL_GetKeyName(SDLK_C), fighter.defense(player));
  tcod::print(console, {x + 1, 6}, msg, std::nullopt, std::nullopt);
}

std::unique_ptr<Action> HistoryInputHandler::keyDown(Command cmd,
                                                     flecs::world ecs) {
  switch (cmd.type) {
  case CommandType::UP:
    if (cursor == 0) {
      cursor = log_length - 1;
    } else {
      cursor--;
    }
    return nullptr;
  case CommandType::DOWN:
    if (cursor == log_length - 1) {
      cursor = 0;
    } else {
      cursor++;
    }
    return nullptr;
  case CommandType::PAGEUP:
    cursor = cursor < 10 ? 0 : cursor - 10;
    return nullptr;
  case CommandType::PAGEDOWN:
    cursor = std::min(cursor + 10, log_length - 1);
    return nullptr;
  case CommandType::HOME:
    cursor = 0;
    return nullptr;
  case CommandType::END:
    cursor = log_length - 1;
    return nullptr;

  default:
    make<MainGameInputHandler>(ecs);
    return nullptr;
  }
}

void HistoryInputHandler::on_render(flecs::world ecs, tcod::Console &console) {
  MainHandler::on_render(ecs, console);
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

void CharacterScreenInputHandler::on_render(flecs::world ecs,
                                            tcod::Console &console) {
  MainHandler::on_render(ecs, console);
  auto player = ecs.lookup("player");
  auto x = menuXLocation(player);
  auto title = std::string{"Character Information"};
  tcod::draw_frame(console, {x, 0, (int)title.size() + 4, 5}, DECORATION,
                   color::text, color::background);
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

void AreaTargetSelector::on_render(flecs::world ecs, tcod::Console &console) {
  SelectInputHandler<true>::on_render(ecs, console);
  auto map = ecs.lookup("currentMap").target<CurrentMap>();
  auto &gm = map.get<GameMap>();
  for (auto y = 0; y < console.get_height(); y++) {
    auto dy = mouse_loc[1] - y;
    for (auto x = 0; x < console.get_width(); x++) {
      auto dx = mouse_loc[0] - x;
      if (dx * dx + dy * dy <= radius * radius && gm.isInFov({x, y})) {
        console.at({x, y}).bg = color::areaTarget;
      }
    }
  }
}

std::unique_ptr<Action> AutoMove::keyDown(Command cmd, flecs::world ecs) {
  switch (cmd.type) {
  case CommandType::ESCAPE:
  case CommandType::ENTER:
    make<MainGameInputHandler>(ecs);
    return nullptr;
  default:
    return nullptr;
  }
}

void AutoMove::on_render(flecs::world ecs, tcod::Console &console) {
  auto map = ecs.lookup("currentMap").target<CurrentMap>();
  auto &gm = map.get<GameMap>();
  auto q = ecs.query_builder<const Position, const Fighter>()
               .with(flecs::ChildOf, map)
               .build();
  auto seen = false;
  q.each([&](auto &p, auto &f) { seen |= gm.isInFov(p) && f.isAlive(); });
  MainHandler::on_render(ecs, console);
  if (seen) {
    make<MainGameInputHandler>(ecs);
  }
}

void AutoExplore::on_render(flecs::world ecs, tcod::Console &console) {
  auto player = ecs.entity("player");
  auto pos = player.get<Position>();
  auto &gameMap = map.get<GameMap>();
  auto dij = pathfinding::Dijkstra(
      {gameMap.getWidth(), gameMap.getHeight()},
      [&](auto xy) {
        if (!gameMap.isExplored(xy))
          return true;
        if (map.world()
                .query_builder<const Position>()
                .with<Item>()
                .with(flecs::ChildOf, map)
                .build()
                .find([&](auto p) { return p == xy; })) {
          return true;
        }
        return false;
      },
      [&](auto &xy) {
        auto ret = std::vector<pathfinding::Index>();
        ret.reserve(9); // 8 directions plus a portal
        for (auto &dir : directions) {
          auto next = pathfinding::Index{xy[0] + dir[0], xy[1] + dir[1]};
          if (gameMap.inBounds(next) && gameMap.isWalkable(next)) {
            ret.push_back(next);
          } else if (map.world()
                         .query_builder<const Position>()
                         .with<Openable>()
                         .with(flecs::ChildOf, map)
                         .build()
                         .find([next](auto &p) { return p == next; })) {
            ret.push_back(next);
          }
        }
        flecs::entity e = ecs.query_builder<const Position>()
                              .with(ecs.component<Portal>(), flecs::Wildcard)
                              .with(flecs::ChildOf, map)
                              .build()
                              .find([xy](auto &p) { return p == xy; });
        if (e) {
          ret.push_back(e.target<Portal>().get<Position>());
        }
        return ret;
      },
      [&](auto xy) {
        if (ecs.query_builder<const Position>()
                .with<Openable>()
                .with(flecs::ChildOf, map)
                .build()
                .find([xy](auto &p) { return p == xy; })) {
          if (!gameMap.isWalkable(xy)) {
            return 2;
          }
        }
        return 1;
      });
  dij.scan();
  auto xy = dij.cameFrom[pos];
  if (!gameMap.inBounds(xy)) {
    MainHandler::on_render(ecs, console);
    make<MainGameInputHandler>(ecs);
    return;
  }
  assert(std::abs(xy[0] - pos.x) <= 1);
  assert(std::abs(xy[1] - pos.y) <= 1);
  std::unique_ptr<Action> act =
      std::make_unique<BumpAction>(xy[0] - pos.x, xy[1] - pos.y, 1);
  auto ret = handle_action(ecs, std::move(act));
  if (this == ecs.get<std::unique_ptr<InputHandler>>().get()) {
    AutoMove::on_render(ecs, console);
    if (!ret) {
      assert(ret.type == ActionResultType::Failure);
      // Verify that we haven't already replaced the current inputHandler and
      // freed this.
      if (this == ecs.get<std::unique_ptr<InputHandler>>().get()) {
        make<MainGameInputHandler>(ecs);
      }
    }
  }
}

PathFinder::PathFinder(flecs::entity map, std::array<int, 2> orig,
                       std::array<int, 2> dest, const InputHandler &handler)
    : AutoMove(handler) {
  auto &gameMap = map.get<GameMap>();
  auto ecs = map.world();
  auto dij = pathfinding::Dijkstra(
      {gameMap.getWidth(), gameMap.getHeight()},
      [=](auto xy) { return orig == xy; },
      [&](auto &xy) {
        auto ret = std::vector<pathfinding::Index>();
        ret.reserve(9); // 8 directions plus a portal
        for (auto &dir : directions) {
          auto next = pathfinding::Index{xy[0] + dir[0], xy[1] + dir[1]};
          if (gameMap.inBounds(next) && gameMap.isExplored(next) &&
              gameMap.isWalkable(next)) {
            ret.push_back(next);
          }
        }
        flecs::entity e = ecs.query_builder<Position>()
                              .with(ecs.component<Portal>(), flecs::Wildcard)
                              .with(flecs::ChildOf, map)
                              .build()
                              .find([xy](auto &p) { return p == xy; });
        if (e) {
          ret.push_back(e.target<Portal>().get<Position>());
        }
        return ret;
      },
      [&](auto xy) {
        if (ecs.query_builder<const Position>()
                .with<Openable>()
                .with(flecs::ChildOf, map)
                .build()
                .find([xy](auto &p) { return p == xy; })) {
          if (!gameMap.isWalkable(xy)) {
            return 2;
          }
        }
        return 1;
      });
  dij.scan();
  path = pathfinding::constructPath(orig, dest, dij.cameFrom);
}

void PathFinder::on_render(flecs::world ecs, tcod::Console &console) {
  if (path.empty()) {
    MainHandler::on_render(ecs, console);
    make<MainGameInputHandler>(ecs);
    return;
  }
  auto [x, y] = *path.rbegin();
  path.pop_back();
  auto player = ecs.entity("player");
  auto pos = player.get<Position>();
  std::unique_ptr<Action> act =
      std::make_unique<BumpAction>(x - pos.x, y - pos.y, 1);
  auto ret = handle_action(ecs, std::move(act));
  if (this == ecs.get<std::unique_ptr<InputHandler>>().get()) {
    AutoMove::on_render(ecs, console);
    if (!ret) {
      assert(ret.type == ActionResultType::Failure);
      // Verify that we haven't already replaced the current inputHanf=dler
      // and freed this.
      if (this == ecs.get<std::unique_ptr<InputHandler>>().get()) {
        make<MainGameInputHandler>(ecs);
      }
    }
  }
}

std::unique_ptr<Action> GameOver::keyDown(Command cmd, flecs::world ecs) {
  switch (cmd.type) {
  case CommandType::ESCAPE: {
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
                            TCOD_CENTER});
    }
    break;
  }
  case 1: {
    ecs.lookup("defeated").get_mut<CenterTextBox>().fg = color::text;

    auto e = ecs.lookup("win");
    if (e) {
      e.get_mut<CenterTextBox>().fg = {level, level, level};
    } else {
      e = ecs.entity("win");
      e.set<CenterTextBox>(
          {{-4, 1}, "You Win.", {level, level, level}, TCOD_LEFT});
    }
    break;
  }
  case 2:
    ecs.lookup("win").get_mut<CenterTextBox>().fg = color::text;
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
      e.set<CenterTextBox>(
          {{-4, 3}, "for now", {level, level, level}, TCOD_CENTER});
    }
    break;
  }
  case 8:
    ecs.lookup("for now").get_mut<CenterTextBox>().fg = color::text;
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
}

void WinScreen::on_render(flecs::world ecs, tcod::Console &console) {
  auto x = console.get_width() / 2;
  auto y = console.get_height() / 2;

  ecs.query<BloodDrop>().each([y, &console](auto e, auto &d) {
    if (d.y() >= y) {
      e.destruct();
    }
    d.render(console, y);
  });

  ecs.query<CenterTextBox>().each([&](auto &b) {
    tcod::print(console, {x + b.offset[0], y + b.offset[1]}, b.text, b.fg,
                std::nullopt, b.alignment);
  });

  GameOver::on_render(ecs, console);
}
