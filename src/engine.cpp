#include "engine.hpp"

#include <libtcod.hpp>

#include "actor.hpp"
#include "game_map.hpp"

void Engine::render(flecs::world ecs) const {
  auto &console = ecs.get_mut<tcod::Console>();

  auto &map = ecs.get_mut<GameMap>();
  map.render(console);

  auto q = ecs.query<const Position, const Renderable>();
  q.each([&](auto p, auto r) {
    if (map.isInFov(p.x, p.y)) {
      auto &tile = console.at(p);
      tile.ch = r.ch;
      tile.fg = r.color;
    }
  });

  ecs.get_mut<tcod::Context>().present(console);
  console.clear();
}
