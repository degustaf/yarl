#pragma once

#include <string>
#include <vector>

#include "color.hpp"
#include "console.hpp"

struct Message {
  std::string plain_text;
  color::RGBA fg;
  int count = 1;

  std::string fullText(void) const;
};

struct MessageLog {
  void addMessage(const std::string &text, color::RGBA fg = Colors::text,
                  bool stack = true);
  size_t size(void) const { return messages.size(); };
  void render(Console &console, int x, int y, int width, int height) const;
  void render(Console &console, int x, int y, int width, int height,
              size_t offset) const;

  std::vector<Message> messages;

private:
  void render(Console &console, int x, int y, int width, int height,
              std::vector<Message>::const_reverse_iterator rbegin) const;
};
