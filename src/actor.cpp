#include "actor.hpp"

#include <algorithm>

#include "ai.hpp"
#include "input_handler.hpp"
#include "level.hpp"
#include "message_log.hpp"

const std::vector<RenderOrder> allRenderOrders = {
    RenderOrder::Corpse, RenderOrder::Item, RenderOrder::Actor};

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
  render.color = {191, 0, 0};
  render.layer = RenderOrder::Corpse;
  self.remove<BlocksMovement>();

  auto ecs = self.world();
  ecs.query_builder("module::ai")
      .with(flecs::IsA, ecs.component<Ai>())
      .build()
      .each([&](auto e) {
        if (self.has(e)) {
          self.remove(e);
        }
      });

  auto &name = self.get_mut<Named>();
  auto player = ecs.lookup("player");
  auto &messageLog = ecs.lookup("messageLog").get_mut<MessageLog>();
  if (self == player) {
    messageLog.addMessage("You died!", color::playerDie);
    ecs.get_mut<EventHandler>().gameOver();
  } else {
    auto msg = tcod::stringf("%s is dead!", name.name.c_str());
    messageLog.addMessage(msg, color::enemyDie);
    player.get_mut<Level>().add_xp(ecs, self.get<XP>().given);
  }
  name.name = "remains of " + name.name;
}

void Renderable::render(tcod::Console &console, const Position &pos) const {
  auto &tile = console.at(pos);
  tile.ch = ch;
  tile.fg = color;
}
