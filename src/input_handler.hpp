#pragma once

#include <SDL3/SDL.h>
#include <flecs.h>
#include <libtcod.h>

#include <array>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <type_traits>

#include "action.hpp"
#include "actor.hpp"
#include "blood.hpp"
#include "game_map.hpp"
#include "inventory.hpp"
#include "message_log.hpp"

struct InputHandler {
  virtual ~InputHandler() = default;

  std::unique_ptr<Action> dispatch(SDL_Event *event, flecs::world &ecs);

  virtual std::unique_ptr<Action> keyDown(SDL_KeyboardEvent &, flecs::world) {
    return nullptr;
  };
  virtual std::unique_ptr<Action> click(SDL_MouseButtonEvent &, flecs::world) {
    return nullptr;
  };
  virtual ActionResult handle_action(flecs::world, std::unique_ptr<Action>);
  virtual void animate(flecs::world, uint64_t t) { time = t; };
  virtual void on_render(flecs::world, tcod::Console &) = 0;

  std::array<int, 2> mouse_loc = {0, 0};
  uint64_t time;

protected:
  template <typename T> inline void make(flecs::world ecs) {
    ecs.set<std::unique_ptr<InputHandler>>(std::make_unique<T>(*this));
  }

  template <typename T, typename... Args>
  inline void make(flecs::world ecs, Args &&...args) {
    ecs.set<std::unique_ptr<InputHandler>>(std::make_unique<T>(args..., *this));
  }
};

template <typename T, typename... Args>
inline void make(flecs::world ecs, Args &&...args) {
  auto &handler = ecs.get<std::unique_ptr<InputHandler>>();
  ecs.set<std::unique_ptr<InputHandler>>(
      std::make_unique<T>(args..., *handler));
}

struct MainMenuInputHandler : InputHandler {
  MainMenuInputHandler() = default;
  MainMenuInputHandler(const InputHandler &h) : InputHandler(h) {};
  MainMenuInputHandler(const MainMenuInputHandler &) = default;
  MainMenuInputHandler(MainMenuInputHandler &&) = default;
  virtual ~MainMenuInputHandler() = default;

  virtual std::unique_ptr<Action> keyDown(SDL_KeyboardEvent &,
                                          flecs::world) override;
  virtual void on_render(flecs::world, tcod::Console &) override;
};

struct MainHandler : InputHandler {
  MainHandler() = default;
  MainHandler(const InputHandler &h) : InputHandler(h) {};
  virtual ~MainHandler() = default;

  virtual void on_render(flecs::world, tcod::Console &) override;
  virtual ActionResult handle_action(flecs::world,
                                     std::unique_ptr<Action>) override;
};

struct MainGameInputHandler : MainHandler {
  MainGameInputHandler() = default;
  MainGameInputHandler(const InputHandler &h) : MainHandler(h) {};
  virtual ~MainGameInputHandler() = default;

  virtual std::unique_ptr<Action> keyDown(SDL_KeyboardEvent &,
                                          flecs::world) override;
  virtual std::unique_ptr<Action> click(SDL_MouseButtonEvent &,
                                        flecs::world) override;
};

template <typename T, typename F,
          typename = std::enable_if_t<std::is_base_of_v<InputHandler, T>>>
struct PopupInputHandler : InputHandler {
  PopupInputHandler(F childOnRendor, const T &p)
      : InputHandler(p), parent(p), childOnRender(childOnRendor) {};
  virtual ~PopupInputHandler() = default;

  virtual std::unique_ptr<Action> keyDown(SDL_KeyboardEvent &key,
                                          flecs::world ecs) override {
    switch (key.scancode) {
    case SDL_SCANCODE_ESCAPE:
    case SDL_SCANCODE_RETURN:
    case SDL_SCANCODE_RETURN2:
    case SDL_SCANCODE_KP_ENTER:
      make<T>(ecs);
      break;
    default:
      break;
    }
    return nullptr;
  };

  virtual void on_render(flecs::world ecs, tcod::Console &console) override {
    parent.on_render(ecs, console);
    for (auto &tile : console) {
      tile.fg /= 8;
      tile.bg /= 8;
    }

    childOnRender(ecs, console);
  }

  T parent;
  std::function<void(flecs::world, tcod::Console &)> childOnRender;
};

template <typename F>
inline void makePopup(flecs::world ecs, F childOnRendor,
                      const MainMenuInputHandler &parent) {
  ecs.set<std::unique_ptr<InputHandler>>(
      std::make_unique<PopupInputHandler<MainMenuInputHandler, F>>(
          childOnRendor, parent));
}

struct AskUserInputHandler : MainHandler {
  AskUserInputHandler(const InputHandler &handler) : MainHandler(handler) {};
  virtual ~AskUserInputHandler() = default;

  virtual std::unique_ptr<Action> keyDown(SDL_KeyboardEvent &,
                                          flecs::world) override;
  virtual std::unique_ptr<Action> click(SDL_MouseButtonEvent &,
                                        flecs::world) override;
  virtual ActionResult handle_action(flecs::world,
                                     std::unique_ptr<Action>) override;
};

struct InventoryInputHandler : AskUserInputHandler {
  InventoryInputHandler(const std::string &title, flecs::world ecs,
                        const InputHandler &handler)
      : AskUserInputHandler(handler), title(title),
        q(ecs.query_builder<const Named>("module::playerItem")
              .with<ContainedBy>(ecs.lookup("player"))
              .with<Item>()
              .cached()
              .build()) {};
  virtual ~InventoryInputHandler() = default;

  virtual std::unique_ptr<Action> keyDown(SDL_KeyboardEvent &,
                                          flecs::world) override;
  virtual void on_render(flecs::world, tcod::Console &) override;
  virtual std::unique_ptr<Action> item_selected(flecs::entity item) = 0;

  std::string title;
  flecs::query<const Named> q;
};

struct DropItemInputHandler : InventoryInputHandler {
  virtual ~DropItemInputHandler() = default;
  DropItemInputHandler(const std::string &title, flecs::world ecs,
                       const InputHandler &handler)
      : InventoryInputHandler(title, ecs, handler) {};
  virtual std::unique_ptr<Action> item_selected(flecs::entity item) override;
};

struct UseItemInputHandler : InventoryInputHandler {
  UseItemInputHandler(const std::string &title, flecs::world ecs,
                      const InputHandler &handler)
      : InventoryInputHandler(title, ecs, handler) {};
  virtual ~UseItemInputHandler() = default;
  virtual std::unique_ptr<Action> item_selected(flecs::entity item) override;
};

struct LevelupHandler : AskUserInputHandler {
  LevelupHandler(const InputHandler &handler) : AskUserInputHandler(handler) {};
  virtual ~LevelupHandler() = default;
  virtual std::unique_ptr<Action> keyDown(SDL_KeyboardEvent &,
                                          flecs::world) override;
  virtual std::unique_ptr<Action> click(SDL_MouseButtonEvent &,
                                        flecs::world) override;
  virtual void on_render(flecs::world, tcod::Console &) override;
};

struct HistoryInputHandler : MainHandler {
  HistoryInputHandler(flecs::world ecs, const InputHandler &handler)
      : MainHandler(handler) {
    log_length = ecs.lookup("messageLog").get<MessageLog>().size();
    cursor = log_length - 1;
  };
  virtual ~HistoryInputHandler() = default;

  virtual std::unique_ptr<Action> keyDown(SDL_KeyboardEvent &,
                                          flecs::world) override;
  virtual void on_render(flecs::world, tcod::Console &) override;

  size_t cursor;
  size_t log_length;
};

struct CharacterScreenInputHandler : AskUserInputHandler {
  CharacterScreenInputHandler(const InputHandler &handler)
      : AskUserInputHandler(handler) {};
  virtual ~CharacterScreenInputHandler() = default;

  virtual void on_render(flecs::world, tcod::Console &) override;
};

template <bool useF> struct SelectInputHandler : AskUserInputHandler {
  SelectInputHandler<useF>(flecs::world ecs, const InputHandler &handler)
      : AskUserInputHandler(handler) {
    mouse_loc = ecs.lookup("player").get<Position>();
  };
  virtual ~SelectInputHandler() = default;

  std::unique_ptr<Action> keyDown(SDL_KeyboardEvent &key, flecs::world ecs) {
    auto dxy = std::array<int, 2>{0, 0};

    switch (key.scancode) {
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
      return loc_selected(ecs, mouse_loc);

    case SDL_SCANCODE_F:
      if (useF) {
        return loc_selected(ecs, mouse_loc);
      }
      // Intentional fallthrough

    default:
      make<MainGameInputHandler>(ecs);
      return nullptr;
    }

    auto modifier = 1;
    if (key.mod & SDL_KMOD_SHIFT) {
      modifier *= 5;
    }
    if (key.mod & SDL_KMOD_CTRL) {
      modifier *= 10;
    }
    if (key.mod & SDL_KMOD_ALT) {
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

  std::unique_ptr<Action> click(SDL_MouseButtonEvent &button,
                                flecs::world ecs) {
    auto currentMap = ecs.lookup("currentMap").target<CurrentMap>();
    auto &map = currentMap.get<GameMap>();
    if (map.inBounds((int)button.x, (int)button.y)) {
      if (button.button == SDL_BUTTON_LEFT) {
        return loc_selected(ecs, {(int)button.x, (int)button.y});
      }
    }
    return AskUserInputHandler::click(button, ecs);
  }

  void on_render(flecs::world ecs, tcod::Console &console) {
    MainHandler::on_render(ecs, console);
    auto &tile = console.at(mouse_loc);
    tile.bg = color::white;
  }

  virtual std::unique_ptr<Action> loc_selected(flecs::world ecs,
                                               std::array<int, 2> loc) = 0;
};

struct LookHandler : SelectInputHandler<false> {
  LookHandler(flecs::world ecs, const InputHandler &handler)
      : SelectInputHandler<false>(ecs, handler) {};

  virtual ~LookHandler() = default;

  virtual std::unique_ptr<Action> loc_selected(flecs::world ecs,
                                               std::array<int, 2> loc) override;
};

template <bool useF> struct TargetSelector : SelectInputHandler<useF> {
  template <typename F>
  TargetSelector(F f, flecs::world ecs, const InputHandler &handler)
      : SelectInputHandler<useF>(ecs, handler), callback(f) {}

  virtual ~TargetSelector() = default;

  std::unique_ptr<Action> loc_selected(flecs::world ecs,
                                       std::array<int, 2> xy) {

    make<MainGameInputHandler>(ecs);
    return callback(xy);
  }

  std::function<std::unique_ptr<Action>(std::array<int, 2>)> callback;
};

struct AreaTargetSelector : TargetSelector<true> {
  template <typename F>
  AreaTargetSelector(F f, int r, flecs::world ecs, const InputHandler &handler)
      : TargetSelector<true>(f, ecs, handler), radius(r) {}

  virtual ~AreaTargetSelector() = default;

  virtual void on_render(flecs::world, tcod::Console &) override;

  int radius;
};

struct PathFinder : MainHandler {
  PathFinder(flecs::entity map, std::array<int, 2> orig,
             std::array<int, 2> dest, const InputHandler &handler)
      : MainHandler(handler) {
    auto &gameMap = map.get<GameMap>();
    pathCallback = std::make_unique<PathCallback>(map);
    path = std::make_unique<TCODPath>(gameMap.getWidth(), gameMap.getHeight(),
                                      pathCallback.get(), nullptr);
    path->compute(orig[0], orig[1], dest[0], dest[1]);
  }

  virtual ~PathFinder() = default;

  virtual void on_render(flecs::world, tcod::Console &) override;

  std::unique_ptr<PathCallback> pathCallback = nullptr;
  std::unique_ptr<TCODPath> path = nullptr;
};

template <bool useRope> struct JumpConfirm : AskUserInputHandler {
  JumpConfirm(flecs::entity item, const InputHandler &handler)
      : AskUserInputHandler(handler), item(item) {};
  virtual ~JumpConfirm() = default;

  virtual std::unique_ptr<Action> keyDown(SDL_KeyboardEvent &key,
                                          flecs::world ecs) override {
    switch (key.scancode) {
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
    return AskUserInputHandler::keyDown(key, ecs);
  }

  virtual void on_render(flecs::world ecs, tcod::Console &console) override {
    MainHandler::on_render(ecs, console);
    for (auto &tile : console) {
      tile.fg /= 8;
      tile.bg /= 8;
    }

    if (useRope) {
      tcod::print(console, {console.get_width() / 2, console.get_height() / 2},
                  "Are you sure you want to climb into the chasm?",
                  color::white, color::black, TCOD_CENTER);
      tcod::print(
          console, {console.get_width() / 2, console.get_height() / 2 + 2},
          "(Y)es     (N)o     (J)ump", color::red, color::black, TCOD_CENTER);
    } else {
      tcod::print(console, {console.get_width() / 2, console.get_height() / 2},
                  "Are you sure you want to jump into the chasm?", color::white,
                  color::black, TCOD_CENTER);
      tcod::print(console,
                  {console.get_width() / 2, console.get_height() / 2 + 2},
                  "(Y)es     (N)o", color::red, color::black, TCOD_CENTER);
    }
  }

  flecs::entity item;
};

struct GameOver : MainHandler {
  GameOver(const InputHandler &handler) : MainHandler(handler) {};
  virtual ~GameOver() = default;

  virtual std::unique_ptr<Action> keyDown(SDL_KeyboardEvent &,
                                          flecs::world) override;
};

struct WinScreen : GameOver {
  WinScreen(const InputHandler &handler)
      : GameOver(handler), start_time(handler.time) {};
  virtual ~WinScreen() = default;

  virtual void animate(flecs::world, uint64_t t) override;
  virtual void on_render(flecs::world, tcod::Console &) override;

  uint64_t start_time;
};
