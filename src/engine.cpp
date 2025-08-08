#include "engine.hpp"

#include <libtcod.hpp>

#include "actor.hpp"
#include "ai.hpp"
#include "game_map.hpp"
#include "render_functions.hpp"

void Engine::render(flecs::world ecs) const {
  auto &console = ecs.get_mut<tcod::Console>();

  auto map = ecs.target<CurrentMap>();
  auto &gMap = map.get_mut<GameMap>();
  gMap.render(console);

  auto q = ecs.query_builder<const Position, const Renderable>()
               .with(flecs::ChildOf, map)
               .order_by<const Renderable>([](auto, auto r1, auto, auto r2) {
                 return (int)(r1->layer - r2->layer);
               })
               .build();

  q.each([&](auto p, auto r) {
    if (gMap.isInFov(p.x, p.y)) {
      r.render(console, p);
    }
  });

  auto player = ecs.lookup("player");
  player.get<Renderable>().render(console, player.get<Position>());

  auto fighter = player.get<Fighter>();
  renderBar(console, fighter.hp(), fighter.max_hp, 20);

  ecs.get_mut<tcod::Context>().present(console);
  console.clear();
}

void Engine::handle_enemy_turns(flecs::world ecs) const {
  auto map = ecs.target<CurrentMap>();

  auto q = ecs.query_builder<Ai>().with(flecs::ChildOf, map).build();

  q.run([](flecs::iter &it) {
    while (it.next()) {
      for (auto i : it) {
        auto e = it.entity(i);
        auto ai_id = it.id(0);
        auto ai_type = static_cast<Ai *>(e.try_get_mut(ai_id));
        auto action = ai_type->act(e);
        if (action) {
          action->perform(e);
        }
      }
    }
  });
}
