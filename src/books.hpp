#pragma once

#include <string>
#include <vector>

#include <libtcod.hpp>

struct Book {
  std::string title;
  std::vector<std::string> body;
};

Book randomBook(TCODRandom &rng, int width);
