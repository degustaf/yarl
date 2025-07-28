#include "engine.hpp"

#include <libtcod.hpp>

#include <iostream>

#include "actor.hpp"
#include "game_map.hpp"

void Engine::render(flecs::world ecs) const {
  auto &console = ecs.get_mut<tcod::Console>();

  auto map = ecs.target<CurrentMap>();
  auto &gMap = map.get_mut<GameMap>();
  gMap.render(console);

  auto q = ecs.query_builder<const Position, const Renderable>().with(
      flecs::ChildOf, map);

  q.each([&](auto p, auto r) {
    if (gMap.isInFov(p.x, p.y)) {
      r.render(console, p);
    }
  });

  auto player = ecs.lookup("player");
  player.get<Renderable>().render(console, player.get<Position>());

  ecs.get_mut<tcod::Context>().present(console);
  console.clear();
}

void Engine::handle_enemy_turns(flecs::world ecs) const {
  auto map = ecs.target<CurrentMap>();

  auto q = ecs.query_builder<const Named>().with(flecs::ChildOf, map);

  q.each([](auto name) {
    std::cout << "The " << name.name
              << " wonders when it will get to take a real turn.\n";
  });
}
