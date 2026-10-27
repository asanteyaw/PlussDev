// The first root. `roots::bisect(f, lo, hi)` finds a root of f between lo and hi, which must bracket one: f(lo) and f(hi) have
// opposite signs. `f` maps a Series to a Series of the same shape, so the same call solves one equation or a whole batch. The result
// holds the root, the residual f(root), the iterations used and whether every lane's bracket had a sign change. Here: the root of
// x^3 - 2x - 5 between 2 and 3 (the real root is 2.0945514815423265).
#include <pluss/ma.h>

#include <cmath>
#include <iomanip>
#include <iostream>

namespace roots = pluss::subsp::math::roots;
using pluss::Series;

const auto f64 = pluss::ScalarType::Float64;
Series vec(std::vector<double> v) { return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(f64); }
int main() {
  const roots::Fn f = [](const Series& x) { return x.pow(3.0) - x * 2.0 - 5.0; };
  const auto r = roots::bisect(f, vec({2.0}), vec({3.0}));
  std::cout << std::setprecision(15) << "root       : " << as_f64(r.root) << "\n";
  std::cout << std::scientific << std::setprecision(2) << "residual   : " << as_f64(r.residual) << "\n";
  std::cout << "iterations : " << r.iterations << "\n";
  std::cout << "bracketed  : " << (r.bracketed ? "yes" : "no") << "\n";
}
