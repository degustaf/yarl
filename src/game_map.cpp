#include "game_map.hpp"

#include <cstddef>

#include "color.hpp"
#include "defines.hpp"
#include "room_accretion.hpp"
#include "scent.hpp"
#include "string.hpp"

static inline void deleteMapEntity(flecs::world ecs, flecs::entity map) {
  auto q = ecs.query_builder("module::mapEntities")
               .with(flecs::ChildOf, map)
               .build();

  auto player = ecs.lookup("player");
  ecs.defer_begin();
  q.each([player](auto e) {
    if (player != e)
      e.destruct();
  });
  ecs.defer_end();

  auto module = ecs.lookup("module");
  assert(module);
  auto q2 = ecs.query_builder()
                .with(flecs::Query)
                .with(flecs::ChildOf, module)
                .build();
  ecs.defer_begin();
  q2.each([](auto e) { e.destruct(); });
  ecs.defer_end();
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

void GameMap::carveOut(int x, int y) { map.setProperties(x, y, true, true); }

void GameMap::nextFloor(flecs::entity player, bool lit) const {
  auto ecs = player.world();
  auto newMap = ecs.entity();
  newMap.set<GameMap>(roomAccretion::generateDungeon(newMap, width, height,
                                                     level + 1, player, lit));
  auto oldMap = ecs.lookup("currentMap").target<CurrentMap>();
  ecs.lookup("currentMap").add<CurrentMap>(newMap);
  deleteMapEntity(oldMap);
}

static constexpr auto floor_light =
    Console::Tile{'.', color::lightFG, color::lightFloor};
static constexpr auto floor_dark =
    Console::Tile{'.', color::darkFG, color::darkFloor};
static constexpr auto floor_sensed =
    Console::Tile{'.', color::sensedFG, color::sensedFloor};

static constexpr auto bloody_floor_light =
    Console::Tile{'.', color::blood, color::lightFloor};
static constexpr auto bloody_floor_dark =
    Console::Tile{'.', color::blood, color::darkFloor};

static constexpr auto wall_light =
    Console::Tile{'#', color::walls, color::lightWallbg};
static constexpr auto wall_dark =
    Console::Tile{'#', color::walls, color::darkWallbg};

static constexpr auto stairs_light =
    Console::Tile{'>', color::stairs, color::lightFloor};
static constexpr auto stairs_dark =
    Console::Tile{'>', color::stairs, color::darkFloor};
static constexpr auto stairs_sensed =
    Console::Tile{'>', color::stairs, color::sensedFloor};

static constexpr auto shroud =
    Console::Tile{' ', color::text, color::background};

static constexpr auto water_light =
    Console::Tile{'~', color::water_fg, color::water_bg};
static constexpr auto water_dark =
    Console::Tile{'~', color::dark_water_fg, color::dark_water_bg};

static constexpr auto chasm_light =
    Console::Tile{0x2591, color::chasmFG, color::chasm};
static constexpr auto chasm_dark =
    Console::Tile{0x2591, color::chasmFG, color::darkFloor};

void GameMap::render(Console &console, uint64_t time) {
  float vec[3] = {0, 0, (float)time / (1000.0f)};
  for (auto y = 0; y < height; y++) {
    vec[1] = (float)y;
    for (auto x = 0; x < width; x++) {
      vec[0] = (float)x;
      if (isVisible(x, y)) {
        console.at({x, y}) = isStairs({x, y})          ? stairs_light
                             : isKnownBloody({x, y})   ? bloody_floor_light
                             : map.isWalkable(x, y)    ? floor_light
                             : isWater(x, y)           ? water_light
                             : map.isTransparent(x, y) ? chasm_light
                                                       : wall_light;
        if (isWater(x, y)) {
          console.at({x, y}).bg += (int8_t)(63 * noise.get(vec));
        }
      } else if (isExplored(x, y)) {
        console.at({x, y}) = isStairs({x, y})          ? stairs_dark
                             : isKnownBloody({x, y})   ? bloody_floor_dark
                             : map.isWalkable(x, y)    ? floor_dark
                             : isWater(x, y)           ? water_dark
                             : map.isTransparent(x, y) ? chasm_dark
                                                       : wall_dark;
        if (isWater(x, y)) {
          console.at({x, y}).bg += (int8_t)(31 * noise.get(vec));
        }
      } else if (isSensed(x, y)) {
        console.at({x, y}) = isStairs({x, y})          ? stairs_sensed
                             : map.isWalkable(x, y)    ? floor_sensed
                             : isWater(x, y)           ? water_dark
                             : map.isTransparent(x, y) ? chasm_dark
                                                       : wall_dark;
        if (isWater(x, y)) {
          console.at({x, y}).bg += (int8_t)(31 * noise.get(vec));
        }
      } else {
        console.at({x, y}) = shroud;
      }
    }
  }
}

void GameMap::update_fov(flecs::entity mapEntity, flecs::entity player) {
  auto pos = player.get<Position>();
  computeFov(mapEntity, *this, pos, 8);
  if (lit) {
    for (auto &l : luminosity) {
      l = 1.0f;
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
  auto q = map.world()
               .query_builder<const Scent, const Position>("module::scent")
               .with(flecs::ChildOf, map)
               .build();
  q.each([&](auto s, auto p) { getScent(p) += s; });
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

  scent = newScents;
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
  auto q = map.world()
               .query_builder<const Position>("module::blocksPosition")
               .with(flecs::ChildOf, map)
               .with<BlocksMovement>()
               .build();
  return q.find([&](const auto &p) { return p == pos; });
}
