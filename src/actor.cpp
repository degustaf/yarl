#include "actor.hpp"

#include "ai.hpp"
#include "engine.hpp"
#include "input_handler.hpp"
#include <libtcod/console_printing.hpp>

const std::vector<RenderOrder> allRenderOrders = {Corpse, Item, Actor};

void Fighter::die(flecs::entity self) {
  auto &render = self.get_mut<Renderable>();
  render.ch = '%';
  render.color = {191, 0, 0};
  render.layer = Corpse;
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
    engine.eventHandler.keyDown = EventHandler::GameOverKeyDown;
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
