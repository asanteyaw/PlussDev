// What counts as an evaluation. Three counters exist: `evaluations` (calls that count against `max_evaluations`),
// `total_evaluations` (every call, bounded by `max_total_evaluations`, default four times `max_evaluations`) and `cache_hits`.
// A call that sets `count_evaluation = false` (a cheap pre-check, say) is in the total but not in `evaluations`; a repeat
// answered from the cache is counted in the total when `count_cache_hits_in_total_evaluations` is true (the default). This
// program makes half of the calls "free" and shows how the counters and the budgets respond.
#include <pluss/ma.h>

#include <cmath>
#include <iomanip>
#include <iostream>

namespace dfo = pluss::subsp::math::optimization;

const auto f64 = pluss::ScalarType::Float64;
pluss::Series vec(std::vector<double> v) { return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(f64); }
pluss::Series filled(std::int64_t n, double v) { return pluss::make_series(std::vector<double>(n, v), {n}).to(f64); }
const char* name(dfo::BlackboxSolverStatus s) {
  return s == dfo::BlackboxSolverStatus::Converged ? "Converged"
         : s == dfo::BlackboxSolverStatus::ReachedEvaluationBudget ? "ReachedEvaluationBudget" : "ReachedIterationBudget";
}
int main() {
  std::cout << std::left << std::setw(40) << "setting" << std::setw(26) << "status" << std::setw(8) << "evals" << std::setw(8) << "total" << "cache hits\n";
  auto run = [](const char* label, dfo::MadsBlackboxAdvancedConfig c, bool free_half) {
    std::int64_t calls = 0;
    auto blackbox = [&](const pluss::Series& x) {
      dfo::BlackboxEvaluation e;
      e.objectives = (x - vec({1.0, 2.0})).abs().sum().reshape({1});
      e.count_evaluation = !(free_half && (calls++ % 2 == 1));
      return e;
    };
    const auto o = dfo::mads_blackbox_minimizer(blackbox, vec({4.0, -3.0}), c);
    std::cout << std::left << std::setw(40) << label << std::setw(26) << name(o.status) << std::setw(8) << o.diagnostics.evaluations
              << std::setw(8) << o.diagnostics.total_evaluations << o.diagnostics.cache_hits << "\n";
  };
  dfo::MadsBlackboxAdvancedConfig base;
  base.max_evaluations = 400;
  run("defaults", base, false);
  run("every other call free", base, true);
  auto capped = base; capped.max_total_evaluations = 300;
  run("every other call free, total cap 300", capped, true);
  auto nocount = base; nocount.count_cache_hits_in_total_evaluations = false;
  run("cache hits not counted in total", nocount, false);
  auto nocache = base; nocache.enable_cache = false;
  run("cache off", nocache, false);
}
