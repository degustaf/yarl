#include "engine.hpp"

#include <libtcod.hpp>

#include "ai.hpp"
#include "game_map.hpp"

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
