#pragma once

#include <SDL3/SDL.h>
#include <flecs.h>

#include <array>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>

#include "action.hpp"
#include "actor.hpp"
#include "blood.hpp"

struct EventHandler {
  EventHandler() { mainMenu(); };
  EventHandler(const EventHandler &) = delete;
  EventHandler &operator=(const EventHandler &) = delete;
  EventHandler(EventHandler &&) = default;
  EventHandler &operator=(EventHandler &&) = default;

  std::unique_ptr<Action> dispatch(SDL_Event *event, flecs::world &ecs);

  template <typename F>
  void makeTargetSelector(F f, flecs::world ecs, bool useF) {
    keyDown = &EventHandler::SelectKeyDown;
    click = &EventHandler::SelectClick;
    on_render = &EventHandler::SelectOnRender;
    handle_action = &EventHandler::AskUserHandleAction;
    item_selected = nullptr;
    loc_selected = &EventHandler::SingleTargetSelectedLoc;

    mouse_loc = ecs.lookup("player").get<Position>();
    callback = f;
    this->useF = useF;
  }
  template <typename F>
  void makeAreaTargetSelector(F f, int r, flecs::world ecs) {
    keyDown = &EventHandler::SelectKeyDown;
    click = &EventHandler::SelectClick;
    on_render = &EventHandler::AreaTargetOnRender;
    handle_action = &EventHandler::AskUserHandleAction;
    item_selected = nullptr;
    loc_selected = &EventHandler::SingleTargetSelectedLoc;

    mouse_loc = ecs.lookup("player").get<Position>();
    callback = f;
    radius = r;
  }
  template <typename F, typename G, typename H> void makePopup(F f, G g, H h) {
    keyDown = &EventHandler::PopupKeyDown;
    click = &EventHandler::EmptyClick;
    on_render = &EventHandler::PopupOnRender;
    handle_action = &EventHandler::MainMenuHandleAction;
    item_selected = nullptr;
    loc_selected = nullptr;

    parent = f;
    parentOnRender = g;
    childOnRender = h;
  }
  void restoreMainGame();
  void jumpConfirm(bool useRope, flecs::entity item);
  void mainMenu(void);
  void gameOver(void);
  void winGame(void);
  void commandsMenu(void);

  std::unique_ptr<Action> (EventHandler::*keyDown)(SDL_KeyboardEvent *event,
                                                   flecs::world &ecs);
  std::unique_ptr<Action> (EventHandler::*click)(SDL_MouseButtonEvent *button,
                                                 flecs::world ecs);
  void (EventHandler::*on_render)(flecs::world ecs, tcod::Console &console,
                                  uint64_t time);
  ActionResult (EventHandler::*handle_action)(flecs::world ecs,
                                              std::unique_ptr<Action> action);
  std::unique_ptr<Action> (EventHandler::*item_selected)(flecs::entity item);
  std::unique_ptr<Action> (EventHandler::*loc_selected)(std::array<int, 2> loc);

  std::array<int, 2> mouse_loc = {0, 0};
  std::string title = "";
  size_t log_length = 0;
  size_t cursor = 0;
  int radius = 0;
  bool useRope = false;
  bool useF = false;
  flecs::entity item;
  flecs::query<const Named> q;
  std::vector<BloodDrop> drops = std::vector<BloodDrop>();
  std::function<std::unique_ptr<Action>(std::array<int, 2>)> callback = nullptr;
  std::function<void(EventHandler *)> parent = nullptr;
  std::function<void(EventHandler *, flecs::world, tcod::Console &, uint64_t)>
      parentOnRender = nullptr;
  std::function<void(flecs::world, tcod::Console &)> childOnRender = nullptr;

  std::unique_ptr<Action> MainGameKeyDown(SDL_KeyboardEvent *key,
                                          flecs::world &);
  std::unique_ptr<Action> GameOverKeyDown(SDL_KeyboardEvent *key,
                                          flecs::world &);
  std::unique_ptr<Action> HistoryKeyDown(SDL_KeyboardEvent *key,
                                         flecs::world &);
  std::unique_ptr<Action> AskUserKeyDown(SDL_KeyboardEvent *key,
                                         flecs::world &);
  std::unique_ptr<Action> InventoryKeyDown(SDL_KeyboardEvent *, flecs::world &);
  std::unique_ptr<Action> SelectKeyDown(SDL_KeyboardEvent *key, flecs::world &);
  std::unique_ptr<Action> MainMenuKeyDown(SDL_KeyboardEvent *key,
                                          flecs::world &);
  std::unique_ptr<Action> PopupKeyDown(SDL_KeyboardEvent *key, flecs::world &);
  std::unique_ptr<Action> LevelUpKeyDown(SDL_KeyboardEvent *key,
                                         flecs::world &);
  std::unique_ptr<Action> JumpKeyDown(SDL_KeyboardEvent *key, flecs::world &);

  std::unique_ptr<Action> EmptyClick(SDL_MouseButtonEvent *, flecs::world);
  std::unique_ptr<Action> MainGameClick(SDL_MouseButtonEvent *, flecs::world);
  std::unique_ptr<Action> AskUserClick(SDL_MouseButtonEvent *, flecs::world);
  std::unique_ptr<Action> SelectClick(SDL_MouseButtonEvent *, flecs::world);
  std::unique_ptr<Action> LevelUpClick(SDL_MouseButtonEvent *, flecs::world);

  void MainGameOnRender(flecs::world ecs, tcod::Console &console,
                        uint64_t time);
  void HistoryOnRender(flecs::world ecs, tcod::Console &console, uint64_t time);
  void InventoryOnRender(flecs::world ecs, tcod::Console &console,
                         uint64_t time);
  void SelectOnRender(flecs::world ecs, tcod::Console &console, uint64_t time);
  void AreaTargetOnRender(flecs::world ecs, tcod::Console &console,
                          uint64_t time);
  void MainMenuOnRender(flecs::world ecs, tcod::Console &console,
                        uint64_t time);
  void PopupOnRender(flecs::world ecs, tcod::Console &console, uint64_t time);
  void LevelUpOnRender(flecs::world ecs, tcod::Console &console, uint64_t time);
  void CharacterScreenOnRender(flecs::world ecs, tcod::Console &console,
                               uint64_t time);
  void JumpOnRender(flecs::world ecs, tcod::Console &console, uint64_t time);
  void WinOnRender(flecs::world ecs, tcod::Console &console, uint64_t time);

  ActionResult MainGameHandleAction(flecs::world ecs, std::unique_ptr<Action>);
  ActionResult AskUserHandleAction(flecs::world ecs, std::unique_ptr<Action>);
  ActionResult MainMenuHandleAction(flecs::world ecs, std::unique_ptr<Action>);

  std::unique_ptr<Action> DropItemSelected(flecs::entity item);
  std::unique_ptr<Action> UseItemSelected(flecs::entity item);

  std::unique_ptr<Action> LookSelectedLoc(std::array<int, 2>);
  std::unique_ptr<Action> SingleTargetSelectedLoc(std::array<int, 2>);
};
