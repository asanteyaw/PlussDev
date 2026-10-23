// The knobs of `NelderMeadConfig`, one at a time, on the same 4-D quadratic with an off-axis valley. Each row changes a
// single setting from the default and shows what it does to the work (evaluations) and the accuracy (minimum reached).
//   position_tolerance, value_tolerance   stop when the simplex is this small AND its values agree to this
//   simplex_delta                         relative size of the starting simplex (default 5%)
//   reflection / expansion / contraction / shrink   the four step coefficients (defaults 1, 2, 0.5, 0.5)
//   max_evaluations                       budget
#include <pluss/ma.h>

#include <iomanip>
#include <iostream>

namespace dfo = pluss::subsp::math::optimization;

const auto f64 = pluss::ScalarType::Float64;
pluss::Series vec(std::vector<double> v) { return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(f64); }

void run(const char* label, dfo::NelderMeadConfig c) {
  auto f = [](const pluss::Series& x) {
    const auto d = x - vec({1.0, -1.0, 2.0, 0.5});
    const auto s = d.select(0, 0) + d.select(0, 1) + d.select(0, 2) + d.select(0, 3);
    return d.square().sum() + s.square() * 0.8;
  };
  const auto o = dfo::nelder_mead_minimizer(f, vec({3.0, 2.0, -1.0, 1.0}), c);
  std::cout << std::left << std::setw(30) << label << std::setw(24)
            << (o.status == dfo::SolverStatus::Converged ? "Converged"
                : o.status == dfo::SolverStatus::ReachedEvaluationBudget ? "ReachedEvaluationBudget" : "ReachedIterationBudget")
            << std::scientific << std::setprecision(2) << std::setw(11) << o.minimum << std::defaultfloat << "evals "
            << o.diagnostics.evaluations << "\n";
}

int main() {
  dfo::NelderMeadConfig base;
  run("defaults", base);
  auto tight = base; tight.position_tolerance = 1e-8; tight.value_tolerance = 1e-8;
  run("tolerances 1e-8", tight);
  auto loose = base; loose.position_tolerance = 1e-2; loose.value_tolerance = 1e-2;
  run("tolerances 1e-2", loose);
  auto big = base; big.simplex_delta = 0.5;
  run("simplex_delta 0.5", big);
  auto tiny = base; tiny.simplex_delta = 0.001;
  run("simplex_delta 0.001", tiny);
  auto bold = base; bold.expansion = 3.0;
  run("expansion 3", bold);
  auto timid = base; timid.contraction = 0.25; timid.shrink = 0.25;
  run("contraction, shrink 0.25", timid);
  auto capped = base; capped.max_evaluations = 100;
  run("max_evaluations 100", capped);
}
