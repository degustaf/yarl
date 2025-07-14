#include "tiles.hpp"

tile tile::floor_tile = {
    true, true, {' ', {255, 255, 255, 255}, {50, 50, 150, 255}}};
tile tile::wall_tile = {
    false, false, {' ', {255, 255, 255, 255}, {0, 0, 100, 255}}};
