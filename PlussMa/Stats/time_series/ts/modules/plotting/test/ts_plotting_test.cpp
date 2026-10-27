#include <pluss/ma.h>

#include <pluss/pl/studio.h>

#include <cmath>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

using pluss::ScalarType;
using pluss::Series;
using pluss::subsp::stats::ts::AcfNorm;

void expect(bool condition, const std::string& message) {
  if (!condition) {
    throw std::runtime_error(message);
  }
}

Series make_ar1(std::int64_t n, double phi, std::uint64_t seed) {
  const auto cfg = pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);
  const auto eps = pluss::series::randn({n}, cfg, seed).host<double>();
  std::vector<double> y(static_cast<std::size_t>(n), 0.0);
  for (std::int64_t t = 1; t < n; ++t) {
    y[static_cast<std::size_t>(t)] = phi * y[static_cast<std::size_t>(t - 1)] + eps[static_cast<std::size_t>(t)];
  }
  return pluss::make_series(y, {n}, cfg);
}

void test_payload_shapes() {
  const auto cfg = pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);
  const Series y = make_ar1(3000, 0.65, 20260406ULL);

  pluss::subsp::stats::ts::plot::CorrPlotOptions opt;
  opt.max_lag = 20;
  opt.norm = AcfNorm::Unbiased;
  opt.include_zero_lag = false;
  const auto a = pluss::subsp::stats::ts::plot::acf_payload(y, opt, cfg);
  const auto p = pluss::subsp::stats::ts::plot::pacf_payload(y, opt, cfg);

  expect(a.values.numel() == 20, "acf_payload values size mismatch");
  expect(a.lags.numel() == 20, "acf_payload lags size mismatch");
  expect(p.values.numel() == 20, "pacf_payload values size mismatch");
  expect(p.lags.numel() == 20, "pacf_payload lags size mismatch");
  expect(std::isfinite(a.conf_low) && std::isfinite(a.conf_high), "acf_payload confidence band must be finite");
  expect(std::isfinite(p.conf_low) && std::isfinite(p.conf_high), "pacf_payload confidence band must be finite");

  const auto resid_payload = pluss::subsp::stats::ts::plot::residuals_payload(y, cfg);
  expect(resid_payload.index.numel() == y.numel(), "residuals_payload index size mismatch");
  expect(resid_payload.residuals.numel() == y.numel(), "residuals_payload residual size mismatch");
}

void test_flash_delegation() {
  const auto cfg = pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);
  const Series y = make_ar1(1024, 0.7, 20260407ULL);

  pluss::subsp::stats::ts::plot::CorrPlotOptions corr_opt;
  corr_opt.max_lag = 16;
  corr_opt.include_zero_lag = true;

  pluss::subsp::stats::ts::plot::ResidualPlotOptions resid_opt;

  pluss::pl::Studio acf_fig("ts-plot-acf", pluss::pl::Theme::paper());
  pluss::subsp::stats::ts::plot::acf(acf_fig, y, corr_opt, cfg);

  pluss::pl::Studio pacf_fig("ts-plot-pacf", pluss::pl::Theme::paper());
  pluss::subsp::stats::ts::plot::pacf(pacf_fig, y, corr_opt, cfg);

  pluss::pl::Studio resid_fig("ts-plot-resid", pluss::pl::Theme::paper());
  pluss::subsp::stats::ts::plot::residuals(resid_fig, y, resid_opt, cfg);
}

} // namespace

int main() {
  try {
    test_payload_shapes();
    test_flash_delegation();
    std::cout << "ts_plotting_test passed\n";
    return 0;
  } catch (const std::exception& e) {
    std::cerr << "ts_plotting_test failed: " << e.what() << '\n';
    return 1;
  }
}
