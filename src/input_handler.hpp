#pragma once

#include <SDL3/SDL.h>

#include <array>
#include <memory>

#include "action.hpp"

struct EventHandler {
  EventHandler() : keyDown(MainGameKeyDown), on_render(MainGameOnRender){};
  std::unique_ptr<Action> dispatch(SDL_Event *event);

  std::unique_ptr<Action> (*keyDown)(SDL_KeyboardEvent *event);
  void (*on_render)(flecs::world ecs);
  std::array<int, 2> mouse_loc = {0, 0};

  static std::unique_ptr<Action> MainGameKeyDown(SDL_KeyboardEvent *key);
  static std::unique_ptr<Action> GameOverKeyDown(SDL_KeyboardEvent *key);
  static void MainGameOnRender(flecs::world ecs);
};
