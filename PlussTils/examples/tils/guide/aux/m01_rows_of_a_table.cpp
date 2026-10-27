// Summing each row of a small table. The table holds three weeks of sales for two stores. map_axis
// hands the function one row at a time, so the function sees a single store's week as a vector.
#include <pluss/mic.h>
#include <pluss/tils/aux/translate.h>

#include <iostream>
#include <vector>

int main() {
  const auto cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);
  const auto sales = pluss::make_series(std::vector<double>{10, 20, 30, 5, 15, 25}, {2, 3}, cfg);

  const auto totals = pluss::tils::map_axis(
      [](const pluss::Series& row) {
        double sum = 0.0;
        for (double v : row.host<double>()) sum += v;
        return sum;
      },
      sales, 1);

  for (std::size_t store = 0; store < totals.size(); ++store) {
    std::cout << "store " << store + 1 << " total: " << totals[store] << '\n';
  }
  return 0;
}
