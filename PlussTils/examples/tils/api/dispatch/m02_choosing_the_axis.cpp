// The same table can be cut two ways. Along the last axis each slice is a row, and along the first
// axis each slice is a column. A negative axis counts from the end, so -1 is the last axis.
#include <pluss/mic.h>
#include <pluss/tils/kit/translate.h>

#include <iostream>
#include <vector>

double sum_of(const pluss::Series& slice) {
  double total = 0.0;
  for (double v : slice.host<double>()) total += v;
  return total;
}

int main() {
  const auto cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);
  const auto table = pluss::make_series(std::vector<double>{1, 2, 3, 4, 5, 6}, {2, 3}, cfg);

  const auto row_sums = pluss::tils::map_axis([](const pluss::Series& s) { return sum_of(s); }, table, -1);
  const auto column_sums = pluss::tils::map_axis([](const pluss::Series& s) { return sum_of(s); }, table, 0);

  std::cout << "row sums (axis -1):";
  for (double v : row_sums) std::cout << ' ' << v;
  std::cout << '\n';
  std::cout << "column sums (axis 0):";
  for (double v : column_sums) std::cout << ' ' << v;
  std::cout << '\n';
  return 0;
}
