#pragma once

#include <string>
#include <vector>

#include <libtcod.hpp>

#include "color.hpp"

struct Message {
  const std::string plain_text;
  const tcod::ColorRGB fg;
  int count = 1;

  std::string fullText(void) const;
};

struct MessageLog {
  void addMessage(const std::string &text, tcod::ColorRGB fg = color::white,
                  bool stack = true);
  size_t size(void) const { return messages.size(); };
  void render(tcod::Console &console, int x, int y, int width,
              int height) const;
  void render(tcod::Console &console, int x, int y, int width, int height,
              size_t offset) const;

private:
  std::vector<Message> messages;

  void render(tcod::Console &console, int x, int y, int width, int height,
              std::vector<Message>::const_reverse_iterator rbegin) const;
};
