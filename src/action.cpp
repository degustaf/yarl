#include "action.hpp"

#include <iostream>

SDL_AppResult MoveAction::perform(flecs::entity e) const {
  auto &pos = e.get_mut<Position>();
  auto mapEntity = e.world().target<CurrentMap>();
  auto &map = mapEntity.get<GameMap>();
  if (map.inBounds(pos + dxy) && map.isWalkable(pos + dxy)) {
    if (GameMap::get_blocking_entity(mapEntity, pos + dxy) == e.null()) {
      pos.move(dxy);
    }
  }
  return SDL_APP_CONTINUE;
}

SDL_AppResult MeleeAction::perform(flecs::entity e) const {
  auto &pos = e.get_mut<Position>();
  auto mapEntity = e.world().target<CurrentMap>();
  auto target = GameMap::get_blocking_entity(mapEntity, pos + dxy);

  if (target != target.null()) {
    const auto &attacker = e.get<Fighter>();
    auto &defender = target.get_mut<Fighter>();
    auto damage = attacker.power - defender.defense;
    std::cout << e.get<Named>().name << " attacks " << target.get<Named>().name;
    if (damage > 0) {
      std::cout << " for " << damage << " hit points\n";
      defender.set_hp(defender.hp() - damage, target);
    } else {
      std::cout << " but does no damge\n";
    }
  }

  return SDL_APP_CONTINUE;
}

SDL_AppResult BumpAction::perform(flecs::entity e) const {
  auto &pos = e.get_mut<Position>();
  auto mapEntity = e.world().target<CurrentMap>();
  if (GameMap::get_blocking_entity(mapEntity, pos + dxy)) {
    return MeleeAction(dxy[0], dxy[1]).perform(e);
  }
  return MoveAction(dxy[0], dxy[1]).perform(e);
}
