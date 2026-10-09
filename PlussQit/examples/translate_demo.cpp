#include <pluss/pluss.h>
#include <pluss/tils.h>

#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

namespace {

template <class T>
void print_vector(const std::string& label, const std::vector<T>& xs) {
  std::cout << label << ": [";
  for (std::size_t i = 0; i < xs.size(); ++i) {
    if (i) std::cout << ", ";
    std::cout << xs[i];
  }
  std::cout << "]\n";
}

} // namespace

int main() {
  std::cout << "=== pluss::tils::map_axis demo ===\n\n";

  auto s2d = pluss::make_series(std::vector<double>{1.0, 2.0, 3.0, 10.0, 20.0, 30.0}, {2, 3});
  auto row_means = pluss::tils::map_axis(
      [](const pluss::Series& row) {
        return row.mean().to_vec<double>(0);
      },
      s2d,
      1);
  print_vector("Series row means", row_means);

  auto note2d = pluss::make_note({{"alpha", "beta"}, {"gamma", "delta"}});
  auto row_char_counts = pluss::tils::map_axis(
      [](const pluss::Note& row) {
        std::int64_t total = 0;
        for (std::int64_t i = 0; i < row.numel(); ++i) {
          total += static_cast<std::int64_t>(row.at(i).size());
        }
        return total;
      },
      note2d,
      1);
  print_vector("Note row character totals", row_char_counts);

  auto timeline2d = pluss::timeline::make({
      {"2024-01-01T01:00:00Z", "2024-01-02T02:00:00Z"},
      {"2024-01-03T03:00:00Z", "2024-01-04T04:00:00Z"},
  });
  auto row_hour_totals = pluss::tils::map_axis(
      [](const pluss::codeh::Timeline& row) {
        const auto hours = row.hour().to_vec<std::int32_t>();
        std::int64_t total = 0;
        for (const auto h : hours) total += static_cast<std::int64_t>(h);
        return total;
      },
      timeline2d,
      1);
  print_vector("Timeline row hour totals", row_hour_totals);

  auto idx = pluss::timeline::make({
          "2024-01-01T00:00:00Z",
          "2024-01-02T00:00:00Z",
          "2024-01-03T00:00:00Z",
      }).index(true);
  const auto index_cardinality = pluss::tils::map_axis(
      [](const pluss::codeh::Timeline& as_timeline) {
        return as_timeline.numel();
      },
      idx.timeline(),
      0);
  std::cout << "Timeline index cardinality via map_axis: " << index_cardinality << "\n";

  pluss::tils::transmit_options par;
  par.exec = pluss::tils::transmit_exec::par;
  par.grain_size = 1;
  par.workers = 2;
  auto row_sums_parallel = pluss::tils::map_axis(
      [](const pluss::Series& row) {
        return row.sum().to_vec<double>(0);
      },
      s2d,
      1,
      par);
  print_vector("Series row sums (parallel)", row_sums_parallel);

  std::cout << "\nDone.\n";
  return 0;
}
