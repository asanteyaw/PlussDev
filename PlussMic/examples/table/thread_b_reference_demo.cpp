#include <algorithm>
#include <cstdint>
#include <iostream>
#include <limits>
#include <vector>

#include <pluss/pluss.h>

int main() {
  using pluss::make_note;
  using pluss::make_series;
  using pluss::make_timeindex;
  using pluss::make_timeline;
  using pluss::table::Thread;
  using pluss::table::WindowOptions;
  using pluss::table::WindowStatistic;

  auto missing = Thread(make_series({
      10.0,
      std::numeric_limits<double>::quiet_NaN(),
      std::numeric_limits<double>::quiet_NaN(),
      40.0,
      50.0,
  }));

  auto scores = Thread(make_series({10.0, 20.0, 30.0, 40.0, 50.0}));
  auto names = Thread(make_note({"Accra", "Kumasi", "Tema"}));
  auto dates = Thread(make_timeline({
      "2024-01-01T00:00:00Z",
      "2024-01-02T00:00:00Z",
      "2024-01-03T00:00:00Z",
  }));
  auto ticks = Thread(make_timeindex({
      "2024-01-01T00:10:00Z",
      "2024-01-01T12:00:00Z",
      "2024-01-02T09:00:00Z",
      "2024-01-03T18:00:00Z",
  }, true));
  auto lower = Thread(make_series({10.0, 20.0, 35.0, 35.0, 45.0}));
  auto upper = Thread(make_series({15.0, 25.0, 35.0, 45.0, 50.0}));

  std::cout << "BFILL_LIMITLESS\n"
            << missing.bfill().to_string() << "\n\n";

  std::cout << "BFILL_LIMIT_1\n"
            << missing.bfill(1).to_string() << "\n\n";

  auto expanding_mean = scores.bounded(
      [](std::int64_t size, const WindowOptions&) {
        std::vector<std::int64_t> starts(static_cast<std::size_t>(size), 0);
        std::vector<std::int64_t> ends;
        ends.reserve(static_cast<std::size_t>(size));
        for (std::int64_t row = 0; row < size; ++row) {
          ends.push_back(row + 1);
        }
        return std::pair{std::move(starts), std::move(ends)};
      },
      WindowStatistic::Mean);
  std::cout << "BOUNDED_EXPANDING_MEAN\n"
            << expanding_mean.to_string() << "\n\n";

  auto trailing_two_mean = scores.bounded(
      [](std::int64_t size, const WindowOptions&) {
        std::vector<std::int64_t> starts;
        std::vector<std::int64_t> ends;
        starts.reserve(static_cast<std::size_t>(size));
        ends.reserve(static_cast<std::size_t>(size));
        for (std::int64_t row = 0; row < size; ++row) {
          starts.push_back(std::max<std::int64_t>(0, row - 1));
          ends.push_back(row + 1);
        }
        return std::pair{std::move(starts), std::move(ends)};
      },
      WindowStatistic::Mean,
      WindowOptions{.min_count = 2});
  std::cout << "BOUNDED_TRAILING_TWO\n"
            << trailing_two_mean.to_string() << "\n\n";

  std::cout << "BTW_NUMERIC_SHARED\n"
            << scores.btw(20.0, 40.0, true).to_string() << "\n\n";

  std::cout << "BTW_NUMERIC_ASYMMETRIC\n"
            << scores.btw(20.0, 40.0, false, true).to_string() << "\n\n";

  std::cout << "BTW_TEMPORAL_SHARED\n"
            << dates.btw("2024-01-01T00:00:00Z", "2024-01-02T00:00:00Z", true).to_string() << "\n\n";

  std::cout << "BTW_TEMPORAL_ASYMMETRIC\n"
            << dates.btw("2024-01-01T00:00:00Z", "2024-01-03T00:00:00Z", false, true).to_string() << "\n\n";

  std::cout << "BTW_ROWWISE_SHARED\n"
            << scores.btw(lower, upper, true).to_string() << "\n\n";

  std::cout << "BTW_ROWWISE_ASYMMETRIC\n"
            << scores.btw(lower, upper, false, true).to_string() << "\n\n";

  std::cout << "BUCKETIZE_AUTO_LABELS\n"
            << scores.bucketize({0.0, 20.0, 40.0, 60.0}).to_string() << "\n\n";

  std::cout << "BUCKETIZE_EXPLICIT_LABELS\n"
            << scores.bucketize({0.0, 20.0, 40.0, 60.0}, {10, 20, 30}, true, true).to_string() << "\n";

  return 0;
}
