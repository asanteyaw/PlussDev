// A worked problem: fit y = a exp(-b t) + c to eight measurements by least squares. The residual sum of squares is
// the objective; no gradient is written. Strategy used throughout the chapter: a quick local solve, then a polish
// that restarts a different method from the first answer. The data were generated from a = 2, b = 0.7, c = 0.5.
#include <pluss/ma.h>

#include <cmath>
#include <iomanip>
#include <iostream>

namespace dfo = pluss::subsp::math::optimization;

const auto f64 = pluss::ScalarType::Float64;
pluss::Series vec(std::vector<double> v) { return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(f64); }

int main() {
  const auto t = vec({0.0, 0.5, 1.0, 1.5, 2.0, 3.0, 4.0, 6.0});
  std::vector<double> yv;
  for (double ti : {0.0, 0.5, 1.0, 1.5, 2.0, 3.0, 4.0, 6.0}) yv.push_back(2.0 * std::exp(-0.7 * ti) + 0.5);
  const auto y = vec(yv);
  auto sse = [&](const pluss::Series& p) {
    const auto model = (t * (p.select(0, 1) * -1.0)).exp() * p.select(0, 0) + p.select(0, 2);
    return (model - y).square().sum();
  };

  dfo::DfoProblem prob;
  prob.initial = vec({1.0, 1.0, 0.0});
  prob.objective = sse;
  prob.lower_bounds = vec({0.0, 0.0, -1.0});
  prob.upper_bounds = vec({10.0, 5.0, 5.0});

  auto show = [](const char* label, const dfo::DfoResult& r) {
    const auto p = r.minimizer.to_vec<double>();
    std::cout << std::left << std::setw(26) << label << std::setw(26) << dfo::to_string(r.status) << std::fixed << std::setprecision(4)
              << "a " << p[0] << "  b " << p[1] << "  c " << p[2] << std::scientific << std::setprecision(1) << "  sse " << r.minimum
              << std::defaultfloat << "  evals " << r.diagnostics.evaluations << "\n";
  };

  dfo::DfoOptions first;
  first.solver = dfo::DfoSolverKind::NelderMead;
  const auto r1 = dfo::DFO::solve(prob, first);
  show("1. Nelder-Mead", r1);

  prob.initial = r1.minimizer;
  dfo::DfoOptions polish;
  polish.solver = dfo::DfoSolverKind::OrthoMads;
  const auto r2 = dfo::DFO::solve(prob, polish);
  show("2. OrthoMads from (1)", r2);
  std::cout << "true values               a 2.0000  b 0.7000  c 0.5000\n";
}
