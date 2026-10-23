#include "../mads_blackbox_minimizers.h"

#include <cmath>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

using pluss::Config;
using pluss::ScalarType;
using pluss::Series;
using pluss::pCPU;
using namespace pluss::subsp::math::optimization;

void expect(bool cond, const std::string& msg, int* failures) {
  if (!cond) {
    ++(*failures);
    std::cerr << "[FAIL] " << msg << '\n';
  }
}

BlackboxEvaluation scalar_sphere(const Series& x) {
  BlackboxEvaluation out;
  out.objectives = x.square().sum();
  return out;
}

BlackboxEvaluation constrained_shifted(const Series& x) {
  BlackboxEvaluation out;
  out.objectives = x.sub(1.0).square().sum();
  out.constraints = x; // x <= 0
  return out;
}

BlackboxEvaluation two_objective_tradeoff(const Series& x) {
  BlackboxEvaluation out;
  const Series x0 = x.reshape({x.numel()}).select(0, 0);
  const Series f1 = x0.square();
  const Series f2 = x0.sub(2.0).square();
  const double f1v = f1.to(pCPU, ScalarType::Float64).item<double>();
  const double f2v = f2.to(pCPU, ScalarType::Float64).item<double>();
  out.objectives = pluss::make_series({f1v, f2v}, Config{}.with_dtype(x.scalar_type()).with_platform(x.platform()));
  return out;
}

} // namespace

int main() {
  int failures = 0;

  const Config cfg = Config{}.with_dtype(ScalarType::Float64).with_platform(pCPU);

  // Cache-hit behavior via degenerate bounds (all candidates collapse to one point).
  {
    const Series x0 = pluss::series::full({3}, 2.0, cfg);
    const Series lock = pluss::series::full({3}, 0.0, cfg);

    MadsBlackboxAdvancedConfig c;
    c.max_iterations = 20;
    c.max_evaluations = 120;
    c.max_total_evaluations = 200;
    c.search_points = 8;
    c.lower_bounds = lock;
    c.upper_bounds = lock;
    c.enable_cache = true;
    c.surrogate_mode = SurrogateMode::CandidateOrdering;
    c.surrogate_min_points = 1;
    c.rng_seed = 401ULL;

    const auto out = orthomads_blackbox_minimizer(scalar_sphere, x0, c);
    expect(out.diagnostics.cache_hits > 0, "expected cache hits in degenerate-bounds case", &failures);
    expect(out.diagnostics.total_evaluations >= out.diagnostics.evaluations,
           "total evaluations should be >= blackbox evaluations", &failures);
    expect(out.diagnostics.surrogate_orderings > 0,
           "expected surrogate ordering usage", &failures);
  }

  // Extreme barrier should keep solution feasible for c(x)=x<=0 in 1D.
  {
    const Series x0 = pluss::make_series({1.5}, cfg);
    const Series lb = pluss::make_series({-2.0}, cfg);
    const Series ub = pluss::make_series({0.0}, cfg);

    MadsBlackboxAdvancedConfig c;
    c.max_iterations = 120;
    c.max_evaluations = 300;
    c.search_points = 6;
    c.lower_bounds = lb;
    c.upper_bounds = ub;
    c.constraint_handling = ConstraintHandling::ExtremeBarrier;
    c.rng_seed = 402ULL;

    const auto out = orthomads_blackbox_minimizer(constrained_shifted, x0, c);
    const Series x_best = out.minimizer.to(pCPU, ScalarType::Float64);
    expect(x_best.item<double>() <= 1e-6, "extreme barrier solution should satisfy x<=0", &failures);
    expect(std::isfinite(out.scalarized_minimum), "extreme barrier should return finite best value", &failures);
  }

  // Multi-objective: ensure objective dimension and Pareto archive are populated.
  {
    const Series x0 = pluss::make_series({0.5}, cfg);
    const Series lb = pluss::make_series({-1.0}, cfg);
    const Series ub = pluss::make_series({3.0}, cfg);

    MadsBlackboxAdvancedConfig c;
    c.max_iterations = 120;
    c.max_evaluations = 180;
    c.search_points = 12;
    c.opportunistic = false;
    c.lower_bounds = lb;
    c.upper_bounds = ub;
    c.multiobjective_scalarization = MultiObjectiveScalarization::WeightedSum;
    c.objective_weights = pluss::make_series({0.5, 0.5}, cfg);
    c.pareto_archive_limit = 256;
    c.rng_seed = 403ULL;

    const auto out = ltmads_blackbox_minimizer(two_objective_tradeoff, x0, c);
    expect(out.diagnostics.objective_dimension == 2, "expected objective_dimension=2", &failures);
    expect(out.pareto_objectives.defined(), "expected defined Pareto objective archive", &failures);
    expect(out.diagnostics.pareto_size > 1, "expected Pareto archive with multiple points", &failures);
  }

  // Family variants: ensure they execute and return finite scalarized values.
  {
    const Series x0 = pluss::series::full({3}, 0.7, cfg);
    const Series lb = pluss::series::full({3}, -2.0, cfg);
    const Series ub = pluss::series::full({3}, 2.0, cfg);

    MadsBlackboxAdvancedConfig base;
    base.max_iterations = 80;
    base.max_evaluations = 200;
    base.max_total_evaluations = 300;
    base.search_points = 8;
    base.lower_bounds = lb;
    base.upper_bounds = ub;
    base.constraint_handling = ConstraintHandling::ProgressiveBarrier;
    base.surrogate_mode = SurrogateMode::CandidateOrdering;
    base.surrogate_min_points = 6;
    base.rng_seed = 500ULL;

    auto bb = [](const Series& x) {
      BlackboxEvaluation out;
      out.objectives = x.square().sum();
      out.constraints = x.sum().sub(0.8);
      return out;
    };

    CoopMadsConfig coop;
    coop.base = base;
    coop.cooperative_rounds = 2;
    coop.subproblem_size = 2;
    coop.subproblems_per_round = 2;
    coop.rng_seed = 501ULL;
    const auto coop_out = coopmads_blackbox_minimizer(bb, x0, coop);
    expect(std::isfinite(coop_out.scalarized_minimum), "COOP returned non-finite scalarized value", &failures);

    PsdMadsConfig psd;
    psd.base = base;
    psd.rounds = 2;
    psd.min_subproblem_size = 1;
    psd.max_subproblem_size = 2;
    psd.subproblems_per_round = 2;
    psd.rng_seed = 502ULL;
    const auto psd_out = psdmads_blackbox_minimizer(bb, x0, psd);
    expect(std::isfinite(psd_out.scalarized_minimum), "PSD returned non-finite scalarized value", &failures);

    DiscoMadsConfig disco;
    disco.base = base;
    disco.rescue_trials = 3;
    disco.rng_seed = 503ULL;
    const auto disco_out = discomads_blackbox_minimizer(bb, x0, disco);
    expect(std::isfinite(disco_out.scalarized_minimum), "DISCO returned non-finite scalarized value", &failures);

    VnsMadsConfig vns;
    vns.base = base;
    vns.neighborhoods = 3;
    vns.rng_seed = 504ULL;
    const auto vns_out = vnsmads_blackbox_minimizer(bb, x0, vns);
    expect(std::isfinite(vns_out.scalarized_minimum), "VNS returned non-finite scalarized value", &failures);
  }

  {
    // Disco must never report the value of a different point as the value of the point it returns: the reported minimum has to
    // be the true objective at the returned minimiser, even when most evaluations fail.
    const Config c64 = Config{}.with_dtype(ScalarType::Float64).with_platform(pCPU);
    auto objective = [](double a, double b) { return (a - 2.0) * (a - 2.0) + (b - 1.0) * (b - 1.0); };
    auto flaky = [&](const Series& x) {
      BlackboxEvaluation e;
      const auto v = x.to_vec<double>();
      double h = std::sin(12345.678 * v[0] + 9876.543 * v[1] + 0.5) * 43758.5453;
      h -= std::floor(h);
      if (h < 0.8) {
        e.objectives = pluss::make_series(std::vector<double>{std::nan("")}, {1}, c64);
        return e;
      }
      e.objectives = pluss::make_series(std::vector<double>{objective(v[0], v[1])}, {1}, c64);
      return e;
    };
    DiscoMadsConfig dc;
    dc.base.max_evaluations = 3000;
    dc.rescue_trials = 12;
    const auto out = discomads_blackbox_minimizer(flaky, pluss::make_series(std::vector<double>{0.0, 0.0}, {2}, c64), dc);
    const auto m = out.minimizer.to_vec<double>();
    expect(std::abs(out.scalarized_minimum - objective(m[0], m[1])) < 1e-9,
           "Disco reports the true objective at the returned minimiser", &failures);
    MadsBlackboxAdvancedConfig plain;
    plain.max_evaluations = 3000;
    const auto base = mads_blackbox_minimizer(flaky, pluss::make_series(std::vector<double>{0.0, 0.0}, {2}, c64), plain);
    expect(out.scalarized_minimum <= base.scalarized_minimum + 1e-12,
           "Disco is never worse than plain MADS on the same flaky function", &failures);
  }

  {
    // The decomposition solvers spend part of the budget on a final full-space run; on a coupled function that run must help
    // (the blocks alone cannot remove the error the coupling leaves).
    const Config c64 = Config{}.with_dtype(ScalarType::Float64).with_platform(pCPU);
    const std::int64_t n = 10;
    auto chain = [n](const Series& x) {
      BlackboxEvaluation e;
      const Series a = x.slice(0, 0, n - 1, 1), b = x.slice(0, 1, n, 1);
      e.objectives = a.sub(b).square().sum().add(x.select(0, 0).sub(1.0).square()).reshape({1});
      return e;
    };
    const Series start = pluss::series::full({n}, -1.0, c64);
    for (const bool psd : {false, true}) {
      double minimum[2] = {0.0, 0.0};
      for (int with_polish = 0; with_polish < 2; ++with_polish) {
        const double fraction = with_polish ? 0.5 : 0.0;
        if (psd) {
          PsdMadsConfig c;
          c.base.max_evaluations = 8000;
          c.base.max_iterations = 5000;
          c.polish_fraction = fraction;
          minimum[with_polish] = psdmads_blackbox_minimizer(chain, start, c).scalarized_minimum;
        } else {
          CoopMadsConfig c;
          c.base.max_evaluations = 8000;
          c.base.max_iterations = 5000;
          c.polish_fraction = fraction;
          minimum[with_polish] = coopmads_blackbox_minimizer(chain, start, c).scalarized_minimum;
        }
      }
      expect(minimum[1] < minimum[0], "a full-space polish improves a decomposition solver on a coupled function", &failures);
    }
  }

  if (failures != 0) {
    std::cerr << "mads_blackbox_advanced_test: " << failures << " failure(s)\n";
    return 1;
  }

  std::cout << "mads_blackbox_advanced_test: all checks passed\n";
  return 0;
}
