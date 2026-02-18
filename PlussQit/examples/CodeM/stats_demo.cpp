#include <pluss/pluss.h>

#include <iostream>
#include <limits>
#include <vector>

int main() {
  let x = pluss::normal(0.0, 1.0, {20});
  std::cout << "x:\n" << x << "\n\n";

  std::cout << "mean:\n" << x.mean() << "\n\n";
  std::cout << "var(unbiased=false):\n" << x.var(false) << "\n\n";
  std::cout << "std(unbiased=false):\n" << x.std(false) << "\n\n";
  std::cout << "skewness:\n" << x.skewness() << "\n\n";
  std::cout << "kurtosis:\n" << x.kurtosis() << "\n\n";
  std::cout << "histcounts(8 bins, [-3, 3]):\n" << x.histcounts(8, -3.0, 3.0) << "\n\n";

  std::cout << "pdf(norm):\n" << x.pdf("norm") << "\n\n";
  std::cout << "cdf(norm):\n" << x.cdf("norm") << "\n\n";

  std::cout << "movmean(window=4):\n" << x.movmean(4) << "\n\n";
  std::cout << "movstd(window=4):\n" << x.movstd(4) << "\n\n";
  std::cout << "standardize:\n" << x.standardize() << "\n\n";
  std::cout << "center:\n" << x.center() << "\n\n";

  const double nan = std::numeric_limits<double>::quiet_NaN();
  let y = pluss::value(std::vector<double>{1.0, nan, 3.0, 5.0, nan, 7.0}, {6});
  std::cout << "y with NaNs:\n" << y << "\n\n";
  std::cout << "nanvar:\n" << y.nanvar() << "\n\n";
  std::cout << "nanstd:\n" << y.nanstd() << "\n";

  return 0;
}
