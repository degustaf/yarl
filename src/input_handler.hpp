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
  EventHandler()
      : keyDown(&EventHandler::MainGameKeyDown),
        click(&EventHandler::MainGameClick),
        on_render(&EventHandler::MainGameOnRender),
        handle_action(&EventHandler::MainGameHandleAction),
        item_selected(nullptr){};
  std::unique_ptr<Action> dispatch(SDL_Event *event, flecs::world ecs);

  template <typename F> void makeTargetSelector(F f, flecs::world ecs) {
    keyDown = &EventHandler::SelectKeyDown;
    click = &EventHandler::SelectClick;
    on_render = &EventHandler::SelectOnRender;
    handle_action = &EventHandler::AskUserHandleAction;
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
    loc_selected = &EventHandler::SingleTargetSelectedLoc;

    mouse_loc = ecs.lookup("player").get<Position>();
    callback = f;
    radius = r;
  }

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
  size_t log_length = 0;
  size_t cursor = 0;
  int radius = 0;
  flecs::query<const Named> q;
  std::function<std::unique_ptr<Action>(std::array<int, 2>)> callback;

  std::unique_ptr<Action> MainGameKeyDown(SDL_KeyboardEvent *key,
                                          flecs::world ecs);
  std::unique_ptr<Action> GameOverKeyDown(SDL_KeyboardEvent *key, flecs::world);
  std::unique_ptr<Action> HistoryKeyDown(SDL_KeyboardEvent *key, flecs::world);
  std::unique_ptr<Action> AskUserKeyDown(SDL_KeyboardEvent *key, flecs::world);
  std::unique_ptr<Action> InventoryKeyDown(SDL_KeyboardEvent *key,
                                           flecs::world);
  std::unique_ptr<Action> SelectKeyDown(SDL_KeyboardEvent *key, flecs::world);

  std::unique_ptr<Action> MainGameClick(SDL_MouseButtonEvent *, flecs::world);
  std::unique_ptr<Action> AskUserClick(SDL_MouseButtonEvent *, flecs::world);
  std::unique_ptr<Action> SelectClick(SDL_MouseButtonEvent *, flecs::world);

  void MainGameOnRender(flecs::world ecs, tcod::Console &console);
  void HistoryOnRender(flecs::world ecs, tcod::Console &console);
  void InventoryOnRender(flecs::world ecs, tcod::Console &console);
  void SelectOnRender(flecs::world ecs, tcod::Console &console);
  void AreaTargetOnRender(flecs::world ecs, tcod::Console &console);

  ActionResult MainGameHandleAction(flecs::world ecs,
                                    std::unique_ptr<Action> action);
  ActionResult AskUserHandleAction(flecs::world ecs,
                                   std::unique_ptr<Action> action);

  std::unique_ptr<Action> DropItemSelected(flecs::entity item);
  std::unique_ptr<Action> UseItemSelected(flecs::entity item);

  std::unique_ptr<Action> LookSelectedLoc(std::array<int, 2>);
  std::unique_ptr<Action> SingleTargetSelectedLoc(std::array<int, 2>);
};
