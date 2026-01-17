#include "action.hpp"

#include <cassert>
#include <memory>

#include "actor.hpp"
#include "color.hpp"
#include "consumable.hpp"
#include "game_map.hpp"
#include "input_handler.hpp"
#include "inventory.hpp"
#include "string.hpp"

static ActionResult attack(flecs::entity e, std::array<int, 2> pos,
                           bool ranged) {
  auto ecs = e.world();
  auto currentMap = ecs.lookup("currentMap");
  assert(currentMap);
  auto mapEntity = currentMap.target<CurrentMap>();
  auto target = GameMap::get_blocking_entity(mapEntity, pos);

  auto attack_color =
      (e == ecs.lookup("player")) ? color::playerAtk : color::enemyAtk;

  if (target != target.null()) {
    auto weapon = e.target<Weapon>();
    if (weapon && weapon.has<Taser>()) {
      auto taser = weapon.get<Taser>();
      taser.apply(target);
      auto msg =
          stringf("%s tases %s for %d turns", e.get<Named>().name.c_str(),
                  target.get<Named>().name.c_str(), taser.turns);
      return {ActionResultType::Success, msg, 1.0f, attack_color};
    }
    const auto &attacker = e.get<Fighter>();
    auto &defender = target.get_mut<Fighter>();
    auto damage =
        std::max(attacker.power(e, ranged) - defender.defense(target), 0);
    auto msg = [&]() {
      if (damage > 0) {
        return stringf("%s attacks %s for %d hit points",
                       e.get<Named>().name.c_str(),
                       target.get<Named>().name.c_str(), damage);
      } else {
        return stringf("%s attacks %s but does no damage",
                       e.get<Named>().name.c_str(),
                       target.get<Named>().name.c_str());
      }
    }();
    defender.take_damage(damage, target);
    return {ActionResultType::Success, msg, 1.0f, attack_color};
  } else {
    return {ActionResultType::Failure, "Nothing to attack.", 0.0f,
            color::impossible};
  }
}

ActionResult MoveAction::perform(flecs::entity e) const {
  auto &pos = e.get_mut<Position>();
  auto mapEntity = e.world().lookup("currentMap").target<CurrentMap>();
  auto &map = mapEntity.get<GameMap>();
  if (map.inBounds(pos + dxy)) {
    if (map.isWalkable(pos + dxy)) {
      if (GameMap::get_blocking_entity(mapEntity, pos + dxy) == e.null()) {
        pos.move(dxy);
        return {ActionResultType::Success, "", 1.0f};
      }
    } else if (map.isTransparent(pos + dxy)) {
      // We've found a chasm.
      auto ecs = e.world();
      make<JumpConfirm<false>>(ecs, e.null());
      return {ActionResultType::Failure, "", 0.0f};
    }
  }
  return {ActionResultType::Failure, "That way is blocked.", 0.0f,
          color::impossible};
}

ActionResult MeleeAction::perform(flecs::entity e) const {
  auto &pos = e.get<Position>();
  return attack(e, pos + dxy, false);
}

ActionResult DoorDirectionAction::perform(flecs::entity e) const {
  auto &pos = e.get<Position>();
  auto ecs = e.world();
  auto currentMap = ecs.lookup("currentMap");
  assert(currentMap);
  auto mapEntity = currentMap.target<CurrentMap>();

  auto q = ecs.query_builder<const Position>("module::blocksPosition")
               .with(flecs::ChildOf, mapEntity)
               .with<Openable>()
               .build();

  auto target = q.find([=](const Position &p) { return p == pos + dxy; });
  if (target) {
    toggleDoor(target);
    return {ActionResultType::Success, "", 0.0f};
  }
  return {ActionResultType::Failure, "There is nothing openable here.", 0.0f,
          color::impossible};
}

ActionResult DoorAction::perform(flecs::entity e) const {
  auto &pos = e.get<Position>();
  auto ecs = e.world();
  auto currentMap = ecs.lookup("currentMap");
  assert(currentMap);
  auto mapEntity = currentMap.target<CurrentMap>();

  auto q = ecs.query_builder<const Position>("module::blocksPosition")
               .with(flecs::ChildOf, mapEntity)
               .with<Openable>()
               .build();

  auto success = false;
  ecs.defer_begin();
  q.each([&](auto e, auto &p) {
    if (pos.distanceSquared(p) <= 2) {
      toggleDoor(e);
      success = true;
    }
  });
  ecs.defer_end();
  if (success) {
    return {ActionResultType::Success, "", 0.0f};
  }
  return {ActionResultType::Failure, "There is nothing openable here.", 0.0f,
          color::impossible};
}

ActionResult BatheAction::perform(flecs::entity e) const {
  assert(fountain.has<Fountain>());
  auto scent = e.try_get_mut<Scent>();
  if (scent) {
    scent->power = 0;
    auto msg = std::string("You bathe and feel refreshingly clean.");
    if (TCODRandom::getInstance()->getInt(1, 3) == 1) {
      fountain.remove<Fountain>();
      fountain.get_mut<Renderable>().fg = color::lightGrey;
      msg += " The fountain dries up.";
    }
    return {ActionResultType::Success, msg, 0.0f};
  }
  return {ActionResultType::Failure, "", 0.0f};
}

ActionResult BumpAction::perform(flecs::entity e) const {
  auto exertion = 0.0f;
  for (auto i = 0; i < speed; i++) {
    auto &pos = e.get_mut<Position>();
    auto mapEntity = e.world().lookup("currentMap").target<CurrentMap>();
    auto target = GameMap::get_blocking_entity(mapEntity, pos + dxy);
    auto result = [&]() {
      if (target) {
        if (target.has<Fighter>()) {
          return MeleeAction(dxy[0], dxy[1]).perform(e);
        }
        if (target.has<Openable>()) {
          return DoorDirectionAction(dxy[0], dxy[1]).perform(e);
        }
        if (target.has<Fountain>()) {
          return BatheAction(target).perform(e);
        }
      }
      return MoveAction(dxy[0], dxy[1]).perform(e);
    }();
    exertion += result.exertion * (float)speed;
    if (!result.msg.empty()) {
      result.exertion = exertion;
      return result;
    }
  }
  return {ActionResultType::Success, "", exertion};
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
  } else if (item.has<DeodorantConsumable>()) {
    return item.get<DeodorantConsumable>().activate(item, e);
  } else if (item.has<ScentConsumable>()) {
    return item.get<ScentConsumable>().activate(item, e);
  } else if (item.has<MagicMappingConsumable>()) {
    return MagicMappingConsumable{}.activate(item, e);
    // } else if (item.has<TrackerConsumable>()) {
    //   return item.get<TrackerConsumable>().activate(item, e);
  } else if (item.has<RopeConsumable>()) {
    return RopeConsumable{}.activate(item, e);
  } else if (item.has<TransporterConsumable>()) {
    return TransporterConsumable{}.activate(item, e);
  }
  assert(false);
  return {ActionResultType::Failure, "", 0.0f};
}

ActionResult PickupAction::perform(flecs::entity e) const {
  auto &inventory = e.get<Inventory>();
  if (!inventory.hasRoom(e)) {
    return {ActionResultType::Failure, "Your inventory is full.", 0.0f,
            color::impossible};
  }

  auto &pos = e.get<Position>();
  auto currentMap = e.world().lookup("currentMap");
  assert(currentMap);
  auto map = currentMap.target<CurrentMap>();
  auto q = e.world()
               .query_builder<const Position>("module::pickup")
               .with<Item>()
               .with(flecs::ChildOf, map)
               .build();
  auto item = q.find(
      [&](flecs::entity item, auto &p) { return e != item && p == pos; });
  if (item) {
    item.add<ContainedBy>(e).remove<Position>().remove(flecs::ChildOf, map);
    auto msg = stringf("You picked up the %s!", item.get<Named>().name.c_str());
    return {ActionResultType::Success, msg, 0.0f};
  }
  return {ActionResultType::Failure, "There is nothing here to pick up.", 0.0f,
          color::impossible};
}

ActionResult DropItemAction::perform(flecs::entity e) const {
  auto msg = isEquipped(e, item) ? toggleEquip<true>(e, item) + " " : "";
  msg = stringf("%sYou dropped the %s.", msg.c_str(),
                item.get<Named>().name.c_str());
  item.remove<ContainedBy>(e)
      .add(flecs::ChildOf, e.world().lookup("currentMap").target<CurrentMap>())
      .set<Position>(e.get<Position>());
  return {ActionResultType::Success, msg, 0.0f};
}

ActionResult TargetedItemAction::perform(flecs::entity e) const {
  if (item.has<ConfusionConsumable>()) {
    return item.get<ConfusionConsumable>().selected(item, e, target);
  } else if (item.has<FireballDamageConsumable>()) {
    return item.get<FireballDamageConsumable>().selected(item, target);
  } else if (item.has<Ranged>()) {
    return attack(e, target, true);
  }
  assert(false);
  return {ActionResultType::Failure, "", 0.0f};
}

ActionResult MessageAction::perform(flecs::entity) const {
  return {ActionResultType::Success, msg, 0.0f, fg};
}

ActionResult TakeStairsAction::perform(flecs::entity e) const {
  auto ecs = e.world();
  assert(e == ecs.lookup("player"));
  auto pos = e.get<Position>();

  auto currentMap = ecs.lookup("currentMap").target<CurrentMap>();
  auto &gameMap = currentMap.get<GameMap>();
  if (gameMap.isStairs(pos)) {
    gameMap.nextFloor(e);

    return {ActionResultType::Success, "You descend the staircase.", 0.0f,
            color::descend};
  }
  return {ActionResultType::Failure, "There is no elevator here.", 0.0f,
          color::impossible};
}

ActionResult JumpAction::perform(flecs::entity e) const {
  auto currentMap = e.world().lookup("currentMap").target<CurrentMap>();
  auto &gameMap = currentMap.get<GameMap>();
  gameMap.nextFloor(e);

  constexpr auto fallDamage = 4;

  if (useRope) {
    return {ActionResultType::Success, "You climb down the chasm.", 1.0f,
            color::descend};
  }
  e.get_mut<Fighter>().take_damage(fallDamage, e);
  auto scent = e.try_get_mut<Scent>();
  if (scent) {
    scent->power /= 2.0f;
  }

  auto msg = stringf("You jump down the chasm taking %d damage.", fallDamage);

  return {ActionResultType::Success, msg, 1.0f, color::descend};
}

ActionResult EquipAction::perform(flecs::entity e) const {
  return {ActionResultType::Success, toggleEquip<true>(e, item), 0.0f};
}

ActionResult RangedTargetAction::perform(flecs::entity e) const {
  auto weapon = e.target<Weapon>();
  if (weapon) {
    auto range = weapon.try_get<Ranged>();
    if (range) {
      auto ecs = e.world();
      make<TargetSelector<true>>(
          ecs,
          [weapon](auto xy) {
            return std::make_unique<TargetedItemAction>(weapon, xy);
          },
          ecs);
      return {ActionResultType::Failure, "", 0.0f};
    }
    auto msg = stringf("Your %s isn't a ranged weapon.",
                       weapon.get<Named>().name.c_str());
    return {ActionResultType::Failure, msg, 0.0f};
  }
  return {ActionResultType::Failure, "You do not have a weapon to fire.", 0.0f};
}
