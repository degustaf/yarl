#include "actor.hpp"

#include <iostream>

#include "ai.hpp"
#include "engine.hpp"
#include "input_handler.hpp"

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
  if (self == player) {
    std::cout << "You died!\n";
    ecs.get_mut<Engine>().eventHandler.dispatch = GameOverDispatch;
  } else {
    std::cout << name.name << " is dead!\n";
  }
  name.name = "remains of " + name.name;
}

void Renderable::render(tcod::Console &console, const Position &pos) const {
  auto &tile = console.at(pos);
  tile.ch = ch;
  tile.fg = color;
}
