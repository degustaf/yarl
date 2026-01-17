#include "engine.hpp"

#include <libtcod.hpp>

#include "actor.hpp"
#include "ai.hpp"
#include "game_map.hpp"
#include "inventory.hpp"
#include "level.hpp"
#include "message_log.hpp"
#include "room_accretion.hpp"
#include "scent.hpp"

void Engine::handle_enemy_turns(flecs::world ecs) {
  auto map = ecs.lookup("currentMap").target<CurrentMap>();

  auto q = ecs.query_builder<Ai>("module::monsterAi")
               .with(flecs::ChildOf, map)
               .build();

  ecs.defer_begin();
  q.run([](flecs::iter &it) {
    while (it.next()) {
      for (auto i : it) {
        auto e = it.entity(i);
        auto ai_id = it.id(0);
        auto ai_type = static_cast<Ai *>(e.try_get_mut(ai_id));
        auto frozen = e.try_get_mut<Frozen>();
        if (frozen) {
          frozen->update(e);
        } else {
          auto action = ai_type->act(e);
          if (action) {
            action->perform(e);
          }
        }
        auto regenerator = e.try_get_mut<Regenerator>();
        if (regenerator) {
          regenerator->update(e);
        }
      }
    }
  });
  ecs.defer_end();
}

void Engine::save_as(flecs::world ecs, const std::filesystem::path &file_name) {
  auto output = std::ofstream(file_name);
  output << ecs.to_json();
}

void Engine::new_game(flecs::world ecs, int map_width, int map_height) {
  auto seed = (uint32_t)TCODRandom::getInstance()->getInt(
      0, (int)std::numeric_limits<int32_t>::max());
  ecs.entity("seed").set<Seed>({seed});
  auto player =
      ecs.entity("player")
          .set<Position>({0, 0})
          .set<Renderable>({'@', {0, 0, 100}, std::nullopt, RenderOrder::Actor})
          .set<Named>({"Player"})
          .emplace<Fighter>(10, 1, 2)
          .set<Inventory>({26})
          .emplace<Level>()
          .set<Scent>({ScentType::player, 0})
          .set<ScentWarning>({false})
          .set<Smeller>({200});
  auto pistol =
      ecs.entity().is_a(ecs.lookup("module::22")).add<ContainedBy>(player);
  toggleEquip<false>(player, pistol);

  auto map = ecs.entity();
  map.emplace<GameMap>(
      roomAccretion::generateDungeon(map, map_width, map_height, 1, player));
  ecs.entity("currentMap").add<CurrentMap>(map);
  map.get_mut<GameMap>().update_fov(player);

  ecs.entity("messageLog")
      .set<MessageLog>({})
      .get_mut<MessageLog>()
      .addMessage("You are in Facility 14. You must plumb the depths, avoiding "
                  "the Fiend until you find the Laser of Yendor, the only "
                  "thing that is capable of defeating it.",
                  color::welcomeText);
}

void Engine::clear_game_data(flecs::world ecs) {
  deleteMapEntity(ecs);
  auto seed = ecs.lookup("seed");
  if (seed)
    seed.destruct();
  auto player = ecs.lookup("player");
  if (player)
    player.destruct();
  auto log = ecs.lookup("messageLog");
  if (log)
    log.destruct();
}
