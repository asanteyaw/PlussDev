#include <pluss/pluss.h>

#include <cstdint>
#include <iostream>
#include <vector>

int main() {
  using namespace pluss::subsp::stat;

  const auto cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);
  const std::int64_t n = 2000;

  const auto eps = pluss::randn({n}, cfg, 20260318ULL).to_vec<double>();
  std::vector<double> y(static_cast<std::size_t>(n), 0.0);
  for (std::int64_t t = 1; t < n; ++t) {
    y[static_cast<std::size_t>(t)] = 0.65 * y[static_cast<std::size_t>(t - 1)] + eps[static_cast<std::size_t>(t)];
  }
  const pluss::Series ys = pluss::make_series(y, {n}, cfg);

  ts::AcfOptions aopt;
  aopt.max_lag = 12;
  aopt.norm = ts::AcfNorm::Biased;
  const pluss::Series a = ts::acf(ys, aopt, cfg);

  ts::PacfOptions popt;
  popt.max_lag = 12;
  const pluss::Series p = ts::pacf(ys, popt, cfg);

  ts::LjungBoxOptions lopt;
  lopt.lags = {6, 12, 24};
  lopt.model_df = 1;
  const auto lb = ts::ljung_box(ys, lopt, cfg);

  std::cout << "ACF\n" << a << "\n\n";
  std::cout << "PACF\n" << p << "\n\n";
  std::cout << "Ljung-Box lags\n" << lb.lags << "\n\n";
  std::cout << "Ljung-Box stat\n" << lb.stat << "\n\n";
  std::cout << "Ljung-Box pvalue\n" << lb.pvalue << "\n\n";
  std::cout << "Ljung-Box dof\n" << lb.dof << "\n";
  return 0;
}
