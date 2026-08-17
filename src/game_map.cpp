#include "game_map.hpp"

#include <cstddef>

#include "color.hpp"
#include "console.hpp"
#include "defines.hpp"
#include "fov.hpp"
#include "map_queries.hpp"
#include "room_accretion.hpp"
#include "scent.hpp"
#include "string.hpp"

void deleteMapQueries(flecs::world ecs) {
  auto module = ecs.lookup("Queries");
  if (module) {
    auto q2 = ecs.query_builder()
                  .with(flecs::Query)
                  .with(flecs::ChildOf, module)
                  .build();
    ecs.defer_begin();
    q2.each([](auto e) { e.destruct(); });
    ecs.defer_end();
  }
}

inline void deleteMapEntity(flecs::world ecs, flecs::entity map) {
  auto q = ecs.query_builder("Queries::mapEntities")
               .with(flecs::ChildOf, map)
               .build();

  auto player = ecs.lookup("player");
  ecs.defer_begin();
  q.each([player](auto e) {
    if (player != e)
      e.destruct();
  });
  ecs.defer_end();

  deleteMapQueries(ecs);
  map.destruct();
}

void deleteMapEntity(flecs::entity map) { deleteMapEntity(map.world(), map); }

void deleteMapEntity(flecs::world ecs) {
  auto currentMap = ecs.lookup("currentMap");
  if (currentMap) {
    auto map = currentMap.target<CurrentMap>();
    deleteMapEntity(ecs, map);
  }
}

void GameMap::carveOut(int x, int y) { setProperties(x, y, true, true); }

void GameMap::nextFloor(flecs::entity player, bool lit) const {
  auto ecs = player.world();
  auto newMap = ecs.entity();
  auto cfg = roomAccretion::Config{};
  cfg.lit = lit;
  newMap.set<GameMap>(roomAccretion::generateDungeon(cfg, newMap, width, height,
                                                     level + 1, player));
  auto oldMap = ecs.lookup("currentMap").target<CurrentMap>();
  ecs.lookup("currentMap").add<CurrentMap>(newMap);
  newMap.add<PrevMap>(oldMap);
  oldMap.add<NextMap>(newMap);
  deleteMapQueries(ecs);
}

void GameMap::render(Console &console, uint64_t time) {
  float vec[3] = {0, 0, (float)time / (1000.0f)};
  for (auto y = 0; y < height; y++) {
    vec[1] = (float)y;
    for (auto x = 0; x < width; x++) {
      vec[0] = (float)x;
      if (isVisible(x, y)) {
        auto t = tiles[y * width + x].luminosity;
        console.at({x, y}) =
            isStairsDown({x, y})    ? lerp(TileModule::stairs_down_light,
                                           TileModule::stairs_down_dark, t)
            : isStairsUp({x, y})    ? lerp(TileModule::stairs_up_light,
                                           TileModule::stairs_up_dark, t)
            : isKnownBloody({x, y}) ? lerp(TileModule::bloody_floor_light,
                                           TileModule::bloody_floor_dark, t)
            : isWalkable(x, y)
                ? lerp(TileModule::floor_light, TileModule::floor_dark, t)
            : isWater(x, y)
                ? lerp(TileModule::water_light, TileModule::water_dark, t)
            : isTransparent(x, y)
                ? lerp(TileModule::chasm_light, TileModule::chasm_dark, t)
                : lerp(TileModule::wall_light, TileModule::wall_dark, t);
        if (isWater(x, y)) {
          auto scale = 63.0f * t + 31.0f * (1 - t);
          console.at({x, y}).bg += (int8_t)(scale * noise.get(vec));
        }
      } else if (isExplored(x, y)) {
        console.at({x, y}) = isStairsDown({x, y}) ? TileModule::stairs_down_dark
                             : isStairsUp({x, y}) ? TileModule::stairs_up_dark
                             : isKnownBloody({x, y})
                                 ? TileModule::bloody_floor_dark
                             : isWalkable(x, y)    ? TileModule::floor_dark
                             : isWater(x, y)       ? TileModule::water_dark
                             : isTransparent(x, y) ? TileModule::chasm_dark
                                                   : TileModule::wall_dark;
        if (isWater(x, y)) {
          console.at({x, y}).bg += (int8_t)(31.0f * noise.get(vec));
        }
      } else if (isSensed(x, y)) {
        console.at({x, y}) = isStairsDown({x, y})
                                 ? TileModule::stairs_down_sensed
                             : isStairsUp({x, y}) ? TileModule::stairs_up_sensed
                             : isWalkable(x, y)   ? TileModule::floor_sensed
                             : isWater(x, y)      ? TileModule::water_dark
                             : isTransparent(x, y) ? TileModule::chasm_dark
                                                   : TileModule::wall_dark;
        if (isWater(x, y)) {
          console.at({x, y}).bg += (int8_t)(31 * noise.get(vec));
        }
      } else {
        console.at({x, y}) = TileModule::shroud;
      }
    }
  }
}

void GameMap::update_fov(flecs::entity mapEntity, flecs::entity player) {
  auto pos = player.get<Position>();
  computeFov(mapEntity, *this, pos, 8);
  if (lit) {
    for (auto &t : tiles) {
      t.luminosity = 1.0f;
    }
  } else {
    addLight(mapEntity, *this);
  }
  for (auto y = 0; y < height; y++) {
    for (auto x = 0; x < width; x++) {
      if (isVisible(x, y)) {
        auto &tile = tiles[(size_t)(y * width + x)];
        tile.flags |= Tile::Explored;
        if (tile.flags & Tile::Bloody)
          tile.flags |= Tile::KnownBloody;
      }
    }
  }
}

static constexpr auto decayFactor = 0.9f;
static constexpr auto decayThreshold = 1.0f;

void GameMap::update_scent(flecs::entity map) {
  auto q = mapQuery<positionQuery::Scent, Scent>(map.world(), map);
  q.each([&](auto p, auto s) { getScent(p) += s; });
  auto player = map.world().lookup("player");
  getScent(player.get<Position>()) += player.get<Scent>();

  auto newScents = std::vector<Scent>(width * height);
  for (auto y = 0; y < height; y++) {
    for (auto x = 0; x < width; x++) {
      if (!isTransparent(x, y)) {
        continue;
      }

      std::array<float, static_cast<size_t>(ScentType::MAX)> levels = {};
      std::array<int, static_cast<size_t>(ScentType::MAX)> count = {};
      auto &s = getScent({x, y});
      levels[static_cast<size_t>(s.type)] = s.power;
      count[static_cast<size_t>(s.type)]++;
      for (auto &dir : directions) {
        auto x2 = x + dir[0];
        auto y2 = y + dir[1];
        if (inBounds(x2, y2) && isTransparent(x2, y2)) {
          auto &s = getScent({x2, y2});
          levels[static_cast<size_t>(s.type)] += s.power;
          count[static_cast<size_t>(s.type)]++;
        }
      }

      auto idx =
          std::max_element(levels.begin(), levels.end()) - levels.begin();
      auto &newS = newScents[y * width + x];
      newS = {static_cast<ScentType>(idx),
              decayFactor * (levels[idx] / (float)count[idx])};
      if (newS.type == ScentType::none || newS.power < decayThreshold) {
        newS = {ScentType::none, 0.0f};
      }
    }
  }

  for (size_t i = 0; i < tiles.size(); i++) {
    tiles[i].scent = newScents[i];
  }
}

void GameMap::reveal() {
  for (auto &tile : tiles)
    tile.flags |= Tile::Sensed;
}

ScentType GameMap::detectScent(flecs::entity e,
                               std::array<int, 2> &strongest) const {
  auto pos = e.get<Position>();
  auto smeller = e.try_get<Smeller>();
  if (smeller == nullptr) {
    return ScentType::none;
  }
  strongest = {0, 0};
  auto power = getScent(pos).power;
  for (auto &dir : directions) {
    auto &scent = getScent(pos + dir);
    if (scent.power > power) {
      power = scent.power;
      strongest = {dir[0], dir[1]};
    }
  }
  if (getScent(pos + strongest).power > smeller->threshold) {
    return getScent(pos + strongest).type;
  }
  strongest = {0, 0};
  return ScentType::none;
}

std::string GameMap::detectScent(flecs::entity e) const {
  auto strongest = std::array{0, 0};
  auto type = detectScent(e, strongest);
  if (type == ScentType::none) {
    return "";
  }
  auto scent = e.try_get<Scent>();
  if (scent && (scent->type == type)) {
    return "All you can smell is your own sweat.";
  }
  auto dir = directionName(strongest);
  if (dir.size() > 0) {
    return stringf("You stink of %s", scentName(type).c_str());
  }
  return stringf("You smell %s to the %s", scentName(type).c_str(),
                 dir.c_str());
}

flecs::entity GameMap::get_blocking_entity(flecs::entity map,
                                           const Position &pos) {
  auto player = map.world().lookup("player");
  if (player.get<Position>() == pos) {
    return player;
  }
  auto q = mapQuery<positionQuery::Blocks>(map.world(), map);
  return q.find([&](const auto &p) { return p == pos; });
}
