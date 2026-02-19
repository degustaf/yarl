#include "engine.hpp"

#include <libtcod.hpp>

#include <filesystem>
#include <fstream>
#include <sstream>

#include "actor.hpp"
#include "ai.hpp"
#include "color.hpp"
#include "game_map.hpp"
#include "input_handler.hpp"
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
            auto in = e.try_get_mut<Invisible>();
            if (in) {
              in->paused = false;
            }
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

bool Engine::load(flecs::world ecs, const std::filesystem::path &file_name,
                  MainMenuInputHandler &handler) {
  auto input = std::ifstream(file_name);
  if (input.fail()) {
    auto f = [](auto, auto &c) {
      c.print({c.get_width() / 2, c.get_height() / 2}, "No saved game to load.",
              color::text, color::background, Console::Alignment::CENTER);
    };
    makePopup<decltype(f)>(ecs, f, handler);
    return false;
  }

  auto buffer = std::stringstream();
  buffer << input.rdbuf();
  if (ecs.from_json(buffer.str().c_str()) == nullptr) {
    auto f = [](auto, auto &c) {
      c.print({c.get_width() / 2, c.get_height() / 2}, "Failed to load save.",
              color::text, color::background, Console::Alignment::CENTER);
    };
    makePopup<decltype(f)>(ecs, f, handler);
    return false;
  }

  auto currentmap = ecs.lookup("currentMap");
  if (currentmap == currentmap.null()) {
    auto f = [](auto, auto &c) {
      c.print({c.get_width() / 2, c.get_height() / 2}, "Failed to load save.",
              color::text, color::background, Console::Alignment::CENTER);
    };
    makePopup<decltype(f)>(ecs, f, handler);
    return false;
  }
  auto map = currentmap.target<CurrentMap>();
  auto &gamemap = map.get_mut<GameMap>();
  gamemap.init();
  auto player = ecs.lookup("player");
  const auto cfg = roomAccretion::Config{};
  roomAccretion::generateDungeon(cfg, map, gamemap, player, false);
  gamemap.update_fov(map, player);

  return true;
}

void Engine::new_game(flecs::world ecs, int map_width, int map_height) {
  auto seed = (uint32_t)TCODRandom::getInstance()->getInt(
      0, (int)std::numeric_limits<int32_t>::max());
  ecs.entity("seed").set<Seed>({seed});
  ecs.entity("turn").set<Turn>({0});
  auto player = ecs.entity("player")
                    .set<Position>({0, 0})
                    .set<Renderable>(
                        {'@', color::player, std::nullopt, RenderOrder::Actor})
                    .set<Named>({"Player"})
                    .emplace<Fighter>(10, 1, 2)
                    .set<Inventory>({26})
                    .emplace<Level>()
                    .set<Scent>({ScentType::player, 0})
                    .set<ScentWarning>({false})
                    .set<Smeller>({200});
  auto sword =
      ecs.entity().is_a(ecs.lookup("module::sword")).add<ContainedBy>(player);
  toggleEquip<false>(player, sword);

  auto map = ecs.entity();
  auto cfg = roomAccretion::Config{};
  cfg.ROOM_MIN_SIZE = 2;
  cfg.MAX_ROOMS = 300;
  cfg.MAX_ITER = 1000;
  cfg.LAKE_ITER = 0;
  map.emplace<GameMap>(roomAccretion::generateDungeon(cfg, map, map_width,
                                                      map_height, 1, player));
  ecs.entity("currentMap").add<CurrentMap>(map);
  map.get_mut<GameMap>().update_fov(map, player);

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
  auto turn = ecs.lookup("turn");
  if (turn)
    turn.destruct();
  auto player = ecs.lookup("player");
  if (player)
    player.destruct();
  auto log = ecs.lookup("messageLog");
  if (log)
    log.destruct();
}
