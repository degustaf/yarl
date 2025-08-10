#pragma once

#include <SDL3/SDL.h>

#include <array>
#include <memory>

#include "action.hpp"

struct Engine;

struct EventHandler {
  EventHandler()
      : keyDown(&EventHandler::MainGameKeyDown),
        on_render(&EventHandler::MainGameOnRender){};
  std::unique_ptr<Action> dispatch(SDL_Event *event, Engine &engine);

  std::unique_ptr<Action> (EventHandler::*keyDown)(SDL_KeyboardEvent *event,
                                                   Engine &engine);
  void (EventHandler::*on_render)(flecs::world ecs);

  std::array<int, 2> mouse_loc = {0, 0};
  size_t log_length = 0;
  size_t cursor = 0;

  std::unique_ptr<Action> MainGameKeyDown(SDL_KeyboardEvent *key,
                                          Engine &engine);
  std::unique_ptr<Action> GameOverKeyDown(SDL_KeyboardEvent *key,
                                          Engine &engine);
  std::unique_ptr<Action> HistoryKeyDown(SDL_KeyboardEvent *key,
                                         Engine &engine);
  void MainGameOnRender(flecs::world ecs);
  void HistoryOnRender(flecs::world ecs);
};
