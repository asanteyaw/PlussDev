// The four iterations side by side. Each row is the residual |f(root)| for the equation e^x = 5 (root ln 5 = 1.6094...) after a fixed
// number of iterations, so the rate of each method shows directly. Bisection halves the bracket each time; Illinois' regula falsi,
// Newton (with the analytic derivative and with the built-in central difference) and the secant method converge faster; the
// brackets are [0, 3] and the Newton / secant starts are 1.0 (and 2.0).
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
  const roots::Fn df = [](const Series& x) { return x.exp(); };
  std::cout << std::left << std::setw(12) << "iterations" << std::setw(12) << "bisect" << std::setw(12) << "illinois" << std::setw(18)
            << "newton (exact)" << std::setw(18) << "newton (diff.)" << "secant\n";
  for (std::int64_t k : {2, 4, 6, 8, 12}) {
    roots::Options o;
    o.max_iterations = k;
    const auto b = roots::bisect(f, vec({0.0}), vec({3.0}), o);
    const auto il = roots::illinois(f, vec({0.0}), vec({3.0}), o);
    const auto nx = roots::newton(f, vec({1.0}), df, o);
    const auto nd = roots::newton(f, vec({1.0}), {}, o);
    const auto sc = roots::secant(f, vec({1.0}), vec({2.0}), o);
    std::cout << std::left << std::setw(12) << k << std::scientific << std::setprecision(1) << std::setw(12) << std::abs(b.residual.item<double>())
              << std::setw(12) << std::abs(il.residual.item<double>()) << std::setw(18) << std::abs(nx.residual.item<double>()) << std::setw(18)
              << std::abs(nd.residual.item<double>()) << std::abs(sc.residual.item<double>()) << "\n";
  }
}
