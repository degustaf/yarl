#pragma once

#include <string>
#include <vector>

#include "random.hpp"

struct Book {
  std::string title;
  std::vector<std::string> body;
};

Book randomBook(Random &rng, int width);
