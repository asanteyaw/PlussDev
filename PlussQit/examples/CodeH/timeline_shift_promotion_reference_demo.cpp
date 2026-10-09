#include <pluss/pluss.h>

#include <array>
#include <cstdint>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

namespace {
void show(std::string_view label, const pluss::Timeline& value) {
  std::cout << "\n=== " << label << " ===\n"
            << pluss::timeline::print(value) << '\n';
}

template <class Fn>
void show_rejection(std::string_view label, Fn&& fn) {
  try {
    fn();
  } catch (const pluss::Error&) {
    std::cout << "\n=== " << label << " ===\nrejected\n";
  }
}
} // namespace

int main() {
  const auto leap = pluss::timeline::make("2024-02-29T08:15:30Z");
  show("shift_year", leap.shift(1, "year"));
  show("shift_month", leap.shift(1, "month"));
  show("shift_day", leap.shift(1, "day"));
  show("shift_hour", leap.shift(1, "hour"));
  show("shift_minute", leap.shift(1, "minute"));
  show("shift_second", leap.shift(1, "second"));
  show("shift_negative_case_insensitive", leap.shift(-1, "DAY"));
  show("shift_zero", leap.shift(0, "month"));

  const auto before = pluss::timeline::make("2024-03-09T10:30:00Z")
                          .tz_convert("America/Edmonton");
  show("shift_civil_day", before.shift(1, "day"));
  show("shift_elapsed_hours", before.shift(24, "hour"));

  const auto matrix = pluss::timeline::make({
      {"2024-01-01T00:00:00Z", ""},
      {"2024-01-03T00:00:00Z", "2024-01-04T00:00:00Z"},
  }).tz_convert("America/Edmonton");
  show("shift_matrix_absent", matrix.shift(2, "hour"));
  show_rejection("shift_alias", [&] { (void)leap.shift(1, "years"); });
  show_rejection("shift_whitespace", [&] { (void)leap.shift(1, " day "); });

  const auto sorted = pluss::timeline::make({
      "2024-01-01T00:00:00Z", "2024-01-01T00:00:00Z",
      "2024-01-03T00:00:00Z"});
  const auto index = sorted.index();
  std::cout << "\n=== timeline_index_sorted_duplicates ===\n"
            << "size=" << index.size()
            << " increasing=" << std::boolalpha << index.is_monotonic_increasing()
            << " unique=" << index.is_unique() << '\n';

  const auto empty = pluss::timeline::epoch({0}).index();
  std::cout << "\n=== timeline_index_empty ===\n"
            << "size=" << empty.size()
            << " increasing=" << empty.is_monotonic_increasing() << '\n';

  const auto unsorted = pluss::timeline::make({
      "2024-01-03T00:00:00Z", "2024-01-01T00:00:00Z"});
  show_rejection("timeline_index_unsorted_default", [&] { (void)unsorted.index(); });
  const auto unchecked = unsorted.index(false);
  std::cout << "\n=== timeline_index_unsorted_allowed ===\n"
            << "increasing=" << unchecked.is_monotonic_increasing() << '\n';

  std::array<std::int64_t, 5> backing{
      1'704'067'200'000'000'000LL, 99,
      1'704'153'600'000'000'000LL, 88,
      1'704'240'000'000'000'000LL};
  const std::array<std::int64_t, 1> shape{3};
  const std::array<std::int64_t, 1> strides{2};
  const auto strided = pluss::Timeline::from_buffer(
      backing.data(), shape, strides, {}, "America/Edmonton");
  show("timeline_index_strided_round_trip", strided.index().timeline());

  show_rejection("timeline_index_nat", [&] {
    (void)pluss::timeline::make({"2024-01-01", ""}).index(false);
  });
  show_rejection("timeline_index_scalar", [&] {
    (void)pluss::timeline::date("2024-01-01").index();
  });
  show_rejection("timeline_index_matrix", [&] {
    (void)pluss::timeline::date(
        std::vector<std::string>{"2024-01-01"}, {1, 1})
        .index();
  });
}
