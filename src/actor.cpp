#include "actor.hpp"

void Renderable::render(tcod::Console &console, const Position &pos) const {
  auto &tile = console.at(pos);
  tile.ch = ch;
  tile.fg = color;
}
