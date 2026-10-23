// A black box that sometimes fails. Imagine the objective is a simulation that does not converge when its first
// parameter is above 1.5. The black-box interface lets the function say so: return `evaluation_ok = false` and the
// solver treats the point as unusable, without needing a made-up large objective value. The true minimum of
// (x0 - 2)^2 + (x1 - 1)^2 is at x0 = 2, inside the failing region, so the best usable answer sits at the edge of it:
// (1.5, 1) with objective 0.25.
#include <pluss/ma.h>

#include <iomanip>
#include <iostream>

namespace dfo = pluss::subsp::math::optimization;

const auto f64 = pluss::ScalarType::Float64;
pluss::Series vec(std::vector<double> v) { return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(f64); }

int main() {
  std::int64_t calls = 0, failures = 0;
  dfo::DfoProblem p;
  p.initial = vec({0.0, 0.0});
  p.blackbox = [&](const pluss::Series& x) {
    ++calls;
    dfo::BlackboxEvaluation e;
    if (x.select(0, 0).item<double>() > 1.5) {
      ++failures;
      e.objectives = pluss::make_series(0.0).to(f64);  // ignored: the point is flagged as failed
      e.evaluation_ok = false;
      return e;
    }
    e.objectives = (x - vec({2.0, 1.0})).square().sum().reshape({1});
    return e;
  };
  const auto r = dfo::DFO::solve(p);
  std::cout << "solver      : " << dfo::to_string(r.selected_solver) << " (" << r.selection_reason << ")\n";
  std::cout << "status      : " << dfo::to_string(r.status) << "\n";
  std::cout << std::fixed << std::setprecision(3) << "minimizer   : " << r.minimizer.select(0, 0).item<double>() << ", "
            << r.minimizer.select(0, 1).item<double>() << "\n";
  std::cout << "objective   : " << r.minimum << "  (usable optimum 0.250)\n";
  std::cout << "calls made  : " << calls << " (" << failures << " of them failed)\n";
}
