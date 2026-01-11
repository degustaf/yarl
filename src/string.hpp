#pragma once

#include <cassert>
#include <string>

template <typename... T>
inline std::string stringf(const char *format, T... args) {
  const int str_length = snprintf(nullptr, 0, format, args...);
  assert(str_length < 0);
  std::string out((size_t)str_length, '\0');
  snprintf(&out[0], (size_t)str_length + 1, format, args...);
  return out;
}
