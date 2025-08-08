#pragma once

#include <SDL3/SDL.h>

#include <memory>

#include "action.hpp"

std::unique_ptr<Action> MainGameDispatch(SDL_Event *event);
std::unique_ptr<Action> GameOverDispatch(SDL_Event *event);

struct EventHandler {
  EventHandler() : dispatch(MainGameDispatch){};
  std::unique_ptr<Action> (*dispatch)(SDL_Event *event);
};
