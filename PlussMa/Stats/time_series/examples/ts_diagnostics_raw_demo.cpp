#include <pluss/ma.h>

#include <cstdint>
#include <iostream>
#include <vector>

int main() {
  using namespace pluss::subsp::stats;

  const auto cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);
  const std::int64_t n = 2000;

  const auto eps = pluss::randn({n}, cfg, 20260318ULL).to_vec<double>();
  std::vector<double> y(static_cast<std::size_t>(n), 0.0);
  std::vector<double> rw(static_cast<std::size_t>(n), 0.0);
  for (std::int64_t t = 1; t < n; ++t) {
    y[static_cast<std::size_t>(t)] = 0.65 * y[static_cast<std::size_t>(t - 1)] + eps[static_cast<std::size_t>(t)];
    rw[static_cast<std::size_t>(t)] = rw[static_cast<std::size_t>(t - 1)] + eps[static_cast<std::size_t>(t)];
  }
  const pluss::Series ys = pluss::make_series(y, {n}, cfg);
  const pluss::Series yrw = pluss::make_series(rw, {n}, cfg);

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

  ts::ArchLmOptions arch_opt;
  arch_opt.lags = 8;
  const auto arch_lm = ts::arch_lm(ys, arch_opt, cfg);

  ts::AdfOptions adf_opt;
  adf_opt.lags = 1;
  adf_opt.model = ts::AdfModel::ARD;
  const auto adf_y = ts::adf_test(ys, adf_opt, cfg);
  const auto adf_rw = ts::adf_test(yrw, adf_opt, cfg);

  ts::KpssOptions kpss_opt;
  kpss_opt.lags = 8;
  kpss_opt.trend = false;
  const auto kpss_y = ts::kpss_test(ys, kpss_opt, cfg);
  const auto kpss_rw = ts::kpss_test(yrw, kpss_opt, cfg);

  std::cout << "ACF\n" << a << "\n\n";
  std::cout << "PACF\n" << p << "\n\n";
  std::cout << "Ljung-Box lags\n" << lb.lags << "\n\n";
  std::cout << "Ljung-Box stat\n" << lb.stat << "\n\n";
  std::cout << "Ljung-Box pvalue\n" << lb.pvalue << "\n\n";
  std::cout << "Ljung-Box dof\n" << lb.dof << "\n";
  std::cout << "\nARCH-LM\n";
  std::cout << "stat=" << arch_lm.stat << " pvalue=" << arch_lm.pvalue
            << " critical=" << arch_lm.critical << " reject=" << arch_lm.reject
            << " dof=" << arch_lm.dof << " effective_n=" << arch_lm.effective_n << "\n";
  std::cout << "\nADF (AR series)\n";
  std::cout << "stat=" << adf_y.stat << " pvalue=" << adf_y.pvalue
            << " critical=" << adf_y.critical << " reject=" << adf_y.reject << "\n";
  std::cout << "ADF (random walk)\n";
  std::cout << "stat=" << adf_rw.stat << " pvalue=" << adf_rw.pvalue
            << " critical=" << adf_rw.critical << " reject=" << adf_rw.reject << "\n";
  std::cout << "\nKPSS (AR series)\n";
  std::cout << "stat=" << kpss_y.stat << " pvalue=" << kpss_y.pvalue
            << " critical=" << kpss_y.critical << " reject=" << kpss_y.reject
            << " long_run_var=" << kpss_y.long_run_var << "\n";
  std::cout << "KPSS (random walk)\n";
  std::cout << "stat=" << kpss_rw.stat << " pvalue=" << kpss_rw.pvalue
            << " critical=" << kpss_rw.critical << " reject=" << kpss_rw.reject
            << " long_run_var=" << kpss_rw.long_run_var << "\n";
  return 0;
}
