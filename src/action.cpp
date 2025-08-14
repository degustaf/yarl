#include "action.hpp"

#include <cassert>
#include <libtcod.hpp>

#include "actor.hpp"
#include "color.hpp"
#include "consumable.hpp"
#include "game_map.hpp"

ActionResult MoveAction::perform(flecs::entity e) const {
  auto &pos = e.get_mut<Position>();
  auto mapEntity = e.world().target<CurrentMap>();
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
  auto mapEntity = ecs.target<CurrentMap>();
  auto target = GameMap::get_blocking_entity(mapEntity, pos + dxy);

  auto attack_color =
      (e == ecs.lookup("player")) ? color::playerAtk : color::EnemyAtk;

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
  auto mapEntity = e.world().target<CurrentMap>();
  if (GameMap::get_blocking_entity(mapEntity, pos + dxy)) {
    return MeleeAction(dxy[0], dxy[1]).perform(e);
  }
  return MoveAction(dxy[0], dxy[1]).perform(e);
}

ActionResult ItemAction::perform(flecs::entity e) const {
  if (item.has<HealingConsumable>()) {
    return item.get_mut<HealingConsumable>().activate(item, e);
  }
  assert(false);
}
