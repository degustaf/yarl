#include "action.hpp"

#include <libtcod.hpp>
#include <libtcod/console_types.hpp>

#include "actor.hpp"
#include "color.hpp"
#include "engine.hpp"
#include "game_map.hpp"

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
  auto ecs = e.world();
  auto mapEntity = ecs.target<CurrentMap>();
  auto target = GameMap::get_blocking_entity(mapEntity, pos + dxy);

  auto attack_color =
      (e == ecs.lookup("player")) ? color::playerAtk : color::EnemyAtk;

  if (target != target.null()) {
    const auto &attacker = e.get<Fighter>();
    auto &defender = target.get_mut<Fighter>();
    auto damage = attacker.power - defender.defense;
    if (damage > 0) {
      auto msg = tcod::stringf("%s attacks %s for %d hit points",
                               e.get<Named>().name.c_str(),
                               target.get<Named>().name.c_str(), damage);
      ecs.get_mut<Engine>().messageLog.addMessage(msg, attack_color);
      defender.set_hp(defender.hp() - damage, target);
    } else {
      auto msg = tcod::stringf("%s attacks %s but does no damage",
                               e.get<Named>().name.c_str(),
                               target.get<Named>().name.c_str());
      ecs.get_mut<Engine>().messageLog.addMessage(msg, attack_color);
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
