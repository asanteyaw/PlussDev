// Why a trust region with indefinite curvature is worth having. f(x, y) = x^2 - y^2 + y^4 / 4 has a saddle at the origin
// (it curves up along x and down along y) and two minima at (0, +-sqrt(2)) where f = -1. The runs start at (x0, eps), very
// close to the line y = 0, where the gradient in y is tiny. L-BFGS builds a model that is always positive-definite, so it sees
// no reason to leave the saddle's neighbourhood and stalls; L-SR1's model can represent the negative curvature along y and
// takes a step along it. The table gives the objective each method ends at (-1 is the minimum). The last row starts so close that
// the gradient is already below the tolerance: every method then reports `Converged`, because the test the solvers share
// is a first-order test, which a saddle satisfies as well as a minimum does.
#include <pluss/ma.h>

#include <iomanip>
#include <iostream>

namespace optim = pluss::optim;
using optim::Trace;

const auto f64 = pluss::ScalarType::Float64;
pluss::Series vec(std::vector<double> v) { return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(f64); }

int main() {
  const auto index_x = pluss::make_series(std::vector<double>{0}, {1}).to(pluss::ScalarType::Int64);
  const auto index_y = pluss::make_series(std::vector<double>{1}, {1}).to(pluss::ScalarType::Int64);
  auto saddle = [&](const Trace& v) -> Trace {
    const Trace x = pluss::subsp::math::diff::index_select(v, 0, index_x), y = pluss::subsp::math::diff::index_select(v, 0, index_y);
    return optim::sum(optim::square(x) - optim::square(y) + 0.25 * optim::square(optim::square(y)));
  };

  std::cout << std::left << std::setw(12) << "x0" << std::setw(10) << "eps" << std::setw(26) << "L-SR1 (exit, objective)" << "L-BFGS (exit, objective)\n";
  struct Start { double x0, eps; };
  for (const Start s : {Start{0.5, 1e-1}, Start{1e-3, 1e-3}, Start{0.5, 1e-6}, Start{1e-3, 1e-6}, Start{0.5, 1e-9}}) {
    const auto sr1 = optim::l_sr1_trust_region(saddle, vec({s.x0, s.eps}));
    optim::SeptimaSolveOptions options;
    options.max_iterations = 2000;
    options.step.optimizer = optim::SeptimaOptimizer::Lbfgs;
    const auto lbfgs = optim::septima(saddle, vec({s.x0, s.eps}), options);
    std::ostringstream a, b;
    a << (sr1.converged ? "Converged" : "not converged") << ", " << std::scientific << std::setprecision(1) << sr1.objective_value;
    b << (lbfgs.converged ? "Converged" : "NoProgress") << ", " << std::scientific << std::setprecision(1) << lbfgs.objective_value;
    std::cout << std::left << std::setw(12) << std::defaultfloat << s.x0 << std::setw(10) << s.eps << std::setw(26) << a.str() << b.str() << "\n";
  }
  return 0;
}
