#include "engine.hpp"

#include <libtcod.hpp>

#include "actor.hpp"
#include "game_map.hpp"

void Engine::render(flecs::world ecs) const {
  auto &console = ecs.get_mut<tcod::Console>();

  ecs.get_mut<GameMap>().render(console);

  auto q = ecs.query<const Position, const Renderable>();
  q.each([&](auto p, auto r) { console.at(p) = {r.ch, r.color}; });

  ecs.get_mut<tcod::Context>().present(console);
  console.clear();
}
