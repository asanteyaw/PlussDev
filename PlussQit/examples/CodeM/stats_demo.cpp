#include <pluss/pluss.h>
#include <pluss/tils.h>

#include <iostream>
#include <limits>
#include <vector>

int main() {
  auto x = pluss::series::normal(0.0, 1.0, {20});
  std::cout << "x:\n" << x << "\n\n";

  std::cout << "mean:\n" << x.mean() << "\n\n";
  std::cout << "stat().mean:\n" << x.stat().mean() << "\n\n";
  std::cout << "stat.mean (dot style):\n" << x.stat.mean() << "\n\n";
  std::cout << "var(unbiased=false):\n" << x.var(false) << "\n\n";
  std::cout << "std(unbiased=false):\n" << x.std(false) << "\n\n";
  std::cout << "skewness:\n" << x.stat.skewness() << "\n\n";
  std::cout << "kurtosis:\n" << x.stat.kurtosis() << "\n\n";
  std::cout << "histc(8 bins, [-3, 3]):\n" << x.histc(8, -3.0, 3.0) << "\n\n";

  std::cout << "stat.pdf():\n" << x.stat.pdf() << "\n\n";
  std::cout << "stat.logpdf():\n" << x.stat.logpdf() << "\n\n";
  std::cout << "stat.logcdf():\n" << x.stat.logcdf() << "\n\n";
  std::cout << "stat.cdf():\n" << x.stat.cdf() << "\n\n";

  std::cout << "movmean(window=4):\n" << x.stat.movmean(4) << "\n\n";
  std::cout << "movstd(window=4):\n" << x.stat.movstd(4) << "\n\n";
  std::cout << "standardize:\n" << x.standardize() << "\n\n";
  std::cout << "center:\n" << x.center() << "\n\n";

  auto a = pluss::series::normal(0.0, 1.0, {8, 3});
  auto b = pluss::series::normal(0.0, 1.0, {5, 3});
  std::cout << "stat(peer).cdist:\n" << a.stat(b).cdist() << "\n\n";

  const double nan = std::numeric_limits<double>::quiet_NaN();
  auto y = pluss::make_series(std::vector<double>{1.0, nan, 3.0, 5.0, nan, 7.0}, {6});
  std::cout << "y with NaNs:\n" << y << "\n\n";
  std::cout << "nanvar:\n" << y.nanvar() << "\n\n";
  std::cout << "stat.nanvar:\n" << y.stat.nanvar() << "\n\n";
  std::cout << "nanstd:\n" << y.nanstd() << "\n";

  return 0;
}
