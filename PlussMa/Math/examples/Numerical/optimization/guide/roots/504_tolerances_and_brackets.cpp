// Stopping and brackets. `Options` is {max_iterations, x_tolerance, f_tolerance, check_every, derivative_step}. With a tolerance set,
// the solver stops early once every lane meets it; the test is made every `check_every` iterations, so the count reported is a
// multiple of that. With both tolerances 0 all `max_iterations` run. A bracketed solver also reports in `bracketed` whether every lane
// had a sign change: if not, the root it returns is meaningless for that lane, and the residual shows it.
#include <pluss/ma.h>

#include <cmath>
#include <iomanip>
#include <iostream>

namespace roots = pluss::subsp::math::roots;
using pluss::Series;

const auto f64 = pluss::ScalarType::Float64;
Series vec(std::vector<double> v) { return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(f64); }
int main() {
  const roots::Fn f = [](const Series& x) { return x.exp() - 5.0; };
  std::cout << std::left << std::setw(42) << "setting" << std::setw(12) << "iterations" << "|residual|\n";
  auto show = [&](const char* label, const roots::Result& r) {
    std::cout << std::left << std::setw(42) << label << std::setw(12) << r.iterations << std::scientific << std::setprecision(1) << std::abs(r.residual.item<double>()) << "\n";
  };
  show("illinois, no tolerance, 30 iterations", roots::illinois(f, vec({0.0}), vec({3.0}), {30, 0.0, 0.0, 8}));
  show("illinois, f_tolerance 1e-8, every 1", roots::illinois(f, vec({0.0}), vec({3.0}), {30, 0.0, 1e-8, 1}));
  show("illinois, f_tolerance 1e-8, every 8", roots::illinois(f, vec({0.0}), vec({3.0}), {30, 0.0, 1e-8, 8}));
  show("bisect, x_tolerance 1e-6, every 1", roots::bisect(f, vec({0.0}), vec({3.0}), {60, 1e-6, 0.0, 1}));

  const roots::Fn g = [](const Series& x) { return x.square() + 1.0; };  // no real root
  const auto bad = roots::bisect(g, vec({-1.0}), vec({1.0}));
  std::cout << "\nx^2 + 1 on [-1, 1]: bracketed = " << (bad.bracketed ? "yes" : "no") << ", residual " << std::fixed << std::setprecision(3) << bad.residual.item<double>() << "\n";
}
