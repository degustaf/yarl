#include "message_log.hpp"

#include <cassert>
#include <optional>
#include <vector>

std::string Message::fullText(void) const {
  if (count > 1) {
    return tcod::stringf("%s (x%d)", plain_text.c_str(), count);
  }
  return plain_text;
}

void MessageLog::addMessage(const std::string &text, tcod::ColorRGB fg,
                            bool stack) {
  if (stack && messages.size() > 0 && messages.back().plain_text == text) {
    messages.back().count++;
  } else {
    messages.push_back({text, fg});
  }
}

struct substringIndices {
  std::string::size_type pos;
  std::string::size_type count;
};

static std::vector<substringIndices> wrap(const std::string &str,
                                          std::string::size_type width) {
  assert(width > 0);
  auto ret = std::vector<substringIndices>();

  std::string::size_type start = 0;
  while (start + width < str.size()) {
    auto end = str.rfind(' ', start + width);
    ret.push_back({start, end - start});
    start = end + 1;
  }
  ret.push_back({start, std::string::npos});

  return ret;
}

void MessageLog::render(tcod::Console &console, int x, int y, int width,
                        int height) const {
  auto y_offset = height - 1;
  for (auto it = messages.rbegin(); it != messages.rend(); it++) {
    assert(width >= 0);
    const auto &str = it->fullText();
    auto lines = wrap(str, (std::string::size_type)width);
    for (auto jt = lines.rbegin(); jt != lines.rend(); jt++) {
      tcod::print(console, {x, y + y_offset}, str.substr(jt->pos, jt->count),
                  it->fg, std::nullopt);
      y_offset--;
      if (y_offset < 0) {
        return;
      }
    }
  }
}
