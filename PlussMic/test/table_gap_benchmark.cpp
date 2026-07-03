#include <algorithm>
#include <chrono>
#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

#include "pluss/pluss.h"

namespace {

using pluss::table::DataFrame;
using pluss::table::Thread;
using Clock = std::chrono::steady_clock;

template <class Fn>
double elapsed_ms(Fn&& fn) {
  const auto started = Clock::now();
  fn();
  return std::chrono::duration<double, std::milli>(Clock::now() - started).count();
}

DataFrame make_frame(std::int64_t rows, std::int64_t case_offset = 0) {
  std::vector<std::int64_t> cases(static_cast<std::size_t>(rows));
  std::vector<std::int64_t> keys(static_cast<std::size_t>(rows));
  std::vector<double> x(static_cast<std::size_t>(rows));
  std::vector<double> y(static_cast<std::size_t>(rows));
  for (std::int64_t i = 0; i < rows; ++i) {
    cases[static_cast<std::size_t>(i)] = i + case_offset;
    keys[static_cast<std::size_t>(i)] = i % 1000;
    x[static_cast<std::size_t>(i)] = static_cast<double>(i % 997);
    y[static_cast<std::size_t>(i)] = static_cast<double>(i % 991);
  }
  return DataFrame(
      {
          {"key", Thread(pluss::make_series(keys))},
          {"x", Thread(pluss::make_series(x))},
          {"y", Thread(pluss::make_series(y))},
      },
      Thread(pluss::make_series(cases)));
}

} // namespace

int main(int argc, char** argv) {
  const std::int64_t rows =
      argc > 1 ? std::max<std::int64_t>(1000, std::stoll(argv[1])) : 100000;
  const auto left = make_frame(rows);
  const auto right = make_frame(rows, rows / 2);
  const auto join_rows = std::min<std::int64_t>(rows, 100000);
  const auto join_left = left.slice(0, join_rows);
  const auto join_right = right.slice(0, join_rows);
  volatile std::int64_t sink = 0;

  const auto alignment = elapsed_ms([&] {
    const auto result = left.add(right);
    sink += result.ncases();
  });
  const auto grouping = elapsed_ms([&] {
    const auto result = left.group_by(
        {"key"},
        {{"x_sum", "x", DataFrame::GroupOp::Sum}},
        true,
        false);
    sink += result.ncases();
  });
  const auto unification = elapsed_ms([&] {
    const auto result = join_left.unify("key", join_right, "key", "common");
    sink += result.ncases();
  });
  const auto reshaping = elapsed_ms([&] {
    const auto result = left.lengthen({"key"}, {"x", "y"});
    sink += result.ncases();
  });
  const auto windows = elapsed_ms([&] {
    const auto result = left.moving_mean(128);
    sink += result.ncases();
  });
  const auto statistics = elapsed_ms([&] {
    const auto result = left.covariance();
    sink += result.ncases();
  });

  std::cout << "{\"rows\":" << rows
            << ",\"join_rows\":" << join_rows
            << ",\"alignment_ms\":" << alignment
            << ",\"grouping_ms\":" << grouping
            << ",\"unification_ms\":" << unification
            << ",\"reshaping_ms\":" << reshaping
            << ",\"windows_ms\":" << windows
            << ",\"statistics_ms\":" << statistics
            << ",\"sink\":" << sink << "}\n";
}
