#include "consumable.hpp"

#include <cassert>
#include <libtcod/mersenne.hpp>
#include <memory>

#include <libtcod.hpp>

#include "action.hpp"
#include "actor.hpp"
#include "ai.hpp"
#include "color.hpp"
#include "defines.hpp"
#include "game_map.hpp"
#include "input_handler.hpp"
#include "message_log.hpp"
#include "scent.hpp"

ActionResult HealingConsumable::activate(flecs::entity item,
                                         flecs::entity target) const {
  auto &fighter = target.get_mut<Fighter>();
  auto amount_recovered = fighter.heal(amount, target);
  if (amount_recovered > 0) {
    auto msg = tcod::stringf("You consume the %s, and recover %d HP!",
                             item.get<Named>().name.c_str(), amount_recovered);
    item.destruct();
    return {ActionResultType::Success, msg, 0.0f, color::healthRecovered};
  } else {
    return {ActionResultType::Failure, "Your health is already full.", 0.0f,
            color::impossible};
  }
}

ActionResult DeodorantConsumable::activate(flecs::entity item,
                                           flecs::entity target) const {
  auto &scent = target.get_mut<Scent>();
  auto amount_reduced = scent.reduce(amount);
  auto msg = tcod::stringf("You apply the %s, and reduce your smell by %d.",
                           item.get<Named>().name.c_str(), (int)amount_reduced);
  item.destruct();
  return {ActionResultType::Success, msg, 0.0f, color::healthRecovered};
}

ActionResult LightningDamageConsumable::activate(flecs::entity item,
                                                 flecs::entity consumer) const {
  auto closestDistanceSq = maximumRange * maximumRange + 1;

  auto ecs = item.world();
  auto map = ecs.lookup("currentMap").target<CurrentMap>();
  auto &gameMap = map.get<GameMap>();
  auto &consumerPos = consumer.get<Position>();
  auto target = consumer.null();
  auto q = ecs.query_builder<Position>("module::fighter")
               .with<Fighter>()
               .with(flecs::ChildOf, map)
               .build();
  q.each([&](auto e, auto &p) {
    if ((e != consumer) && (gameMap.isInFov(p))) {
      auto d2 = consumerPos.distanceSquared(p);
      if (d2 < closestDistanceSq) {
        target = e;
        closestDistanceSq = d2;
      }
    }
  });

  if (target == consumer.null()) {
    return {ActionResultType::Failure, "No enemy is close enough to strike.",
            0.0f, color::impossible};
  }
  auto msg = tcod::stringf(
      "A lighting bolt strikes the %s with a loud thunder, for %d damage!",
      target.get<Named>().name.c_str(), damage);
  target.get_mut<Fighter>().take_damage(damage, target);
  item.destruct();
  return {ActionResultType::Success, msg, 0.0f};
}

ActionResult ConfusionConsumable::activate(flecs::entity item) const {
  auto &eventHandler = item.world().get_mut<EventHandler>();
  eventHandler.makeTargetSelector(
      [item](auto xy) {
        return std::make_unique<TargetedItemAction>(item, xy);
      },
      item.world(), false);

  return {ActionResultType::Failure, "Select a target location.", 0.0f,
          color::needsTarget};
}

ActionResult ConfusionConsumable::selected(flecs::entity item,
                                           flecs::entity consumer,
                                           std::array<int, 2> target) const {
  auto ecs = item.world();
  auto map = ecs.lookup("currentMap").target<CurrentMap>();
  auto &gameMap = map.get<GameMap>();
  if (!gameMap.isInFov(target)) {
    return {ActionResultType::Failure,
            "You cannot target an area that you cannot see.", 0.0f,
            color::impossible};
  }

  auto target_entity = ecs.query_builder<const Position>("module::enemyWithAi")
                           .with(flecs::ChildOf, map)
                           .with<Ai>()
                           .build()
                           .find([target](auto &pos) { return pos == target; });
  if (target_entity == target_entity.null()) {
    return {ActionResultType::Failure, "You must select an enemy to target.",
            0.0f, color::impossible};
  }
  if (target_entity == consumer) {
    return {ActionResultType::Failure, "You cannot confuse yourself!", 0.0f,
            color::impossible};
  }

  auto msg = tcod::stringf(
      "The eyes of the %s look vacant, as it starts to stumble around!",
      target_entity.get<Named>().name.c_str());

  auto ai = ecs.lookup("module::Ai");
  auto q = ecs.query_builder("module::ai").with(flecs::IsA, ai).build();
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
  return {ActionResultType::Success, msg, 0.0f, color::statusEffectApplied};
}

ActionResult FireballDamageConsumable::activate(flecs::entity item) const {
  auto ecs = item.world();
  auto &eventHandler = ecs.get_mut<EventHandler>();
  eventHandler.makeAreaTargetSelector(
      [item](auto xy) {
        return std::make_unique<TargetedItemAction>(item, xy);
      },
      radius, ecs);

  return {ActionResultType::Failure, "Select a target location.", 0.0f,
          color::needsTarget};
}

ActionResult
FireballDamageConsumable::selected(flecs::entity item,
                                   std::array<int, 2> target) const {
  auto ecs = item.world();
  auto map = ecs.lookup("currentMap").target<CurrentMap>();
  auto &gameMap = map.get<GameMap>();
  if (!gameMap.isInFov(target)) {
    return {ActionResultType::Failure,
            "You cannot target an area that you cannot see.", 0.0f,
            color::impossible};
  }

  auto q = ecs.query_builder<const Position, Fighter, const Named>(
                  "module::fighterPosition")
               .with(flecs::ChildOf, map)
               .build();
  auto targets_hit = false;
  auto &messageLog = ecs.lookup("messageLog").get_mut<MessageLog>();
  ecs.defer_begin();
  q.each([&](auto e, const Position &p, Fighter &f, const Named &name) {
    if (p.distanceSquared(target) <= radius * radius) {
      targets_hit = true;
      auto msg = tcod::stringf(
          "The %s is engulfed in a fiery explosion, taking %d damage!",
          name.name.c_str(), damage);
      messageLog.addMessage(msg, color::enemyDie);
      f.take_damage(damage, e);
    }
  });
  ecs.defer_end();

  if (targets_hit) {
    item.destruct();
    return {ActionResultType::Success, "", 0.0f};
  } else {
    return {ActionResultType::Failure, "There are no targets in the radius.",
            0.0f};
  }
}

ActionResult ScentConsumable::activate(flecs::entity item,
                                       flecs::entity consumer) const {
  consumer.set<Scent>(scent);
  auto msg = tcod::stringf("You apply the %s and now smell like %s.",
                           item.get<Named>().name.c_str(),
                           scentName(scent.type).c_str());
  item.destruct();
  return {ActionResultType::Success, msg, 0.0f, color::healthRecovered};
}

ActionResult MagicMappingConsumable::activate(flecs::entity item,
                                              flecs::entity consumer) const {
  consumer.world()
      .lookup("currentMap")
      .target<CurrentMap>()
      .get_mut<GameMap>()
      .reveal();
  item.destruct();

  return {ActionResultType::Success, "", 0.0f};
}

template <typename T>
ActionResult TrackerConsumable<T>::activate(flecs::entity item,
                                            flecs::entity consumer) const {
  consumer.set<TrackerConsumable>(item.get<TrackerConsumable>());
  item.destruct();

  return {ActionResultType::Success, "", 0.0f};
}

template <typename T>
template <typename Console>
void TrackerConsumable<T>::render(Console &console, flecs::entity map) const {
  auto ecs = map.world();
  auto &gMap = map.get<GameMap>();
  auto q = ecs.query_builder<const Position, const Renderable>(
                  std::string("module::Tracker ") + typeid(T).name())
               .with(flecs::ChildOf, map)
               .with<T>()
               .build();
  q.each([&](auto p, auto r) {
    if (!gMap.isInFov(p)) {
      r.render(console, p, true);
      console.at(p).bg = color::neonGreen;
    }
  });
}

ActionResult RopeConsumable::activate(flecs::entity item,
                                      flecs::entity consumer) const {
  auto ecs = item.world();
  auto cm = ecs.lookup("currentMap").target<CurrentMap>();
  auto &map = cm.get<GameMap>();
  auto pos = consumer.get<Position>();
  bool usable = false;
  for (auto &dir : directions) {
    if (map.isChasm(pos + dir)) {
      usable = true;
    }
  }
  if (usable) {
    ecs.get_mut<EventHandler>().jumpConfirm(true, item);
    return {ActionResultType::Failure, "", 0.0f};
  }
  return {ActionResultType::Failure, "There is no chasm to climb down here.",
          0.0f, color::impossible};
}

ActionResult TransporterConsumable::activate(flecs::entity item,
                                             flecs::entity consumer) const {
  auto ecs = consumer.world();
  auto mapEntity = ecs.lookup("currentMap").target<CurrentMap>();
  auto &map = mapEntity.get<GameMap>();
  auto rng = TCODRandom::getInstance();
  while (true) {
    auto x = rng->getInt(0, map.getWidth() - 1);
    auto y = rng->getInt(0, map.getHeight() - 1);
    auto e = map.get_blocking_entity(mapEntity, {x, y});
    if (map.isWalkable(x, y) && e == e.null()) {
      consumer.set<Position>({x, y});
      item.destruct();
      return {ActionResultType::Success,
              "You transport to another location on the floor.", 0.0f};
    }
  }
}
