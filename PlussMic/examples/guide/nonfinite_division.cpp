// Where NaN and infinity come from: dividing by zero. The same answers on a bare Series, on a Shelf (a column) and on a table column.
#include <pluss/pluss.h>

#include <iostream>

int main() {
  // Floating point: x / 0 is +inf for a positive x, -inf for a negative x, and NaN for 0 / 0.
  const auto top = pluss::make_series({6.0, -6.0, 0.0, 6.0});
  const auto bottom = pluss::make_series({0.0, 0.0, 0.0, 3.0});
  std::cout << "float   x / y   " << (top / bottom) << "\n";

  // Integers: with no zero divisor the answer stays an integer ...
  const auto count = pluss::make_series(std::vector<std::int64_t>{7, -7, 0, 5});
  const auto by = pluss::make_series(std::vector<std::int64_t>{2, 2, 2, 5});
  std::cout << "int     x // y  " << count.floor_divide(by) << "\n";

  // ... and with a zero divisor the same call returns +inf, -inf and NaN instead of raising an error, so the result is a float Series.
  const auto zero_by = pluss::make_series(std::vector<std::int64_t>{0, 0, 0, 3});
  std::cout << "int     x // y  " << count.floor_divide(zero_by) << "\n";

  // The remainder by zero is NaN for every dividend.
  std::cout << "int     x % y   " << count.remainder(zero_by) << "\n";
}
