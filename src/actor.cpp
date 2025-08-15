#include "actor.hpp"

#include <algorithm>

#include "ai.hpp"
#include "engine.hpp"
#include "input_handler.hpp"

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
  ecs.query_builder()
      .with(flecs::IsA, ecs.component<Ai>())
      .build()
      .each([&](auto e) {
        if (self.has(e)) {
          self.remove(e);
        }
      });

  auto &name = self.get_mut<Named>();
  auto player = ecs.lookup("player");
  auto &engine = ecs.get_mut<Engine>();
  if (self == player) {
    engine.messageLog.addMessage("You died!", color::playerDie);
    engine.eventHandler.keyDown = &EventHandler::GameOverKeyDown;
  } else {
    auto msg = tcod::stringf("%s is dead!", name.name.c_str());
    engine.messageLog.addMessage(msg, color::EnemyDie);
  }
  name.name = "remains of " + name.name;
}

void Renderable::render(tcod::Console &console, const Position &pos) const {
  auto &tile = console.at(pos);
  tile.ch = ch;
  tile.fg = color;
}
