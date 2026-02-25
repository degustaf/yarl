#include "books.hpp"
#include <cctype>
#include <vector>

static const auto loremIpsum = std::string(
    "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Sed id malesuada "
    "mi. Lorem ipsum dolor sit amet, consectetur adipiscing elit. Nulla at "
    "ante elit. Pellentesque eleifend tempus mauris a rutrum. Integer viverra, "
    "tellus ac mollis auctor, ex est iaculis metus, eu sollicitudin mauris "
    "metus quis nulla. Phasellus vulputate semper tempus. Sed pellentesque "
    "vulputate nunc at ornare. Fusce id nulla diam. Cras fermentum tellus "
    "eros, at sollicitudin magna iaculis luctus. Proin porttitor ut sapien a "
    "tempus. Praesent at felis neque. Duis tincidunt nisi in arcu hendrerit "
    "suscipit. Suspendisse id tincidunt libero. Cras auctor lectus sed rhoncus "
    "efficitur. Nulla quis nunc ex. Ut turpis massa, laoreet eget congue at, "
    "congue ut arcu. Praesent venenatis eleifend sem. Integer ut accumsan "
    "tortor. Nulla massa justo, facilisis vitae congue eu, laoreet id nisi. "
    "Duis facilisis eget nisl id sollicitudin. Cras suscipit, ligula sed "
    "fermentum mattis, odio nisl sollicitudin elit, id pellentesque eros risus "
    "eget justo. Vivamus molestie nec neque non viverra. Aenean ullamcorper "
    "tristique quam at bibendum. Sed interdum tempus mi, sed cursus libero "
    "tempor vitae. Sed ac dapibus risus. Vestibulum sed consequat lacus, "
    "mattis consectetur sapien. Donec luctus sagittis sem, quis interdum velit "
    "fermentum eu.");

std::vector<std::string> split(const std::string &s, char delimiter) {
  std::vector<std::string> tokens;
  std::string token;
  std::istringstream tokenStream(s);
  while (std::getline(tokenStream, token, delimiter)) {
    tokens.push_back(token);
  }
  return tokens;
}

std::vector<std::string> splitLines(const std::string &in, int maxWidth) {
  auto ret = std::vector<std::string>{};
  int start = 0;

  while (start + maxWidth < (int)in.length()) {
    for (auto i = maxWidth; i >= 0; i--) {
      if (isspace(in[(size_t)(start + i)])) {
        ret.push_back(in.substr((size_t)start, (size_t)i));
        start += i + 1;
        break;
      }
    }
  }
  ret.push_back(in.substr((size_t)start));

  return ret;
}

std::string join(const std::vector<std::string> &v, int n) {
  auto ret = v[0];
  for (auto i = 1; i < n; i++) {
    ret += " " + v[(size_t)i];
  }
  return ret;
}

Book randomBook(TCODRandom &rng, int width) {
  static const auto lorem = split(loremIpsum, ' ');
  static const auto loremLength = (int)lorem.size();

  auto len = rng.get(loremLength / 4, loremLength);
  auto text = join(lorem, len);
  return {"Lorem Ipsum", splitLines(text, width)};
}
