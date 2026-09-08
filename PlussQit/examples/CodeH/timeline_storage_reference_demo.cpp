#include <pluss/pluss.h>

#include <array>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <iterator>
#include <string_view>
#include <vector>

namespace {
void show(std::string_view label, const pluss::Timeline& value) {
  std::cout << "\n=== " << label << " ===\n" << pluss::timeline::print(value) << '\n';
}

void show(std::string_view label, const pluss::Series& value) {
  std::cout << "\n=== " << label << " ===\n" << pluss::series::print(value) << '\n';
}

void show_vector(std::string_view label, const std::vector<std::int64_t>& values) {
  std::cout << "\n=== " << label << " ===\n[";
  for (std::size_t i = 0; i < values.size(); ++i) {
    if (i) std::cout << ", ";
    std::cout << values[i];
  }
  std::cout << "]\n";
}
} // namespace

int main() {
  const auto instants = pluss::timeline::from_epoch(
      {{-1LL, 0LL}, {1'999'999'999LL, pluss::Timeline::kNaT}});
  show("instants", instants);
  show("epoch_nanos", instants.epoch_nanos());
  show("epoch_micros", instants.epoch_micros());
  show("epoch_millis", instants.epoch_millis());
  show("epoch_seconds", instants.epoch_seconds());
  show("epoch_days", instants.epoch_days());

  show("from_epoch_micros", pluss::Timeline::from_epoch_micros(
      pluss::make_series({-1LL, 0LL, 1'999'999LL})).epoch_nanos());
  show("from_epoch_millis", pluss::Timeline::from_epoch_millis(
      pluss::make_series({-1LL, 0LL, 1'999LL})).epoch_nanos());
  show("from_epoch_seconds", pluss::Timeline::from_epoch_seconds(
      pluss::make_series({{-1LL, 0LL}, {1LL, pluss::Timeline::kNaT}}),
      "America/New_York").epoch_nanos());

  const auto year = pluss::make_series(2024LL);
  const auto month = pluss::make_series({2LL, 3LL});
  show("from_ymd", pluss::Timeline::from_ymd(
      year, month, pluss::make_series({29LL, pluss::Timeline::kNaT}), "UTC"));
  show("from_ymdhms", pluss::Timeline::from_ymdhms(
      year, month, pluss::make_series({29LL, 1LL}),
      pluss::make_series({6LL, 7LL}), pluss::make_series(30LL),
      pluss::make_series(45LL), "UTC"));

  std::vector<std::int64_t> copied;
  instants.copy_ticks(std::back_inserter(copied));
  show_vector("copy_ticks", copied);

  bool released = false;
  {
    auto* raw = static_cast<std::int64_t*>(std::malloc(3 * sizeof(std::int64_t)));
    raw[0] = 0;
    raw[1] = 111;
    raw[2] = 2'000'000'000LL;
    const std::array<std::int64_t, 1> shape{2};
    const std::array<std::int64_t, 1> strides{2};
    const auto strided = pluss::Timeline::from_buffer(
        raw, shape, strides,
        [&](void* p) { released = true; std::free(p); }, "UTC");
    show("strided_buffer", strided);
    std::cout << "released while Timeline lives: " << std::boolalpha << released << '\n';
  }
  std::cout << "released after Timeline dies: " << std::boolalpha << released << '\n';

  const auto sorted = pluss::timeline::make({
      "2024-01-01T00:00:00Z", "2024-01-01T01:00:00Z",
  });
  const auto axis = sorted.to_timeindex();
  std::cout << "\n=== to_timeindex ===\nsize=" << axis.size()
            << ", increasing=" << axis.is_monotonic_increasing() << '\n';
}
