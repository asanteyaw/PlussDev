// Two objectives. The black box returns a vector of objective values and the solver turns it into one number to
// minimise: a weighted sum by default, or the Chebyshev (largest weighted gap to a reference) rule. The weights say
// which objective matters more. Here f1 = (x-1)^2 and f2 = (x+1)^2 pull a single variable in opposite directions;
// any x in [-1, 1] is a fair compromise, and the weights pick the point. For the weighted sum with weights (w, 1-w)
// the minimiser is x = 1 - 2 (1 - w) = 2w - 1.
#include <pluss/ma.h>

#include <cmath>
#include <iomanip>
#include <iostream>

namespace dfo = pluss::subsp::math::optimization;

const auto f64 = pluss::ScalarType::Float64;
pluss::Series vec(std::vector<double> v) { return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(f64); }

int main() {
  std::cout << std::left << std::setw(12) << "weight w" << std::setw(12) << "x" << std::setw(12) << "expected" << std::setw(12)
            << std::setw(12) << "f1" << std::setw(12) << "f2" << "pareto points\n";
  for (double w : {0.2, 0.5, 0.8}) {
    dfo::DfoProblem p;
    p.initial = vec({0.0});
    p.blackbox = [](const pluss::Series& x) {
      dfo::BlackboxEvaluation e;
      const auto f1 = (x - 1.0).square().sum().reshape({1});
      const auto f2 = (x + 1.0).square().sum().reshape({1});
      e.objectives = f1.unite({f2}, 0);
      return e;
    };
    dfo::DfoOptions o;
    o.mads.multiobjective_scalarization = dfo::MultiObjectiveScalarization::WeightedSum;
    o.mads.objective_weights = vec({w, 1.0 - w});
    const auto r = dfo::DFO::solve(p, o);
    const auto f = r.objective_vector.host<double>();
    std::cout << std::fixed << std::setprecision(3) << std::left << std::setw(12) << w << std::setw(12)
              << r.minimizer.select(0, 0).item<double>() << std::setw(12) << 2.0 * w - 1.0 << std::setw(12) << std::setw(12) << f[0] << std::setw(12) << f[1] << r.pareto_points.size(0) << "\n";
  }
}
