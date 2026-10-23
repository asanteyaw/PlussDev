// Plots for checking a model. `plot::residuals(studio, residuals)` draws the residuals against time (with a zero line, `ResidualPlotOptions::show_zero_line`), and the
// correlation plots of the residuals show whether any structure is left: bars outside the confidence band point to autocorrelation the model has not explained.
// `plot::residuals_payload` returns the plotted numbers (`index`, `residuals`). Here an AR(1) series is fitted by a model with no autoregressive term and by an AR(1);
// the program counts the residual autocorrelations outside the band, and saves the pictures.
#include <pluss/ma.h>
#include <pluss/pl/studio.h>

#include <cmath>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

namespace ts = pluss::subsp::stats::ts;
using pluss::Series;

const auto f64 = pluss::ScalarType::Float64;
Series vec(std::vector<double> v) { return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(f64); }
std::vector<double> noise(int n) {
  std::vector<double> e(static_cast<std::size_t>(n));
  for (int i = 0; i < n; ++i) {
    double u = std::sin(12.9898 * (i + 1.0) + 78.233) * 43758.5453;
    u -= std::floor(u);
    e[static_cast<std::size_t>(i)] = u - 0.5;
  }
  return e;
}
int main() {
  const int n = 300;
  const auto e = noise(n);
  std::vector<double> x(static_cast<std::size_t>(n));
  double carry = 0.0;
  for (int t = 0; t < n; ++t) {
    carry = 0.7 * carry + e[static_cast<std::size_t>(t)];
    x[static_cast<std::size_t>(t)] = carry;
  }
  const Series y = vec(x);
  const char* dir = std::getenv("PLUSSMA_GUIDE_ASSETS");

  std::cout << std::left << std::setw(24) << "model" << std::setw(28) << "lags outside band (of 20)" << "largest |autocorrelation|\n";
  for (int p = 0; p <= 1; ++p) {
    ts::ArimaOptions o;
    o.ar_order = p;
    const auto resid = ts::Arima::fit_mle(y, o).infer(y);

    ts::plot::CorrPlotOptions options;
    options.max_lag = 20;
    const auto a = ts::plot::acf_payload(resid, options);
    int outside = 0;
    double largest = 0.0;
    for (double v : a.values.to_vec<double>()) {
      if (std::abs(v) > a.conf_high) ++outside;
      largest = std::max(largest, std::abs(v));
    }
    std::cout << std::left << std::setw(24) << (p == 0 ? "no AR term" : "AR(1)") << std::setw(28) << outside << std::fixed << std::setprecision(3) << largest << "\n";

    if (dir != nullptr) {
      const std::string tag = p == 0 ? "ar0" : "ar1";
      pluss::pl::Studio figure("residual acf " + tag, pluss::pl::Theme::paper());
      ts::plot::acf(figure, resid, options);
      figure.save_svg(std::string(dir) + "/ts_resid_acf_" + tag + ".svg");
      if (p == 1) {
        pluss::pl::Studio series_figure("residuals", pluss::pl::Theme::paper());
        ts::plot::residuals(series_figure, resid);
        series_figure.save_svg(std::string(dir) + "/ts_residuals_ar1.svg");
      }
    }
  }
}
