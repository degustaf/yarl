#include "module.hpp"

#include <libtcod.hpp>

#include "actor.hpp"
#include "ai.hpp"
#include "engine.hpp"
#include "game_map.hpp"

module::module(flecs::world ecs) {
  ecs.module<module>("module");

  // TCOD
  ecs.component<tcod::Context>();
  ecs.component<tcod::Console>();

  // actor.hpp
  ecs.component<Position>();
  ecs.component<Renderable>();
  ecs.component<Named>();
  ecs.component<BlocksMovement>();
  ecs.component<Fighter>();

  // ai.hpp
  ecs.component<Ai>();
  ecs.component<HostileAi>().is_a<Ai>();

  // engine.hpp
  ecs.component<Engine>();

  // game_map.hpp
  ecs.component<CurrentMap>().add(flecs::Exclusive);
  ecs.component<GameMap>();

  ecs.prefab("orc")
      .set<Renderable>({'o', {63, 127, 63}, Actor})
      .set<Named>({"Orc"})
      .add<BlocksMovement>()
      .set<HostileAi>({})
      .emplace<Fighter>(10, 0, 3);

  ecs.prefab("troll")
      .set<Renderable>({'T', {0, 127, 0}, Actor})
      .set<Named>({"Troll"})
      .add<BlocksMovement>()
      .set<HostileAi>({})
      .emplace<Fighter>(16, 1, 4);
}
