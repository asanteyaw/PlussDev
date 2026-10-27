// The Pareto archive. The solver keeps the non-dominated points among those it has evaluated: a point enters if no archived
// point is at least as good on every objective (within `pareto_epsilon`), and leaves when a later point dominates it. The
// archive keeps at most `pareto_archive_limit` points; past the limit the most crowded entry (the one closest to its
// neighbour in objective space) is dropped, so the archive keeps its spread. Problem: f1 = x^2, f2 = (x-2)^2 on one variable,
// whose true non-dominated set is 0 <= x <= 2. The archive covers the part the search visited; it may include a point just
// outside [0, 2] if no evaluated point dominates it, as the first row shows.
#include <pluss/ma.h>

#include <cmath>
#include <iomanip>
#include <iostream>

namespace dfo = pluss::subsp::math::optimization;

const auto f64 = pluss::ScalarType::Float64;
pluss::Series vec(std::vector<double> v) { return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(f64); }
pluss::Series filled(std::int64_t n, double v) { return pluss::make_series(std::vector<double>(n, v), {n}).to(f64); }
int main() {
  std::cout << std::left << std::setw(22) << "pareto_archive_limit" << std::setw(16) << "archive size" << std::setw(4) << "" << "x range\n";
  for (std::int64_t limit : {256, 20, 5}) {
    dfo::MadsBlackboxAdvancedConfig c;
    c.pareto_archive_limit = limit;
    c.multiobjective_scalarization = dfo::MultiObjectiveScalarization::WeightedSum;
    c.objective_weights = vec({0.5, 0.5});
    c.max_evaluations = 800;
    auto blackbox = [](const pluss::Series& x) {
      dfo::BlackboxEvaluation e;
      const auto a = x.select(0, 0);
      e.objectives = a.square().reshape({1}).unite({(a - 2.0).square().reshape({1})}, 0);
      return e;
    };
    const auto o = dfo::mads_blackbox_minimizer(blackbox, vec({5.0}), c);
    const auto p = o.pareto_points.reshape({o.pareto_points.numel()}).host<double>();
    
    double lo = 1e300, hi = -1e300;
    for (double v : p) { lo = std::min(lo, v); hi = std::max(hi, v); }
    std::cout << std::left << std::setw(22) << limit << std::setw(16) << p.size() << std::setw(4) << "" << std::fixed << std::setprecision(3) << lo << " .. " << hi << "\n";
  }
}
