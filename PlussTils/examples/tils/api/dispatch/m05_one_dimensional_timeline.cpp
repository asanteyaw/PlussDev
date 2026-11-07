// A one-dimensional timeline has a single slice along its only axis, which holds every tick. The
// function receives all the timestamps at once, so it can find the span they cover.
#include <pluss/mic.h>
#include <pluss/tils/auxiliary/translate.h>

#include <iostream>
#include <vector>

int main() {
  const auto stamps = pluss::make_timeline({"2024-01-01", "2024-01-02", "2024-01-03", "2024-01-04"});

  const auto counts = pluss::tils::map_axis(
      [](const pluss::Timeline& slice) { return static_cast<std::int64_t>(slice.numel()); }, stamps, 0);

  std::cout << "slices: " << counts.size() << '\n';
  std::cout << "timestamps in the slice: " << counts.front() << '\n';
  return 0;
}
