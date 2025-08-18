#include "module.hpp"

#include <libtcod.hpp>

#include "actor.hpp"
#include "ai.hpp"
#include "consumable.hpp"
#include "engine.hpp"
#include "game_map.hpp"
#include "inventory.hpp"

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

  // consumable.hpp
  ecs.component<HealingConsumable>();
  ecs.component<LightningDamageConsumable>();

  // engine.hpp
  ecs.component<Engine>();

  // game_map.hpp
  ecs.component<CurrentMap>().add(flecs::Exclusive);
  ecs.component<GameMap>();

  // inventory.hpp
  ecs.component<Inventory>();
  ecs.component<ContainedBy>().add(flecs::Exclusive);
  ecs.component<Item>();

  ecs.prefab("orc")
      .set<Renderable>({'o', {63, 127, 63}, RenderOrder::Actor})
      .set<Named>({"Orc"})
      .add<BlocksMovement>()
      .set<HostileAi>({})
      .emplace<Fighter>(10, 0, 3);

  ecs.prefab("troll")
      .set<Renderable>({'T', {0, 127, 0}, RenderOrder::Actor})
      .set<Named>({"Troll"})
      .add<BlocksMovement>()
      .set<HostileAi>({})
      .emplace<Fighter>(16, 1, 4);

  ecs.prefab("healthPotion")
      .set<Renderable>({'!', {127, 0, 255}, RenderOrder::Item})
      .set<Named>({"Health Potion"})
      .add<Item>()
      .set<HealingConsumable>({4});

  ecs.prefab("lightningScroll")
      .set<Renderable>({'~', {255, 255, 0}, RenderOrder::Item})
      .set<Named>({"Lightning Scroll"})
      .add<Item>()
      .set<LightningDamageConsumable>({20, 5});
}
