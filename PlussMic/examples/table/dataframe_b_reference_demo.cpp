#include <algorithm>
#include <cstdint>
#include <iostream>
#include <limits>
#include <string_view>
#include <vector>

#include <pluss/pluss.h>

int main() {
  using pluss::make_note;
  using pluss::make_series;
  using pluss::table::DataFrame;
  using pluss::table::WindowOptions;
  using pluss::table::WindowStatistic;

  auto missing = DataFrame({
      {"score", make_series({72.0, std::numeric_limits<double>::quiet_NaN(), 81.0,
                             std::numeric_limits<double>::quiet_NaN(), 95.0})},
      {"cost", make_series({10.0, std::numeric_limits<double>::quiet_NaN(),
                            std::numeric_limits<double>::quiet_NaN(), 40.0,
                            std::numeric_limits<double>::quiet_NaN()})},
      {"city", make_note({"Accra", "Tema", "Kumasi", "Tamale", "Sunyani"})},
  }).title("missing");

  auto grades = DataFrame({
      {"student", make_note({"Ada", "Bola", "Chen", "Dede"})},
      {"assignment", make_series({9.5, 8.0, 7.5, 9.0})},
      {"final", make_series({28.0, 24.0, 21.5, 26.0})},
  }, make_note({"s1", "s2", "s3", "s4"})).title("grades");

  auto metrics = DataFrame({
      {"sales", make_series({12.0, 27.0, 35.0, 49.0})},
      {"cost", make_series({5.0, 18.0, 33.0, 60.0})},
      {"city", make_note({"Accra", "Tema", "Kumasi", "Tamale"})},
  }).title("metrics");
  std::array<std::string_view, 2> pair_features{"sales", "cost"};

  auto numeric = metrics.features({"sales", "cost"});

  std::cout << "BFILL_LIMITLESS\n"
            << missing.bfill().to_string() << "\n\n";

  DataFrame::FillOptions fill_options;
  fill_options.features = {"score"};
  fill_options.limit = 1;
  std::cout << "BFILL_OPTIONS\n"
            << missing.bfill(fill_options).to_string() << "\n\n";

  std::cout << "BOTTOM\n"
            << grades.bottom("final", 2).to_string() << "\n\n";

  auto expanding_mean = numeric.bounded(
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

  auto trailing_two_mean = numeric.bounded(
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

  std::cout << "BUCKETIZE_ALL\n"
            << metrics.bucketize({0.0, 20.0, 40.0, 60.0}).to_string() << "\n\n";

  std::cout << "BUCKETIZE_ONE\n"
            << metrics.bucketize("sales", {0.0, 20.0, 40.0, 60.0}, {10, 20, 30}).to_string() << "\n\n";

  std::cout << "BUCKETIZE_SUBSET\n"
            << metrics.bucketize(pair_features, {0.0, 25.0, 50.0, 75.0}, {1, 2, 3}).to_string() << "\n";

  return 0;
}
