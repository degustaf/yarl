#include "action.hpp"

#include <cassert>
#include <libtcod.hpp>

#include "actor.hpp"
#include "color.hpp"
#include "consumable.hpp"
#include "game_map.hpp"
#include "inventory.hpp"

ActionResult MoveAction::perform(flecs::entity e) const {
  auto &pos = e.get_mut<Position>();
  auto mapEntity = e.world().lookup("currentMap").target<CurrentMap>();
  auto &map = mapEntity.get<GameMap>();
  if (map.inBounds(pos + dxy) && map.isWalkable(pos + dxy)) {
    if (GameMap::get_blocking_entity(mapEntity, pos + dxy) == e.null()) {
      pos.move(dxy);
      return {ActionResultType::Success, ""};
    }
  }
  return {ActionResultType::Failure, "That way is blocked.", color::impossible};
}

ActionResult MeleeAction::perform(flecs::entity e) const {
  auto &pos = e.get_mut<Position>();
  auto ecs = e.world();
  auto mapEntity = ecs.lookup("currentMap").target<CurrentMap>();
  auto target = GameMap::get_blocking_entity(mapEntity, pos + dxy);

  auto attack_color =
      (e == ecs.lookup("player")) ? color::playerAtk : color::enemyAtk;

  if (target != target.null()) {
    const auto &attacker = e.get<Fighter>();
    auto &defender = target.get_mut<Fighter>();
    auto damage = attacker.power - defender.defense;
    auto msg = [&]() {
      if (damage > 0) {
        return tcod::stringf("%s attacks %s for %d hit points",
                             e.get<Named>().name.c_str(),
                             target.get<Named>().name.c_str(), damage);
      } else {
        return tcod::stringf("%s attacks %s but does no damage",
                             e.get<Named>().name.c_str(),
                             target.get<Named>().name.c_str());
      }
    }();
    defender.set_hp(defender.hp() - damage, target);
    return {ActionResultType::Success, msg, attack_color};
  } else {
    return {ActionResultType::Failure, "Nothing to attack.", color::impossible};
  }
}

ActionResult BumpAction::perform(flecs::entity e) const {
  auto &pos = e.get_mut<Position>();
  auto mapEntity = e.world().lookup("currentMap").target<CurrentMap>();
  if (GameMap::get_blocking_entity(mapEntity, pos + dxy)) {
    return MeleeAction(dxy[0], dxy[1]).perform(e);
  }
  return MoveAction(dxy[0], dxy[1]).perform(e);
}

ActionResult ItemAction::perform(flecs::entity e) const {
  if (item.has<HealingConsumable>()) {
    return item.get<HealingConsumable>().activate(item, e);
  } else if (item.has<LightningDamageConsumable>()) {
    return item.get<LightningDamageConsumable>().activate(item, e);
  } else if (item.has<ConfusionConsumable>()) {
    return item.get<ConfusionConsumable>().activate(item);
  } else if (item.has<FireballDamageConsumable>()) {
    return item.get<FireballDamageConsumable>().activate(item);
  }
  assert(false);
}

ActionResult PickupAction::perform(flecs::entity e) const {
  auto &inventory = e.get<Inventory>();
  if (!inventory.hasRoom(e)) {
    return {ActionResultType::Failure, "Your inventory is full.",
            color::impossible};
  }

  auto &pos = e.get<Position>();
  auto q = e.world()
               .query_builder<const Position>("module::pickup")
               .with<Item>()
               .build();
  auto item = q.find(
      [&](flecs::entity item, auto &p) { return e != item && p == pos; });
  if (item) {
    item.add<ContainedBy>(e).remove<Position>();
    auto msg =
        tcod::stringf("You picked up the %s!", item.get<Named>().name.c_str());
    return {ActionResultType::Success, msg};
  }
  return {ActionResultType::Failure, "There is nothing here to pick up.",
          color::impossible};
}

ActionResult DropItemAction::perform(flecs::entity e) const {
  auto msg =
      tcod::stringf("You dropped the %s.", item.get<Named>().name.c_str());
  item.remove<ContainedBy>(e)
      .add(flecs::ChildOf, e.world().lookup("currentMap").target<CurrentMap>())
      .set<Position>(e.get<Position>());
  return {ActionResultType::Success, msg};
}

ActionResult TargetedItemAction::perform(flecs::entity e) const {
  if (item.has<ConfusionConsumable>()) {
    return item.get<ConfusionConsumable>().selected(item, e, target);
  } else if (item.has<FireballDamageConsumable>()) {
    return item.get<FireballDamageConsumable>().selected(item, target);
  }
  assert(false);
}

ActionResult MessageAction::perform(flecs::entity) const {
  return {ActionResultType::Success, msg};
}
