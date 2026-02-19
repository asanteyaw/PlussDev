#include <array>
#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

#include "CodeN/core/Chord.h"
#include "CodeN/core/OpsInternal.h"
#include "pluss/coden.h"
#include "pluss/coden/note_factory.h"

int main() {
  const auto scalar = pluss::content("hello");
  std::cout << "scalar: " << scalar.scalar() << "\n";

  const std::vector<std::string> flat = {"a", "b", "c", "d"};
  const std::array<std::int64_t, 2> sizes{2, 2};
  const auto matrix = pluss::make_note(flat, sizes);

  std::cout << "matrix.debug(): " << matrix.debug(8) << "\n";
  std::cout << "join_flat('-'): " << matrix.join_flat("-") << "\n";

  const auto m = matrix.matrix();
  std::cout << "matrix(1, 0): " << m[1][0] << "\n";
  std::cout << "flat index [1,1]: " << matrix.at_indices(std::array<std::int64_t, 2>{1, 1}) << "\n";

  std::cout << "iterate:";
  for (auto cell : matrix) {
    std::cout << " " << cell;
  }
  std::cout << "\n";

  const auto chord = pluss::coden::ops_internal::chord_from_utf8("Ab1");
  std::cout << "chord.as_string(): " << chord.as_string() << "\n";

  int sum = 0;
  chord.for_each_byte([&](char c) { sum += static_cast<unsigned char>(c); });
  std::cout << "chord ascii sum: " << sum << "\n";
  return 0;
}
