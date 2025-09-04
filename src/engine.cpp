#include "engine.hpp"

#include <cstdint>
#include <fstream>
#include <limits>

#include <libtcod.hpp>
#include <sstream>

#include "ai.hpp"
#include "game_map.hpp"
#include "inventory.hpp"
#include "level.hpp"
#include "message_log.hpp"
#include "room_accretion.hpp"

void Engine::handle_enemy_turns(flecs::world ecs) {
  auto map = ecs.lookup("currentMap").target<CurrentMap>();

  auto q = ecs.query_builder<Ai>("module::monsterAi")
               .with(flecs::ChildOf, map)
               .build();

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

void Engine::save_as(flecs::world ecs, const std::string &file_name) {
  auto output = std::ofstream(file_name);
  output << ecs.to_json();
}

bool Engine::load(flecs::world ecs, const std::string &file_name,
                  EventHandler &eventHandler) {
  auto input = std::ifstream(file_name);
  if (input.fail()) {
    eventHandler.makePopup(
        [](auto e) { e->mainMenu(); },
        [](auto e, auto world, auto &c) { e->MainMenuOnRender(world, c); },
        "No saved game to load.");
    return false;
  }

  auto buffer = std::stringstream();
  buffer << input.rdbuf();
  if (ecs.from_json(buffer.str().c_str()) == nullptr) {
    eventHandler.makePopup(
        [](auto e) { e->mainMenu(); },
        [](auto e, auto world, auto &c) { e->MainMenuOnRender(world, c); },
        "Failed to load save.");
    return false;
  }

  auto currentmap = ecs.lookup("currentMap");
  if (currentmap == currentmap.null()) {
    eventHandler.makePopup(
        [](auto e) { e->mainMenu(); },
        [](auto e, auto world, auto &c) { e->MainMenuOnRender(world, c); },
        "Failed to load save.");
    return false;
  }
  auto map = currentmap.target<CurrentMap>();
  auto &gamemap = map.get_mut<GameMap>();
  gamemap.init();
  auto player = ecs.lookup("player");
  generateDungeon(map, gamemap, player, false);
  gamemap.update_fov(player);

  return true;
}

void Engine::new_game(flecs::world ecs) {
  const int map_width = 80;
  const int map_height = 43;

  auto seed = (uint32_t)TCODRandom::getInstance()->getInt(
      0, (int)std::numeric_limits<int32_t>::max());
  ecs.entity("seed").set<Seed>({seed});
  auto player = ecs.entity("player")
                    .set<Position>({0, 0})
                    .set<Renderable>({'@', {255, 255, 255}, RenderOrder::Actor})
                    .set<Named>({"Player"})
                    .emplace<Fighter>(30, 1, 2)
                    .set<Inventory>({26})
                    .emplace<Level>();
  auto dagger =
      ecs.entity().is_a(ecs.lookup("module::dagger")).add<ContainedBy>(player);
  toggleEquip<false>(player, dagger);
  auto leather = ecs.entity()
                     .is_a(ecs.lookup("module::leatherArmor"))
                     .add<ContainedBy>(player);
  toggleEquip<false>(player, leather);

  auto map = ecs.entity();
  map.emplace<GameMap>(generateDungeon(map, map_width, map_height, 1, player));
  ecs.entity("currentMap").add<CurrentMap>(map);
  map.get_mut<GameMap>().update_fov(player);

  ecs.entity("messageLog")
      .set<MessageLog>({})
      .get_mut<MessageLog>()
      .addMessage("Hello and welcome, adventurer, to yet another dungeon!",
                  color::welcomeText);
}
