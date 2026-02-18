#include "consumable.hpp"

#include <cassert>
#include <memory>
#include <optional>

#include "action.hpp"
#include "actor.hpp"
#include "ai.hpp"
#include "color.hpp"
#include "defines.hpp"
#include "game_map.hpp"
#include "input_handler.hpp"
#include "inventory.hpp"
#include "message_log.hpp"
#include "position.hpp"
#include "scent.hpp"
#include "string.hpp"

ActionResult HealingConsumable::activate(flecs::entity item,
                                         flecs::entity target) const {
  auto &fighter = target.get_mut<Fighter>();
  auto amount_recovered = fighter.heal(amount, target);
  if (amount_recovered > 0) {
    auto msg = stringf("You consume the %s, and recover %d HP!",
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
  auto msg = stringf("You apply the %s, and reduce your smell by %d.",
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
  auto q = ecs.query_builder<const Position, const Fighter>("module::fighter")
               .with(flecs::ChildOf, map)
               .build();
  q.each([&](auto e, auto &p, auto &f) {
    if ((e != consumer) && (f.hp() > 0) && (gameMap.isVisible(p))) {
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

  ecs.get_mut<Trauma>().trauma += 0.25f;
  auto &tp = target.get<Position>();
  ecs.entity()
      .set<Position>(tp)
      .set<MoveAnimation>({(float)consumerPos.x, (float)consumerPos.y, 0.05f})
      .set<Renderable>(
          {0x3df, color::lightning, std::nullopt, RenderOrder::Item})
      .add<DisappearOnHit>()
      .add(flecs::ChildOf, map);

  auto msg = stringf(
      "A lighting bolt strikes the %s with a loud thunder, for %d damage!",
      target.get<Named>().name.c_str(), damage);
  target.get_mut<Fighter>().take_damage(damage, target);
  item.destruct();
  return {ActionResultType::Success, msg, 0.0f};
}

ActionResult ConfusionConsumable::activate(flecs::entity item,
                                           flecs::entity) const {
  auto ecs = item.world();
  make<TargetSelector<false>>(ecs, [item](auto xy) {
    return std::make_unique<TargetedItemAction>(item, xy);
  });

  return {ActionResultType::Failure, "Select a target location.", 0.0f,
          color::needsTarget};
}

ActionResult ConfusionConsumable::selected(flecs::entity item,
                                           flecs::entity consumer,
                                           std::array<int, 2> target) const {
  auto ecs = item.world();
  auto map = ecs.lookup("currentMap").target<CurrentMap>();
  auto &gameMap = map.get<GameMap>();
  if (!gameMap.isVisible(target)) {
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

  auto msg =
      stringf("The eyes of the %s look vacant, as it starts to stumble around!",
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

ActionResult FireballDamageConsumable::activate(flecs::entity item,
                                                flecs::entity) const {
  auto ecs = item.world();
  make<AreaTargetSelector>(
      ecs,
      [item](auto xy) {
        return std::make_unique<TargetedItemAction>(item, xy);
      },
      radius);

  return {ActionResultType::Failure, "Select a target location.", 0.0f,
          color::needsTarget};
}

ActionResult
FireballDamageConsumable::selected(flecs::entity item,
                                   std::array<int, 2> target) const {
  auto ecs = item.world();
  auto map = ecs.lookup("currentMap").target<CurrentMap>();
  auto &gameMap = map.get<GameMap>();
  if (!gameMap.isVisible(target)) {
    return {ActionResultType::Failure,
            "You cannot target an area that you cannot see.", 0.0f,
            color::impossible};
  }

  auto q = ecs.query_builder<const Position, Fighter, const Named>(
                  "module::fighterPosition")
               .with(flecs::ChildOf, map)
               .build();
  auto flammableQ = ecs.query_builder<const Position, const Named>(
                           "module::flammablePosition")
                        .with<Flammable>()
                        .with(flecs::ChildOf, map)
                        .build();
  auto targets_hit = false;
  auto &messageLog = ecs.lookup("messageLog").get_mut<MessageLog>();
  ecs.defer_begin();
  q.each([&](auto e, const Position &p, Fighter &f, const Named &name) {
    if (p.distanceSquared(target) <= radius * radius) {
      targets_hit = true;
      auto msg =
          stringf("The %s is engulfed in a fiery explosion, taking %d damage!",
                  name.name.c_str(), damage);
      messageLog.addMessage(msg, color::enemyDie);
      f.take_damage(damage, e);
    }
  });
  flammableQ.each([&](auto e, const Position &p, const Named &n) {
    if (p.distanceSquared(target) <= radius * radius) {
      targets_hit = true;
      auto msg = stringf("The %s burns up.", n.name.c_str());
      e.destruct();
    }
  });
  // TODO destroy flammable items in inventories.
  ecs.defer_end();
  item.destruct();

  auto rng = TCODRandom::getInstance();
  for (auto i = 0; i < 1000; i++) {
    auto dx = rng->get(-1.0f, 1.0f);
    auto dy = rng->get(-1.0f, 1.0f);
    auto r2 = dx * dx + dy * dy;
    if (r2 > 1.0f)
      continue;

    auto scale = rng->get(0.5f, 2.0f);
    auto center = FPosition{(float)target[0], (float)target[1]};
    ecs.entity()
        .set<FPosition>(center)
        .set<Velocity>({2 * dx, 2 * dy})
        .set<RadialLimit>({center, (float)radius})
        .set<Renderable>({0x2022,
                          color::lerp(color::fireball, color::lightning, r2),
                          std::nullopt, RenderOrder::Actor, scale})
        .set<Fade>({0.25f, 0.3f})
        .add(flecs::ChildOf, map);
  }

  ecs.get_mut<Trauma>().trauma += 0.25f;

  if (targets_hit) {
    return {ActionResultType::Success, "", 0.0f};
  } else {
    return {ActionResultType::Failure, "There are no targets in the radius.",
            0.0f};
  }
}

ActionResult ScentConsumable::activate(flecs::entity item,
                                       flecs::entity consumer) const {
  consumer.set<Scent>(scent);
  auto msg =
      stringf("You apply the %s and now smell like %s.",
              item.get<Named>().name.c_str(), scentName(scent.type).c_str());
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
void TrackerConsumable<T>::render(Console &console, flecs::entity map) const {
  auto ecs = map.world();
  auto &gMap = map.get<GameMap>();
  auto q =
      ecs.query_builder<const Position, const Renderable, const Invisible *>(
             std::string("module::Tracker ") + typeid(T).name())
          .with(flecs::ChildOf, map)
          .with<T>()
          .build();
  q.each([&](auto &p, auto &r, auto i) {
    if (!gMap.isVisible(p) || (i && !i->paused)) {
      r.render(console, p, true);
      console.at(p).bg = color::sensed;
    }
  });
}

ActionResult RopeConsumable::activate(flecs::entity item,
                                      flecs::entity consumer) const {
  auto ecs = item.world();
  auto currentMap = ecs.lookup("currentMap").target<CurrentMap>();
  auto &map = currentMap.get<GameMap>();
  auto pos = consumer.get<Position>();
  bool usable = false;
  for (auto &dir : directions) {
    if (map.isChasm(pos + dir)) {
      usable = true;
    }
  }
  if (usable) {
    make<JumpConfirm<true>>(ecs, item);
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
