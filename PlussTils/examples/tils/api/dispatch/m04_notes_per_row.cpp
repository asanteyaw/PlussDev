// Working with text. A grid of region names is cut into rows, and each row is a note of its own, so a
// function can count or join the names that belong together.
#include <pluss/mic.h>
#include <pluss/tils/kit/translate.h>

#include <iostream>
#include <string>
#include <vector>

int main() {
  const auto regions = pluss::make_note(
      std::vector<std::string>{"north", "south", "east", "west"}, {2, 2});

  const auto counts = pluss::tils::map_axis(
      [](const pluss::Note& row) { return static_cast<std::int64_t>(row.numel()); }, regions, 1);

  for (std::size_t r = 0; r < counts.size(); ++r) {
    std::cout << "row " << r << " holds " << counts[r] << " names\n";
  }
  return 0;
}
