// Turning each column of a region-by-month table into shares of that month's total. This is a
// common step before comparing regions fairly, because a big month would otherwise dominate.
#include <pluss/mic.h>
#include <pluss/tils/kit/translate.h>

#include <cstddef>
#include <iostream>
#include <iomanip>
#include <vector>

int main() {
  const auto cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);
  // Rows are regions north, south, east. Columns are January, February.
  const auto revenue = pluss::make_series(std::vector<double>{60, 20, 40, 30, 20, 50}, {3, 2}, cfg);

  const auto shares = pluss::tils::map_axis(
      [](const pluss::Series& month) {
        const auto values = month.host<double>();
        double total = 0.0;
        for (double v : values) total += v;
        std::vector<double> out;
        for (double v : values) out.push_back(v / total);
        return out;
      },
      revenue, 0);

  std::cout << std::fixed << std::setprecision(2);
  const char* regions[] = {"north", "south", "east"};
  for (std::size_t r = 0; r < 3; ++r) {
    std::cout << regions[r] << ": january " << shares[0][r] << ", february " << shares[1][r] << '\n';
  }
  return 0;
}
