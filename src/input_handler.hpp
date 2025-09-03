#pragma once

#include <SDL3/SDL.h>
#include <flecs.h>

#include <array>
#include <functional>
#include <memory>
#include <string>

#include "action.hpp"
#include "actor.hpp"

struct EventHandler {
  EventHandler() { mainMenu(); };
  EventHandler(const EventHandler &) = delete;
  EventHandler &operator=(const EventHandler &) = delete;
  EventHandler(EventHandler &&) = default;
  EventHandler &operator=(EventHandler &&) = default;

  std::unique_ptr<Action> dispatch(SDL_Event *event, flecs::world ecs);

  template <typename F> void makeTargetSelector(F f, flecs::world ecs) {
    keyDown = &EventHandler::SelectKeyDown;
    click = &EventHandler::SelectClick;
    on_render = &EventHandler::SelectOnRender;
    handle_action = &EventHandler::AskUserHandleAction;
    item_selected = nullptr;
    loc_selected = &EventHandler::SingleTargetSelectedLoc;

    mouse_loc = ecs.lookup("player").get<Position>();
    callback = f;
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
  template <typename F, typename G> void makePopup(F f, G g, std::string text) {
    keyDown = &EventHandler::PopupKeyDown;
    click = &EventHandler::MainGameClick;
    on_render = &EventHandler::PopupOnRender;
    handle_action = &EventHandler::MainMenuHandleAction;
    item_selected = nullptr;
    loc_selected = nullptr;

    this->text = text;
    parent = f;
    parentOnRender = g;
  }
  void mainMenu(void);
  void gameOver(void);

  std::unique_ptr<Action> (EventHandler::*keyDown)(SDL_KeyboardEvent *event,
                                                   flecs::world ecs);
  std::unique_ptr<Action> (EventHandler::*click)(SDL_MouseButtonEvent *button,
                                                 flecs::world ecs);
  void (EventHandler::*on_render)(flecs::world ecs, tcod::Console &console);
  ActionResult (EventHandler::*handle_action)(flecs::world ecs,
                                              std::unique_ptr<Action> action);
  std::unique_ptr<Action> (EventHandler::*item_selected)(flecs::entity item);
  std::unique_ptr<Action> (EventHandler::*loc_selected)(std::array<int, 2> loc);

  std::array<int, 2> mouse_loc = {0, 0};
  std::string title = "";
  std::string text = "";
  size_t log_length = 0;
  size_t cursor = 0;
  int radius = 0;
  flecs::query<const Named> q;
  std::function<std::unique_ptr<Action>(std::array<int, 2>)> callback = nullptr;
  std::function<void(EventHandler *)> parent = nullptr;
  std::function<void(EventHandler *, flecs::world, tcod::Console &)>
      parentOnRender = nullptr;

  std::unique_ptr<Action> MainGameKeyDown(SDL_KeyboardEvent *key, flecs::world);
  std::unique_ptr<Action> GameOverKeyDown(SDL_KeyboardEvent *key, flecs::world);
  std::unique_ptr<Action> HistoryKeyDown(SDL_KeyboardEvent *key, flecs::world);
  std::unique_ptr<Action> AskUserKeyDown(SDL_KeyboardEvent *key, flecs::world);
  std::unique_ptr<Action> InventoryKeyDown(SDL_KeyboardEvent *, flecs::world);
  std::unique_ptr<Action> SelectKeyDown(SDL_KeyboardEvent *key, flecs::world);
  std::unique_ptr<Action> MainMenuKeyDown(SDL_KeyboardEvent *key, flecs::world);
  std::unique_ptr<Action> PopupKeyDown(SDL_KeyboardEvent *key, flecs::world);
  std::unique_ptr<Action> LevelUpKeyDown(SDL_KeyboardEvent *key, flecs::world);

  std::unique_ptr<Action> MainGameClick(SDL_MouseButtonEvent *, flecs::world);
  std::unique_ptr<Action> AskUserClick(SDL_MouseButtonEvent *, flecs::world);
  std::unique_ptr<Action> SelectClick(SDL_MouseButtonEvent *, flecs::world);
  std::unique_ptr<Action> LevelUpClick(SDL_MouseButtonEvent *, flecs::world);

  void MainGameOnRender(flecs::world ecs, tcod::Console &console);
  void HistoryOnRender(flecs::world ecs, tcod::Console &console);
  void InventoryOnRender(flecs::world ecs, tcod::Console &console);
  void SelectOnRender(flecs::world ecs, tcod::Console &console);
  void AreaTargetOnRender(flecs::world ecs, tcod::Console &console);
  void MainMenuOnRender(flecs::world ecs, tcod::Console &console);
  void PopupOnRender(flecs::world ecs, tcod::Console &console);
  void LevelUpOnRender(flecs::world ecs, tcod::Console &console);

  ActionResult MainGameHandleAction(flecs::world ecs, std::unique_ptr<Action>);
  ActionResult AskUserHandleAction(flecs::world ecs, std::unique_ptr<Action>);
  ActionResult MainMenuHandleAction(flecs::world ecs, std::unique_ptr<Action>);

  std::unique_ptr<Action> DropItemSelected(flecs::entity item);
  std::unique_ptr<Action> UseItemSelected(flecs::entity item);

  std::unique_ptr<Action> LookSelectedLoc(std::array<int, 2>);
  std::unique_ptr<Action> SingleTargetSelectedLoc(std::array<int, 2>);
};
