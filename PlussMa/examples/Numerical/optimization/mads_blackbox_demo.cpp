#include <pluss/pluss.h>

#include <cmath>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

double as_f64(const pluss::Series& x) {
  if (!x.defined() || x.numel() != 1) {
    throw std::runtime_error("expected scalar Series");
  }
  return x.to(pluss::pCPU, pluss::ScalarType::Float64).item<double>();
}

pluss::Series rugged_ackley_blackbox(const pluss::Series& x) {
  const pluss::Series sq_mean = x.square().mean();
  const pluss::Series cos_mean = x.mul(2.0 * 3.14159265358979323846).cos().mean();
  const pluss::Series ackley = pluss::make_series(20.0, x.scalar_type())
      .add(pluss::make_series(2.71828182845904523536, x.scalar_type()))
      .sub(sq_mean.sqrt().mul(-0.2).exp().mul(20.0))
      .sub(cos_mean.exp());

  // Hidden-constraint-like failure region and deterministic ripple.
  if (x.abs().amax().item<double>() > 4.75) {
    return pluss::make_series(std::numeric_limits<double>::infinity());
  }
  const pluss::Series ripple = x.sum().mul(0.5).sin().abs().mul(0.02);
  return ackley.add(ripple);
}

std::string to_string(pluss::subsp::math::optimization::BlackboxSolverStatus s) {
  using pluss::subsp::math::optimization::BlackboxSolverStatus;
  switch (s) {
    case BlackboxSolverStatus::Converged:
      return "Converged";
    case BlackboxSolverStatus::ReachedIterationBudget:
      return "ReachedIterationBudget";
    case BlackboxSolverStatus::ReachedEvaluationBudget:
      return "ReachedEvaluationBudget";
  }
  return "Unknown";
}

void print_outcome(
    const std::string& name,
    const pluss::subsp::math::optimization::BlackboxSolverOutcome& out) {
  const pluss::Series x = out.minimizer.to(pluss::pCPU, pluss::ScalarType::Float64);
  const auto summary = pluss::transmit(
      [](double v) { return std::abs(v); },
      x.to_vec<double>());
  double max_abs = 0.0;
  for (double v : summary) {
    max_abs = std::max(max_abs, v);
  }

  std::cout << name
            << " status=" << to_string(out.status)
            << " iter=" << out.diagnostics.iterations
            << " evals=" << out.diagnostics.evaluations
            << " f=" << out.minimum
            << " max|x|=" << max_abs
            << " phase=" << out.diagnostics.last_phase
            << " mesh=" << out.diagnostics.mesh_size
            << " frame=" << out.diagnostics.frame_size
            << '\n';
}

} // namespace

int main() {
  using namespace pluss::subsp::math::optimization;

  const std::int64_t dim = 10;
  const pluss::Config cfg = pluss::Config{}
                                .with_dtype(pluss::ScalarType::Float64)
                                .with_platform(pluss::pCPU);
  const pluss::Series x0 = pluss::full({dim}, 3.25, cfg);
  const pluss::Series lb = pluss::full({dim}, -5.0, cfg);
  const pluss::Series ub = pluss::full({dim}, 5.0, cfg);

  MadsBlackboxConfig base_cfg;
  base_cfg.max_iterations = 1200;
  base_cfg.max_evaluations = 5000;
  base_cfg.check_every = 20;
  base_cfg.search_points = 8;
  base_cfg.relative_improvement_tolerance = 1e-9;
  base_cfg.mesh_tolerance = 1e-7;
  base_cfg.initial_mesh_size = 0.2;
  base_cfg.initial_frame_size = 0.5;
  base_cfg.mesh_refine_factor = 0.5;
  base_cfg.frame_refine_factor = 0.5;
  base_cfg.mesh_expand_factor = 1.8;
  base_cfg.frame_expand_factor = 1.6;
  base_cfg.min_improvement = 1e-14;
  base_cfg.opportunistic = true;
  base_cfg.lower_bounds = lb;
  base_cfg.upper_bounds = ub;
  base_cfg.display = BlackboxSolverDisplay::None;

  auto ortho_cfg = base_cfg;
  ortho_cfg.rng_seed = 20260301ULL;
  ortho_cfg.poll_scheme = MadsPollScheme::Ortho2N;

  auto lt_cfg = base_cfg;
  lt_cfg.rng_seed = 20260302ULL;
  lt_cfg.poll_scheme = MadsPollScheme::LtMadsRandom2N;

  std::cout << std::fixed << std::setprecision(8);
  std::cout << "NOMAD-style blackbox optimization demo (MADS variants)\n";
  std::cout << "Objective: rugged Ackley in " << dim << "D\n";

  const auto ortho_out = orthomads_minimizer(rugged_ackley_blackbox, x0, ortho_cfg);
  const auto lt_out = ltmads_minimizer(rugged_ackley_blackbox, x0, lt_cfg);

  print_outcome("orthomads", ortho_out);
  print_outcome("ltmads", lt_out);
  return 0;
}
