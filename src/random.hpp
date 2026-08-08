#pragma once

#include <algorithm>

template <typename _RandomAccessIterator, typename _RandomNumberGenerator>
void random_shuffle(_RandomAccessIterator __first, _RandomAccessIterator __last,
                    _RandomNumberGenerator &&__rand) {
  if (__first == __last)
    return;
  for (auto __i = __first + 1; __i != __last; ++__i) {
    auto __j = __first + __rand((__i - __first) + 1);
    if (__i != __j)
      std::iter_swap(__i, __j);
  }
}
