#include "module.hpp"

#include <libtcod.hpp>

#include "actor.hpp"
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

  // engine.hpp
  ecs.component<Engine>();

  // game_map.hpp
  ecs.component<CurrentMap>().add(flecs::Exclusive);
  ecs.component<GameMap>();

  ecs.prefab("orc")
      .set<Renderable>({'o', {63, 127, 63}})
      .set<Named>({"Orc"})
      .add<BlocksMovement>();

  ecs.prefab("troll")
      .set<Renderable>({'T', {0, 127, 0}})
      .set<Named>({"Troll"})
      .add<BlocksMovement>();
}
