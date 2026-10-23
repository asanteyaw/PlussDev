// Forecasting. `fit.forecast(h, y)` returns the next h values given the observed series y (the model is applied to the history, so the last
// observations and the last residuals start the recursion). For a stationary model the forecasts approach the mean as the horizon grows; for an AR(1)
// the h-step forecast is mu + phi^h (x_n - mu), which is computed by hand below as a check. Here: the AR(1) series, forecast 8 steps ahead.
#include <pluss/ma.h>

#include <cmath>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <vector>

namespace ts = pluss::subsp::stats::ts;
using pluss::Series;

const auto f64 = pluss::ScalarType::Float64;
Series vec(std::vector<double> v) { return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(f64); }
// A reproducible pseudo-random sequence in (-0.5, 0.5): e_t = frac(sin(12.9898 t + 78.233) * 43758.5453) - 0.5, t = 1, 2, ...
std::vector<double> noise(int n) {
  std::vector<double> e(static_cast<std::size_t>(n));
  for (int i = 0; i < n; ++i) {
    double u = std::sin(12.9898 * (i + 1.0) + 78.233) * 43758.5453;
    u -= std::floor(u);
    e[static_cast<std::size_t>(i)] = u - 0.5;
  }
  return e;
}
// AR(1): x_t = phi x_{t-1} + e_t
std::vector<double> ar1(int n, double phi) {
  auto e = noise(n);
  std::vector<double> x(static_cast<std::size_t>(n));
  double carry = 0.0;
  for (int i = 0; i < n; ++i) { carry = phi * carry + e[static_cast<std::size_t>(i)]; x[static_cast<std::size_t>(i)] = carry; }
  return x;
}
std::vector<double> running_sum(const std::vector<double>& e) {
  std::vector<double> w(e.size());
  double s = 0.0;
  for (std::size_t i = 0; i < e.size(); ++i) { s += e[i]; w[i] = s; }
  return w;
}
void print_row(const char* label, const Series& s, int precision = 4) {
  std::cout << std::left << std::setw(16) << label << std::fixed << std::setprecision(precision);
  for (double v : s.to_vec<double>()) std::cout << " " << std::setw(8) << v;
  std::cout << "\n";
}
int main() {
  const auto data = ar1(200, 0.7);
  const Series y = vec(data);
  ts::ArimaOptions o;
  o.ar_order = 1;
  const auto fit = ts::Arima::fit_mle(y, o);
  const auto fc = fit.forecast(8, y).to_vec<double>();
  const double phi = fit.ar[0];
  const double mu = fit.mean + fit.intercept / (1.0 - phi);  // the level the series reverts to
  std::cout << std::fixed << std::setprecision(4) << "phi = " << phi << ", mean mu = " << mu << ", last observation x_n = " << data.back() << "\n\n";
  std::cout << std::left << std::setw(8) << "step" << std::setw(14) << "forecast" << std::setw(26) << "mu + phi^h (x_n - mu)" << "difference\n";
  for (int h = 1; h <= 8; ++h) {
    const double by_hand = mu + std::pow(phi, h) * (data.back() - mu);
    std::cout << std::left << std::setw(8) << h << std::setw(14) << fc[static_cast<std::size_t>(h - 1)] << std::setw(26) << by_hand << std::scientific << std::setprecision(1) << std::abs(fc[static_cast<std::size_t>(h - 1)] - by_hand)
              << std::fixed << std::setprecision(4) << "\n";
  }
}
