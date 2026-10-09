// Summing many columns at once across threads. The parallel result must match the sequential one,
// so the program compares the two before trusting the parallel answer.
#include <pluss/mic.h>
#include <pluss/tils/aux/translate.h>

#include <cstddef>
#include <iostream>
#include <vector>

int main() {
  const std::int64_t rows = 2000;
  const std::int64_t cols = 64;
  std::vector<double> values(static_cast<std::size_t>(rows * cols));
  for (std::size_t i = 0; i < values.size(); ++i) values[i] = static_cast<double>(i % 7);
  const auto cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);
  const auto table = pluss::make_series(values, {rows, cols}, cfg);

  const auto column_sum = [](const pluss::Series& s) {
    double total = 0.0;
    for (double v : s.to_vec<double>()) total += v;
    return total;
  };

  const auto sequential = pluss::tils::map_axis(column_sum, table, 0);
  const auto parallel = pluss::tils::map_axis(
      column_sum, table, 0,
      pluss::tils::transmit_options{.exec = pluss::tils::transmit_exec::par, .grain_size = 8, .workers = 4});

  std::cout << "column sums: " << parallel.size() << '\n';
  std::cout << "parallel equals sequential: " << (parallel == sequential) << '\n';
  return 0;
}
