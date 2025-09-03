#include "level.hpp"

#include <cassert>

#include <libtcod.hpp>

#include "actor.hpp"
#include "message_log.hpp"

static constexpr auto level_up_base = 200;
static constexpr auto level_up_factor = 150;

int Level::xp_to_next_level(void) const {
  return level_up_base + current * level_up_factor;
}

void Level::add_xp(flecs::world ecs, int new_xp) {
  assert(new_xp > 0);
  xp += new_xp;
  auto msg = tcod::stringf("You gain %d experience points", new_xp);
  auto &log = ecs.lookup("messageLog").get_mut<MessageLog>();
  log.addMessage(msg);

  if (requires_level_up()) {
    msg = tcod::stringf("You advance to level %d", current + 1);
    log.addMessage(msg);
  }
}

void Level::increaseLevel(void) {
  xp -= xp_to_next_level();
  current++;
}

const char *Level::increase_max_hp(flecs::entity e, int amount) {
  static constexpr auto msg = "Your health improves!";

  auto fighter = e.get_mut<Fighter>();
  fighter.max_hp += amount;
  fighter.heal(amount, e);
  increaseLevel();

  return msg;
}

const char *Level::increase_power(flecs::entity e, int amount) {
  static constexpr auto msg = "You feel stronger!";

  e.get_mut<Fighter>().power += amount;
  increaseLevel();

  return msg;
}

const char *Level::increase_defense(flecs::entity e, int amount) {
  static constexpr auto msg = "Your movements are getting swifter!";

  e.get_mut<Fighter>().defense += amount;
  increaseLevel();

  return msg;
}
