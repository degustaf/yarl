#pragma once

#include <string>
#include <vector>

#include <libtcod.hpp>

#include "color.hpp"

struct Message {
  std::string plain_text;
  tcod::ColorRGB fg;
  int count = 1;

  std::string fullText(void) const;
};

struct MessageLog {
  void addMessage(const std::string &text, tcod::ColorRGB fg = color::text,
                  bool stack = true);
  size_t size(void) const { return messages.size(); };
  void render(tcod::Console &console, int x, int y, int width,
              int height) const;
  void render(tcod::Console &console, int x, int y, int width, int height,
              size_t offset) const;

  std::vector<Message> messages;

private:
  void render(tcod::Console &console, int x, int y, int width, int height,
              std::vector<Message>::const_reverse_iterator rbegin) const;
};
