#include "actor.hpp"

#include <algorithm>
#include <cassert>

#include "ai.hpp"
#include "game_map.hpp"
#include "input_handler.hpp"
#include "inventory.hpp"
#include "level.hpp"
#include "message_log.hpp"
#include "string.hpp"

const std::vector<RenderOrder> allRenderOrders = {
    RenderOrder::Corpse, RenderOrder::Item, RenderOrder::Actor};

void toggleDoor(flecs::entity door) {
  assert(door.has<Openable>());
  auto p = door.get<Position>();
  auto r = door.get<Renderable>();
  auto &map =
      door.world().lookup("currentMap").target<CurrentMap>().get_mut<GameMap>();
  if (door.has<BlocksMovement>()) {
    door.remove<BlocksMovement>().remove<BlocksFov>();
    assert(r.ch == '+');
    r.ch = '-';
    r.layer = RenderOrder::Item;
    door.set<Renderable>(r);
    map.setProperties(p.x, p.y, true, true);
  } else {
    door.add<BlocksMovement>().add<BlocksFov>();
    assert(r.ch == '-');
    r.ch = '+';
    r.layer = RenderOrder::Actor;
    door.set<Renderable>(r);
    map.setProperties(p.x, p.y, false, false);
  }
}

void Fighter::set_hp(int value, flecs::entity self) {
  _hp = std::clamp(value, 0, max_hp);
  if (_hp == 0) {
    die(self);
  }
}

int Fighter::heal(int amount, flecs::entity self) {
  if (_hp == max_hp) {
    return 0;
  }
  auto amountRecovered = -_hp;
  set_hp(_hp + amount, self);
  amountRecovered += _hp;

  return amountRecovered;
}

void Fighter::take_damage(int amount, flecs::entity self) {
  set_hp(_hp - amount, self);
}

void Fighter::die(flecs::entity self) {
  auto &render = self.get_mut<Renderable>();
  render.ch = '%';
  render.fg = color::RGB{191, 0, 0};
  render.layer = RenderOrder::Corpse;
  self.remove<BlocksMovement>();

  auto ecs = self.world();
  auto win = false;
  ecs.query_builder("module::ai")
      .with(flecs::IsA, ecs.component<Ai>())
      .build()
      .each([&](auto e) {
        if (self.has(e)) {
          self.remove(e);
        }
      });

  ecs.query_builder("module::onDeath")
      .with(flecs::IsA, ecs.component<OnDeath>())
      .build()
      .each([self](auto e) {
        if (self.has(e)) {
          auto onDeath = static_cast<const OnDeath *>(self.try_get(e));
          onDeath->onDeath(self);
          self.remove(e);
        }
      });

  auto &name = self.get_mut<Named>();
  auto player = ecs.lookup("player");
  auto &messageLog = ecs.lookup("messageLog").get_mut<MessageLog>();
  if (self == player) {
    messageLog.addMessage("You died!", color::playerDie);
    make<GameOver>(ecs);
  } else {
    auto msg = stringf("%s is dead!", name.name.c_str());
    messageLog.addMessage(msg, color::enemyDie);
    auto xp = self.try_get<XP>();
    if (xp)
      player.get_mut<Level>().add_xp(ecs, xp->given);
    if (win)
      make<WinScreen>(ecs);
  }
  name.name = "remains of " + name.name;
}

int Fighter::defense(flecs::entity self) const {
  auto defense = base_defense;
  auto weapon = self.target<Weapon>();
  if (weapon) {
    defense += weapon.get<Equippable>().defense_bonus;
  }
  auto armor = self.target<Armor>();
  if (armor) {
    defense += armor.get<Equippable>().defense_bonus;
  }

  return defense;
}

int Fighter::power(flecs::entity self, bool ranged) const {
  auto power = base_power;
  auto weapon = self.target<Weapon>();
  if (weapon && (weapon.has<Ranged>() == ranged)) {
    power += weapon.get<Equippable>().power_bonus;
  }
  auto armor = self.target<Armor>();
  if (armor) {
    power += armor.get<Equippable>().power_bonus;
  }

  return power;
}

void Renderable::render(Console &console, const Position &pos,
                        bool inFov) const {
  if (fovOnly && !inFov) {
    return;
  }
  auto &tile = console.at(pos);
  tile.encodeChar(ch);
  tile.fg = inFov ? fg : (fg / darknessFactor);
  if (bg) {
    tile.bg = inFov ? *bg : (*bg / darknessFactor);
  }
  tile.flipped = flipped;
}

void Regenerator::update(flecs::entity self) {
  turns++;
  if (turns >= healTurns) {
    self.get_mut<Fighter>().heal(1, self);
    turns -= healTurns;
  }
}

void Frozen::update(flecs::entity self) {
  turns--;
  if (turns <= 0) {
    self.remove<Frozen>();
  }
}
