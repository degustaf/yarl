#include "consumable.hpp"

#include <libtcod.hpp>
#include <libtcod/console_printing.hpp>

#include "actor.hpp"
#include "color.hpp"
#include "game_map.hpp"

ActionResult HealingConsumable::activate(flecs::entity item,
                                         flecs::entity target) {
  auto &fighter = target.get_mut<Fighter>();
  auto amount_recovered = fighter.heal(amount, target);
  if (amount_recovered > 0) {
    auto msg = tcod::stringf("You consume the %s, and recover %d HP!",
                             item.get<Named>().name.c_str(), amount_recovered);
    item.destruct();
    return {ActionResultType::Success, msg, color::healthRecovered};
  } else {
    return {ActionResultType::Failure, "Your health is already full.",
            color::impossible};
  }
}

ActionResult LightningDamageConsumable::activate(flecs::entity item,
                                                 flecs::entity consumer) {
  auto closestDistanceSq = maximumRange * maximumRange + 1;

  auto ecs = item.world();
  auto map = ecs.target<CurrentMap>();
  auto &gameMap = map.get<GameMap>();
  auto &consumerPos = consumer.get<Position>();
  auto target = consumer.null();
  auto q = ecs.query_builder<Position>()
               .with<Fighter>()
               .with(flecs::ChildOf, map)
               .build();
  q.each([&](auto e, auto &p) {
    if ((e != consumer) && (gameMap.isInFov(p.x, p.y))) {
      auto d2 = consumerPos.distanceSquared(p);
      if (d2 < closestDistanceSq) {
        target = e;
        closestDistanceSq = d2;
      }
    }
  });

  if (target == consumer.null()) {
    return {ActionResultType::Failure, "No enemy is close enough to strike.",
            color::impossible};
  }
  auto msg = tcod::stringf(
      "A lighting bolt strikes the %s with a loud thunder, for %d damage!",
      target.get<Named>().name.c_str(), damage);
  target.get_mut<Fighter>().take_damage(damage, target);
  item.destruct();
  return {ActionResultType::Success, msg};
}
