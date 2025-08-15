#include "consumable.hpp"

#include <libtcod.hpp>

#include "actor.hpp"
#include "color.hpp"

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
