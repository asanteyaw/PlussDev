// The surrogate. With `surrogate_mode = CandidateOrdering` the solver fits a cheap model to the points it has already evaluated
// and uses it only to decide the ORDER in which poll candidates are evaluated, most promising first; every point is still
// evaluated by the true function, so the answer is never wrong, and an opportunistic poll that finds its improvement earlier
// stops sooner. `surrogate_min_points` is the number of evaluated points needed before the model is used,
// `surrogate_max_points` the number kept, `surrogate_bandwidth` its smoothing length (0 = chosen automatically).
// `surrogate_orderings` counts the polls that were reordered. Problem: a weighted quadratic sum_i i (x_i - 1)^2 in 4, 8 and
// 12 variables, started at (-1, ..., -1) and run to convergence, so the evaluation counts compare like with like.
#include <pluss/ma.h>

#include <cmath>
#include <iomanip>
#include <iostream>

namespace dfo = pluss::subsp::math::optimization;

const auto f64 = pluss::ScalarType::Float64;
pluss::Series vec(std::vector<double> v) { return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(f64); }
pluss::Series filled(std::int64_t n, double v) { return pluss::make_series(std::vector<double>(n, v), {n}).to(f64); }
int main() {
  std::cout << std::left << std::setw(6) << "dim" << std::setw(20) << "surrogate_mode" << std::setw(14) << "status" << std::setw(8) << "evals" << "orderings\n";
  for (std::int64_t dim : {4, 8, 12}) {
    std::vector<double> w(dim);
    for (std::int64_t i = 0; i < dim; ++i) w[i] = static_cast<double>(i + 1);
    const auto weights = vec(w);
    for (auto mode : {dfo::SurrogateMode::Off, dfo::SurrogateMode::CandidateOrdering}) {
      dfo::MadsBlackboxAdvancedConfig c;
      c.surrogate_mode = mode;
      c.max_iterations = 5000;
      c.max_evaluations = 20000;
      auto blackbox = [&](const pluss::Series& x) {
        dfo::BlackboxEvaluation e;
        e.objectives = ((x - 1.0).square() * weights).sum().reshape({1});
        return e;
      };
      const auto o = dfo::mads_blackbox_minimizer(blackbox, filled(dim, -1.0), c);
      std::cout << std::left << std::setw(6) << dim << std::setw(20) << (mode == dfo::SurrogateMode::Off ? "Off" : "CandidateOrdering")
                << std::setw(14) << dfo::to_string(o.status) << std::setw(8) << o.diagnostics.evaluations << o.diagnostics.surrogate_orderings << "\n";
    }
  }
}
