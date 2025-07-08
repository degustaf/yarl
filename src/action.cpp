#include "action.hpp"
#include <SDL3/SDL_scancode.h>

Action Action::get(const SDL_Event *event) {
  switch (event->type) {
  case SDL_EVENT_QUIT:
    return {ActionType::QUIT, {0, 0}};
    break;

  case SDL_EVENT_KEY_DOWN:
    switch (event->key.scancode) {
    case SDL_SCANCODE_UP:
      return {ActionType::MOVE, {0, -1}};
      break;
    case SDL_SCANCODE_DOWN:
      return {ActionType::MOVE, {0, 1}};
      break;
    case SDL_SCANCODE_LEFT:
      return {ActionType::MOVE, {-1, 0}};
      break;
    case SDL_SCANCODE_RIGHT:
      return {ActionType::MOVE, {1, 0}};
      break;

    case SDL_SCANCODE_ESCAPE:
      return {ActionType::ESCAPE, {0, 0}};

    default:
      break;
    }
    break;

  default:
    break;
  }

  return {ActionType::NONE, {0, 0}};
}
