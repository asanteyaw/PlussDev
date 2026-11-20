// Series::where: every overload and rule, with the result printed. Each block between the "### n" markers is one documented example.
#include <pluss/pluss.h>

#include <cmath>
#include <iostream>
#include <vector>

int main() {
  auto x = pluss::make_series({1.0, 2.0, 3.0, 4.0});
  auto big = x.gt(2.5);

  std::cout << "### 1\n";
  std::cout << x.where(big, pluss::make_series({10.0, 20.0, 30.0, 40.0})) << "\n";

  std::cout << "### 2\n";
  std::cout << x.where(big, 0.0) << "\n";

  std::cout << "### 3\n";
  std::cout << pluss::series::where(big, x, 0.0) << "\n";
  std::cout << pluss::series::where(big, 1.0, 0.0) << "\n";

  std::cout << "### 4\n";
  std::cout << x.where(pluss::make_series({1.0, 0.0, 1.0, 0.0}), -1.0) << "\n";

  std::cout << "### 5\n";
  auto m = pluss::make_series({1.0, 2.0, 3.0, 4.0, 5.0, 6.0}).reshape({3, 2});
  auto rows = pluss::make_series({true, false, true}).reshape({3, 1});
  std::cout << m.where(rows, 0.0) << "\n";

  std::cout << "### 6\n";
  auto counts = pluss::make_series(std::vector<std::int64_t>{1, 2, 3, 4});
  std::cout << counts.where(counts.gt(2), 0.0) << "\n";
  std::cout << counts.where(counts.gt(2), 0.5) << "\n";

  std::cout << "### 7\n";
  auto y = pluss::make_series({1.0, 2.0, 3.0, 4.0});
  y.where_(y.gt(2.5), 0.0);
  std::cout << y << "\n";

  std::cout << "### 8\n";
  auto with_nan = pluss::make_series({1.0, std::nan(""), 3.0});
  std::cout << with_nan.where(with_nan.gt(0.0), -1.0) << "\n";
  std::cout << with_nan.where(with_nan.isnan().logical_not(), -1.0) << "\n";

  std::cout << "### 9\n";
  try {
    x.where(pluss::make_series({true, false, true, false, true}), 0.0);
  } catch (const pluss::Error &e) {
    std::cout << "five flags for four values: " << e.message() << "\n";
  }
}
