#include <iostream>

#include <pluss/pluss.h>
#include <pluss/ma.h>

int main() {
  const pluss::Series x = pluss::make_series({0.5});
  const pluss::subsp::stats::distr::Normal dist{0.0, 1.0};

  std::cout << dist.logpdf(x).item<double>() << '\n';
  return 0;
}
