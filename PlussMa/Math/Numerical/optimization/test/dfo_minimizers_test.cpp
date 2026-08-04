#include "../dfo_minimizers.h"

#include <cmath>
#include <cstdint>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>

namespace {

using pluss::Config;
using pluss::ScalarType;
using pluss::Series;
using pluss::pCPU;
using namespace pluss::subsp::math::optimization;

double as_f64(const Series& x) {
  if (!x.defined() || x.numel() != 1) {
    throw std::runtime_error("expected scalar Series");
  }
  return x.to(pCPU, ScalarType::Float64).item<double>();
}

double l2_norm(const Series& x) {
  return as_f64(x.square().sum().sqrt());
}

void expect(bool cond, const std::string& msg, int* failures) {
  if (!cond) {
    ++(*failures);
    std::cerr << "[FAIL] " << msg << '\n';
  }
}

Series sphere(const Series& x) {
  return x.square().sum();
}

Series shifted_sphere(const Series& x, double shift) {
  return x.sub(shift).square().sum();
}

Series guarded_sphere(const Series& x) {
  if (!x.abs().le(0.8).all().item<bool>()) {
    return pluss::series::full({1}, std::numeric_limits<double>::infinity(),
                       Config{}.with_dtype(ScalarType::Float64).with_platform(x.platform()));
  }
  return x.square().sum();
}

} // namespace

int main() {
  int failures = 0;
  const Config cfg = Config{}.with_dtype(ScalarType::Float64).with_platform(pCPU);
  const std::int64_t dim = 6;
  const Series x0 = pluss::series::full({dim}, 2.0, cfg);

  DifferentialEvolutionConfig de_cfg;
  de_cfg.population_size = 120;
  de_cfg.max_generations = 500;
  de_cfg.check_every = 20;
  de_cfg.relative_improvement_tolerance = 1e-10;
  de_cfg.rng_seed = 101ULL;
  const auto de_out = differential_evolution_minimizer(sphere, x0, de_cfg);
  std::cerr << "DE f=" << de_out.minimum << " ||x||=" << l2_norm(de_out.minimizer) << '\n';
  expect(std::isfinite(de_out.minimum), "DE produced non-finite objective", &failures);
  expect(de_out.minimum < 1e-2, "DE failed sphere accuracy target", &failures);
  expect(l2_norm(de_out.minimizer) < 1.5e-1, "DE minimizer is too far from zero", &failures);

  DifferentialEvolutionPrmmConfig de_prmm_cfg;
  de_prmm_cfg.population_size = 120;
  de_prmm_cfg.max_evaluations = 25000;
  de_prmm_cfg.check_every = 20;
  de_prmm_cfg.relative_improvement_tolerance = 1e-10;
  de_prmm_cfg.rng_seed = 102ULL;
  const auto de_prmm_out = differential_evolution_prmm_minimizer(sphere, x0, de_prmm_cfg);
  std::cerr << "DE-PRMM f=" << de_prmm_out.minimum << " ||x||=" << l2_norm(de_prmm_out.minimizer) << '\n';
  expect(std::isfinite(de_prmm_out.minimum), "DE-PRMM produced non-finite objective", &failures);
  expect(de_prmm_out.minimum < 1e-6, "DE-PRMM failed sphere accuracy target", &failures);
  expect(l2_norm(de_prmm_out.minimizer) < 2e-2, "DE-PRMM minimizer is too far from zero", &failures);

  ParticleSwarmConfig pso_cfg;
  pso_cfg.population_size = 120;
  pso_cfg.max_generations = 800;
  pso_cfg.check_every = 20;
  pso_cfg.relative_improvement_tolerance = 1e-10;
  pso_cfg.center_particle = true;
  pso_cfg.time_varying_acceleration = true;
  pso_cfg.rng_seed = 103ULL;
  const auto pso_out = particle_swarm_minimizer(sphere, x0, pso_cfg);
  std::cerr << "PSO f=" << pso_out.minimum << " ||x||=" << l2_norm(pso_out.minimizer) << '\n';
  expect(std::isfinite(pso_out.minimum), "PSO produced non-finite objective", &failures);
  expect(pso_out.minimum < 1e-3, "PSO failed sphere accuracy target", &failures);
  expect(l2_norm(pso_out.minimizer) < 2e-1, "PSO minimizer is too far from zero", &failures);

  ParticleSwarmDvConfig pso_dv_cfg;
  pso_dv_cfg.population_size = 120;
  pso_dv_cfg.max_generations = 800;
  pso_dv_cfg.check_every = 20;
  pso_dv_cfg.relative_improvement_tolerance = 1e-10;
  pso_dv_cfg.rng_seed = 104ULL;
  const auto pso_dv_out = particle_swarm_dv_minimizer(sphere, x0, pso_dv_cfg);
  std::cerr << "PSO-DV f=" << pso_dv_out.minimum << " ||x||=" << l2_norm(pso_dv_out.minimizer) << '\n';
  expect(std::isfinite(pso_dv_out.minimum), "PSO-DV produced non-finite objective", &failures);
  expect(pso_dv_out.minimum < 1.0, "PSO-DV failed sphere accuracy target", &failures);
  expect(l2_norm(pso_dv_out.minimizer) < 1.2, "PSO-DV minimizer is too far from zero", &failures);

  DifferentialEvolutionConfig de_rep_cfg;
  de_rep_cfg.population_size = 80;
  de_rep_cfg.max_generations = 200;
  de_rep_cfg.check_every = 20;
  de_rep_cfg.rng_seed = 2026ULL;
  const auto de_rep_a = differential_evolution_minimizer(sphere, x0, de_rep_cfg);
  const auto de_rep_b = differential_evolution_minimizer(sphere, x0, de_rep_cfg);
  const double rep_obj_diff = std::abs(de_rep_a.minimum - de_rep_b.minimum);
  const double rep_x_diff = as_f64(de_rep_a.minimizer.sub(de_rep_b.minimizer).abs().amax(0, false));
  expect(rep_obj_diff < 1e-12, "DE reproducibility objective mismatch", &failures);
  expect(rep_x_diff < 1e-10, "DE reproducibility minimizer mismatch", &failures);

  DifferentialEvolutionConfig de_compat_cfg = de_cfg;
  de_compat_cfg.execution_mode = PopulationSolverExecutionMode::OptimMasterCompat;
  de_compat_cfg.population_size = 80;
  de_compat_cfg.max_generations = 300;
  de_compat_cfg.rng_seed = 6001ULL;
  const auto de_compat_a = differential_evolution_minimizer(sphere, x0, de_compat_cfg);
  const auto de_compat_b = differential_evolution_minimizer(sphere, x0, de_compat_cfg);
  std::cerr << "DE compat f=" << de_compat_a.minimum << " ||x||=" << l2_norm(de_compat_a.minimizer) << '\n';
  expect(std::abs(de_compat_a.minimum - de_compat_b.minimum) < 1e-12,
         "DE compat reproducibility objective mismatch", &failures);
  expect(as_f64(de_compat_a.minimizer.sub(de_compat_b.minimizer).abs().amax(0, false)) < 1e-10,
         "DE compat reproducibility minimizer mismatch", &failures);
  expect(de_compat_a.minimum < 1e-2, "DE compat failed sphere accuracy target", &failures);

  DifferentialEvolutionPrmmConfig de_prmm_compat_cfg = de_prmm_cfg;
  de_prmm_compat_cfg.execution_mode = PopulationSolverExecutionMode::OptimMasterCompat;
  de_prmm_compat_cfg.population_size = 80;
  de_prmm_compat_cfg.max_evaluations = 18000;
  de_prmm_compat_cfg.rng_seed = 6002ULL;
  const auto de_prmm_compat_a = differential_evolution_prmm_minimizer(sphere, x0, de_prmm_compat_cfg);
  const auto de_prmm_compat_b = differential_evolution_prmm_minimizer(sphere, x0, de_prmm_compat_cfg);
  std::cerr << "DE-PRMM compat f=" << de_prmm_compat_a.minimum << " ||x||=" << l2_norm(de_prmm_compat_a.minimizer) << '\n';
  expect(std::abs(de_prmm_compat_a.minimum - de_prmm_compat_b.minimum) < 1e-12,
         "DE-PRMM compat reproducibility objective mismatch", &failures);
  expect(as_f64(de_prmm_compat_a.minimizer.sub(de_prmm_compat_b.minimizer).abs().amax(0, false)) < 1e-10,
         "DE-PRMM compat reproducibility minimizer mismatch", &failures);
  expect(de_prmm_compat_a.minimum < 1e-5, "DE-PRMM compat failed sphere accuracy target", &failures);

  ParticleSwarmConfig pso_compat_cfg = pso_cfg;
  pso_compat_cfg.execution_mode = PopulationSolverExecutionMode::OptimMasterCompat;
  pso_compat_cfg.population_size = 80;
  pso_compat_cfg.max_generations = 500;
  pso_compat_cfg.rng_seed = 6003ULL;
  const auto pso_compat_a = particle_swarm_minimizer(sphere, x0, pso_compat_cfg);
  const auto pso_compat_b = particle_swarm_minimizer(sphere, x0, pso_compat_cfg);
  std::cerr << "PSO compat f=" << pso_compat_a.minimum << " ||x||=" << l2_norm(pso_compat_a.minimizer) << '\n';
  expect(std::abs(pso_compat_a.minimum - pso_compat_b.minimum) < 1e-12,
         "PSO compat reproducibility objective mismatch", &failures);
  expect(as_f64(pso_compat_a.minimizer.sub(pso_compat_b.minimizer).abs().amax(0, false)) < 1e-10,
         "PSO compat reproducibility minimizer mismatch", &failures);
  expect(pso_compat_a.minimum < 1e-2, "PSO compat failed sphere accuracy target", &failures);

  ParticleSwarmDvConfig pso_dv_compat_cfg = pso_dv_cfg;
  pso_dv_compat_cfg.execution_mode = PopulationSolverExecutionMode::OptimMasterCompat;
  pso_dv_compat_cfg.population_size = 80;
  pso_dv_compat_cfg.max_generations = 500;
  pso_dv_compat_cfg.rng_seed = 6004ULL;
  const auto pso_dv_compat_a = particle_swarm_dv_minimizer(sphere, x0, pso_dv_compat_cfg);
  const auto pso_dv_compat_b = particle_swarm_dv_minimizer(sphere, x0, pso_dv_compat_cfg);
  std::cerr << "PSO-DV compat f=" << pso_dv_compat_a.minimum << " ||x||=" << l2_norm(pso_dv_compat_a.minimizer) << '\n';
  expect(std::abs(pso_dv_compat_a.minimum - pso_dv_compat_b.minimum) < 1e-12,
         "PSO-DV compat reproducibility objective mismatch", &failures);
  expect(as_f64(pso_dv_compat_a.minimizer.sub(pso_dv_compat_b.minimizer).abs().amax(0, false)) < 1e-10,
         "PSO-DV compat reproducibility minimizer mismatch", &failures);
  expect(pso_dv_compat_a.minimum < 5.0, "PSO-DV compat failed sphere accuracy target", &failures);

  const Series lb = pluss::series::full({dim}, -1.0, cfg);
  const Series ub = pluss::series::full({dim}, 1.0, cfg);
  DifferentialEvolutionConfig de_bound_cfg;
  de_bound_cfg.population_size = 100;
  de_bound_cfg.max_generations = 350;
  de_bound_cfg.check_every = 20;
  de_bound_cfg.rng_seed = 777ULL;
  de_bound_cfg.lower_bounds = lb;
  de_bound_cfg.upper_bounds = ub;
  const auto de_bound_out = differential_evolution_minimizer(
      [](const Series& x) { return shifted_sphere(x, 3.0); }, x0, de_bound_cfg);
  const Series xb = de_bound_out.minimizer.to(pCPU, ScalarType::Float64);
  const double xb_min = as_f64(xb.amin(0, false));
  const double xb_max = as_f64(xb.amax(0, false));
  const double xb_mean = as_f64(xb.mean());
  expect(xb_min >= -1.000001 && xb_max <= 1.000001, "Bounded DE returned value outside bounds", &failures);
  expect(xb_mean > 0.7, "Bounded DE did not approach constrained optimum on upper face", &failures);

  DifferentialEvolutionConfig de_guard_cfg;
  de_guard_cfg.population_size = 80;
  de_guard_cfg.max_generations = 250;
  de_guard_cfg.check_every = 20;
  de_guard_cfg.rng_seed = 909ULL;
  de_guard_cfg.lower_bounds = pluss::series::full({dim}, -2.0, cfg);
  de_guard_cfg.upper_bounds = pluss::series::full({dim}, 2.0, cfg);
  const Series x0_guard = pluss::series::full({dim}, 0.5, cfg);
  const auto de_guard_out = differential_evolution_minimizer(guarded_sphere, x0_guard, de_guard_cfg);
  std::cerr << "DE guarded f=" << de_guard_out.minimum << '\n';
  expect(std::isfinite(de_guard_out.minimum), "DE non-finite objective handling failed", &failures);

  if (failures != 0) {
    std::cerr << "dfo_minimizers_test: " << failures << " failure(s)\n";
    return 1;
  }

  std::cout << "dfo_minimizers_test: all checks passed\n";
  return 0;
}
