#include "consumable.hpp"

#include <cassert>

#include <libtcod.hpp>
#include <libtcod/console_printing.hpp>
#include <memory>

#include "action.hpp"
#include "actor.hpp"
#include "ai.hpp"
#include "color.hpp"
#include "engine.hpp"
#include "game_map.hpp"

ActionResult HealingConsumable::activate(flecs::entity item,
                                         flecs::entity target) const {
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
                                                 flecs::entity consumer) const {
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

ActionResult ConfusionConsumable::activate(flecs::entity item) const {
  auto &eventHandler = item.world().get_mut<Engine>().eventHandler;
  eventHandler.makeTargetSelector(
      [item](auto xy) {
        return std::make_unique<TargetedItemAction>(item, xy);
      },
      item.world());

  return {ActionResultType::Failure, "Select a target location.",
          color::needsTarget};
}

ActionResult ConfusionConsumable::selected(flecs::entity item,
                                           flecs::entity consumer,
                                           std::array<int, 2> target) const {
  auto ecs = item.world();
  auto map = ecs.target<CurrentMap>();
  auto &gameMap = map.get<GameMap>();
  if (!gameMap.isInFov(target[0], target[1])) {
    return {ActionResultType::Failure,
            "You cannot target an area that you cannot see.",
            color::impossible};
  }

  auto target_entity = ecs.query_builder<Position>()
                           .with(flecs::ChildOf, map)
                           .with<Ai>()
                           .build()
                           .find([target](auto &pos) { return pos == target; });
  if (target_entity == target_entity.null()) {
    return {ActionResultType::Failure, "You must select an enemy to target.",
            color::impossible};
  }
  if (target_entity == consumer) {
    return {ActionResultType::Failure, "You cannot confuse yourself!",
            color::impossible};
  }

  auto msg = tcod::stringf(
      "The eyes of the %s look vacant, as it starts to stumble around!",
      target_entity.get<Named>().name.c_str());

  auto ai = ecs.lookup("module::Ai");
  auto q = ecs.query_builder().with(flecs::IsA, ai).build();
  q.each([target_entity](auto ai) {
    if (target_entity.has(ai) && target_entity.enabled(ai)) {
      target_entity.disable(ai);
    }
  });
  if (target_entity.has<ConfusedAi>()) {
    target_entity.get_mut<ConfusedAi>().turns_remaining += number_of_turns;
  } else {
    target_entity.emplace<ConfusedAi>(number_of_turns);
  }

  item.destruct();
  return {ActionResultType::Success, msg, color::statusEffectApplied};
}
