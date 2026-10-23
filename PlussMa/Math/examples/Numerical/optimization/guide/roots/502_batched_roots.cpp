// Many equations in one call. Every solver advances all lanes of a Series together with ordinary array operations, so a parameter
// sweep is one call. Here the cubic x^3 - x = k is solved for six values of k at once (a different equation in each lane),
// bracketed by [1, 10] in every lane. For k = 6, 24, 60 the roots are exactly 2, 3, 4 (since x^3 - x = 6, 24, 60).
#include <pluss/ma.h>

#include <cmath>
#include <iomanip>
#include <iostream>

namespace roots = pluss::subsp::math::roots;
using pluss::Series;

const auto f64 = pluss::ScalarType::Float64;
Series vec(std::vector<double> v) { return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(f64); }
int main() {
  const Series k = vec({0.5, 2.0, 6.0, 24.0, 60.0, 120.0});
  const roots::Fn f = [&](const Series& x) { return x.pow(3.0) - x - k; };
  const auto r = roots::illinois(f, k * 0.0 + 1.0, k * 0.0 + 10.0, {200, 0.0, 1e-14, 8});
  const auto x = r.root.to_vec<double>();
  const auto kv = k.to_vec<double>();
  const auto res = r.residual.to_vec<double>();
  std::cout << std::left << std::setw(8) << "k" << std::setw(20) << "root" << "residual\n";
  for (std::size_t i = 0; i < x.size(); ++i)
    std::cout << std::left << std::fixed << std::setprecision(1) << std::setw(8) << kv[i] << std::setprecision(12) << std::setw(20) << x[i] << std::scientific << std::setprecision(1) << res[i] << "\n";
  std::cout << "iterations: " << r.iterations << " (all lanes together)\n";
}
