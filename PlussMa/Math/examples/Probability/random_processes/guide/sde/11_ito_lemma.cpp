// Ito's lemma says how a smooth function g of an Ito process X, dX = mu dt + sigma dW, moves: unlike ordinary
// calculus there is a second-order term, because dW^2 = dt. The differential of g(X) has drift
// g'(x) mu + 1/2 g''(x) sigma^2 and diffusion g'(x) sigma. `x.calc.diff.ito(g, mu, sigma)` computes both by
// automatic differentiation, at every entry of x at once. The three functions below are checked against their
// closed forms.
#include <pluss/ma.h>

#include <cmath>
#include <functional>
#include <iomanip>
#include <iostream>
#include <vector>

int main() {
  const auto f64 = pluss::ScalarType::Float64;
  auto vec = [&](std::vector<double> v) {
    return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(f64);
  };
  const std::vector<double> xv = {0.5, 1.0, -0.3}, mv = {0.1, 0.2, -0.1}, sv = {0.3, 0.5, 0.2};
  const pluss::Series x = vec(xv), mu = vec(mv), sigma = vec(sv);

  struct Row {
    const char* name;
    std::function<pluss::Series(const pluss::Series&)> g;
    std::function<double(double, double, double)> drift;      // closed form
    std::function<double(double, double, double)> diffusion;  // closed form
  };
  const std::vector<Row> rows = {
      {"x^2", [](const pluss::Series& s) { return s.mul(s); },
       [](double x, double m, double s) { return 2 * x * m + s * s; }, [](double x, double, double s) { return 2 * x * s; }},
      {"exp(x)", [](const pluss::Series& s) { return s.exp(); },
       [](double x, double m, double s) { return std::exp(x) * (m + 0.5 * s * s); },
       [](double x, double, double s) { return std::exp(x) * s; }},
      {"sin(x)", [](const pluss::Series& s) { return s.sin(); },
       [](double x, double m, double s) { return std::cos(x) * m - 0.5 * std::sin(x) * s * s; },
       [](double x, double, double s) { return std::cos(x) * s; }}};

  std::cout << std::fixed << std::setprecision(5);
  for (const auto& row : rows) {
    const auto d = x.calc.diff.ito(row.g, mu, sigma);
    const auto drift = d.drift.to_vec<double>();
    const auto diffusion = d.diffusion.to_vec<double>();
    double worst = 0.0;
    std::cout << "g(x) = " << row.name << "\n";
    for (std::size_t i = 0; i < xv.size(); ++i) {
      const double want_drift = row.drift(xv[i], mv[i], sv[i]);
      const double want_diffusion = row.diffusion(xv[i], mv[i], sv[i]);
      worst = std::max({worst, std::abs(drift[i] - want_drift), std::abs(diffusion[i] - want_diffusion)});
      std::cout << "  x = " << std::setw(5) << xv[i] << "   drift " << std::setw(8) << drift[i] << "   diffusion "
                << std::setw(8) << diffusion[i] << "\n";
    }
    std::cout << "  largest difference from the closed form: " << std::scientific << std::setprecision(1) << worst
              << std::fixed << std::setprecision(5) << "\n";
  }
  return 0;
}
