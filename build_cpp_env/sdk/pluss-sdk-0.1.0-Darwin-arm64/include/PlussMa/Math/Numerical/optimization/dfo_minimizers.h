#pragma once

#include <pluss/codem/series_factory.h>

#include <cstdint>
#include <limits>
#include <functional>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>

namespace pluss::subsp::math::optimization {

enum class PopulationSolverStatus {
  Converged,
  ReachedIterationBudget,
  ReachedEvaluationBudget,
};

inline std::string to_string(PopulationSolverStatus status) {
  switch (status) {
    case PopulationSolverStatus::Converged:
      return "Converged";
    case PopulationSolverStatus::ReachedIterationBudget:
      return "ReachedIterationBudget";
    case PopulationSolverStatus::ReachedEvaluationBudget:
      return "ReachedEvaluationBudget";
  }
  return "Unknown";
}

enum class PopulationSolverDisplay {
  None,
  Iter,
};

enum class PopulationSolverExecutionMode {
  Vectorized,
  OptimMasterCompat,
};

struct PopulationSolverDiagnostics {
  std::int64_t iterations{0};
  std::int64_t evaluations{0};
  std::string last_step{};
};

struct PopulationSolverOutcome {
  Series minimizer{pluss::series::undefined()};
  double minimum{0.0};
  PopulationSolverStatus status{PopulationSolverStatus::ReachedIterationBudget};
  PopulationSolverDiagnostics diagnostics{};
};

struct DifferentialEvolutionConfig {
  std::int64_t population_size{200};
  std::int64_t max_generations{1000};
  std::int64_t max_evaluations{0};
  std::int64_t check_every{0};
  double relative_improvement_tolerance{1e-8};

  double mutation_factor{0.8};
  double crossover_rate{0.9};
  double best_mix{0.0};

  double initial_radius{std::numeric_limits<double>::infinity()};  // infinity: the whole box if bounded, else start +- 0.5
  Series lower_bounds{pluss::series::undefined()};
  Series upper_bounds{pluss::series::undefined()};

  std::uint64_t rng_seed{5489ULL};
  bool promote_cpu_to_float64{true};
  PopulationSolverDisplay display{PopulationSolverDisplay::None};
  PopulationSolverExecutionMode execution_mode{PopulationSolverExecutionMode::Vectorized};
};

struct DifferentialEvolutionPrmmConfig {
  std::int64_t population_size{200};
  std::int64_t elite_count{6};
  std::int64_t max_generations{0};
  std::int64_t max_evaluations{100000};
  std::int64_t check_every{0};
  std::int64_t reduction_passes{3};
  std::int64_t pmax{4};
  double relative_improvement_tolerance{1e-8};

  double initial_mutation_factor{0.8};
  double initial_crossover_rate{0.9};
  double mutation_lower{0.1};
  double mutation_upper{1.0};
  double tau_mutation{0.1};
  double tau_crossover{0.1};
  double random_main_strategy_probability{0.75};
  double diversity_epsilon{0.5};

  double initial_radius{std::numeric_limits<double>::infinity()};  // infinity: the whole box if bounded, else start +- 0.5
  Series lower_bounds{pluss::series::undefined()};
  Series upper_bounds{pluss::series::undefined()};

  std::uint64_t rng_seed{5489ULL};
  bool promote_cpu_to_float64{true};
  PopulationSolverDisplay display{PopulationSolverDisplay::None};
  PopulationSolverExecutionMode execution_mode{PopulationSolverExecutionMode::Vectorized};
};

struct ParticleSwarmConfig {
  std::int64_t population_size{200};
  std::int64_t max_generations{1000};
  std::int64_t max_evaluations{0};
  std::int64_t check_every{0};
  double relative_improvement_tolerance{1e-8};

  bool center_particle{true};
  bool linear_inertia_schedule{true};
  bool time_varying_acceleration{false};
  double inertia{1.0};
  double inertia_max{0.99};
  double inertia_min{0.10};
  double inertia_damp{0.99};
  double cognitive{2.0};
  double social{2.0};
  double final_cognitive{0.5};
  double final_social{2.5};
  double velocity_clamp_ratio{0.0};

  double initial_radius{std::numeric_limits<double>::infinity()};  // infinity: the whole box if bounded, else start +- 0.5
  Series lower_bounds{pluss::series::undefined()};
  Series upper_bounds{pluss::series::undefined()};

  std::uint64_t rng_seed{5489ULL};
  bool promote_cpu_to_float64{true};
  PopulationSolverDisplay display{PopulationSolverDisplay::None};
  PopulationSolverExecutionMode execution_mode{PopulationSolverExecutionMode::Vectorized};
};

struct ParticleSwarmDvConfig {
  std::int64_t population_size{200};
  std::int64_t max_generations{1000};
  std::int64_t max_evaluations{0};
  std::int64_t check_every{0};
  double relative_improvement_tolerance{1e-8};

  double inertia{1.0};
  double inertia_damp{0.99};
  double beta{0.5};
  double social{1.494};
  double crossover_rate{0.7};
  std::int64_t stagnation_limit{50};

  double initial_radius{std::numeric_limits<double>::infinity()};  // infinity: the whole box if bounded, else start +- 0.5
  Series lower_bounds{pluss::series::undefined()};
  Series upper_bounds{pluss::series::undefined()};

  std::uint64_t rng_seed{5489ULL};
  bool promote_cpu_to_float64{true};
  PopulationSolverDisplay display{PopulationSolverDisplay::None};
  PopulationSolverExecutionMode execution_mode{PopulationSolverExecutionMode::Vectorized};
};

PopulationSolverOutcome differential_evolution_minimizer(
    const std::function<Series(const Series&)>& objective,
    const Series& initial,
    const DifferentialEvolutionConfig& config = {});

PopulationSolverOutcome differential_evolution_prmm_minimizer(
    const std::function<Series(const Series&)>& objective,
    const Series& initial,
    const DifferentialEvolutionPrmmConfig& config = {});

PopulationSolverOutcome particle_swarm_minimizer(
    const std::function<Series(const Series&)>& objective,
    const Series& initial,
    const ParticleSwarmConfig& config = {});

PopulationSolverOutcome particle_swarm_dv_minimizer(
    const std::function<Series(const Series&)>& objective,
    const Series& initial,
    const ParticleSwarmDvConfig& config = {});

template <class Fn, std::enable_if_t<!std::is_same_v<std::decay_t<Fn>, std::function<Series(const Series&)>>, int> = 0>
PopulationSolverOutcome differential_evolution_minimizer(
    Fn&& objective,
    const Series& initial,
    const DifferentialEvolutionConfig& config = {}) {
  auto wrapped = [fn = std::forward<Fn>(objective)](const Series& x) -> Series {
    using Ret = std::remove_cvref_t<decltype(fn(x))>;
    if constexpr (std::is_same_v<Ret, Series>) {
      const Series out = fn(x);
      if (!out.defined() || out.numel() != 1 || out.dtype().is_complex()) {
        ::pluss::throw_error("differential_evolution_minimizer objective must return real scalar Series");
      }
      return out;
    } else if constexpr (std::is_arithmetic_v<Ret>) {
      return pluss::make_series(static_cast<double>(fn(x)));
    } else {
      static_assert(std::is_arithmetic_v<Ret> || std::is_same_v<Ret, Series>,
                    "differential_evolution_minimizer objective must return Series or arithmetic scalar");
    }
  };
  std::function<Series(const Series&)> objective_fn(std::move(wrapped));
  return differential_evolution_minimizer(objective_fn, initial, config);
}

template <class Fn, std::enable_if_t<!std::is_same_v<std::decay_t<Fn>, std::function<Series(const Series&)>>, int> = 0>
PopulationSolverOutcome differential_evolution_prmm_minimizer(
    Fn&& objective,
    const Series& initial,
    const DifferentialEvolutionPrmmConfig& config = {}) {
  auto wrapped = [fn = std::forward<Fn>(objective)](const Series& x) -> Series {
    using Ret = std::remove_cvref_t<decltype(fn(x))>;
    if constexpr (std::is_same_v<Ret, Series>) {
      const Series out = fn(x);
      if (!out.defined() || out.numel() != 1 || out.dtype().is_complex()) {
        ::pluss::throw_error("differential_evolution_prmm_minimizer objective must return real scalar Series");
      }
      return out;
    } else if constexpr (std::is_arithmetic_v<Ret>) {
      return pluss::make_series(static_cast<double>(fn(x)));
    } else {
      static_assert(std::is_arithmetic_v<Ret> || std::is_same_v<Ret, Series>,
                    "differential_evolution_prmm_minimizer objective must return Series or arithmetic scalar");
    }
  };
  std::function<Series(const Series&)> objective_fn(std::move(wrapped));
  return differential_evolution_prmm_minimizer(objective_fn, initial, config);
}

template <class Fn, std::enable_if_t<!std::is_same_v<std::decay_t<Fn>, std::function<Series(const Series&)>>, int> = 0>
PopulationSolverOutcome particle_swarm_minimizer(
    Fn&& objective,
    const Series& initial,
    const ParticleSwarmConfig& config = {}) {
  auto wrapped = [fn = std::forward<Fn>(objective)](const Series& x) -> Series {
    using Ret = std::remove_cvref_t<decltype(fn(x))>;
    if constexpr (std::is_same_v<Ret, Series>) {
      const Series out = fn(x);
      if (!out.defined() || out.numel() != 1 || out.dtype().is_complex()) {
        ::pluss::throw_error("particle_swarm_minimizer objective must return real scalar Series");
      }
      return out;
    } else if constexpr (std::is_arithmetic_v<Ret>) {
      return pluss::make_series(static_cast<double>(fn(x)));
    } else {
      static_assert(std::is_arithmetic_v<Ret> || std::is_same_v<Ret, Series>,
                    "particle_swarm_minimizer objective must return Series or arithmetic scalar");
    }
  };
  std::function<Series(const Series&)> objective_fn(std::move(wrapped));
  return particle_swarm_minimizer(objective_fn, initial, config);
}

template <class Fn, std::enable_if_t<!std::is_same_v<std::decay_t<Fn>, std::function<Series(const Series&)>>, int> = 0>
PopulationSolverOutcome particle_swarm_dv_minimizer(
    Fn&& objective,
    const Series& initial,
    const ParticleSwarmDvConfig& config = {}) {
  auto wrapped = [fn = std::forward<Fn>(objective)](const Series& x) -> Series {
    using Ret = std::remove_cvref_t<decltype(fn(x))>;
    if constexpr (std::is_same_v<Ret, Series>) {
      const Series out = fn(x);
      if (!out.defined() || out.numel() != 1 || out.dtype().is_complex()) {
        ::pluss::throw_error("particle_swarm_dv_minimizer objective must return real scalar Series");
      }
      return out;
    } else if constexpr (std::is_arithmetic_v<Ret>) {
      return pluss::make_series(static_cast<double>(fn(x)));
    } else {
      static_assert(std::is_arithmetic_v<Ret> || std::is_same_v<Ret, Series>,
                    "particle_swarm_dv_minimizer objective must return Series or arithmetic scalar");
    }
  };
  std::function<Series(const Series&)> objective_fn(std::move(wrapped));
  return particle_swarm_dv_minimizer(objective_fn, initial, config);
}

} // namespace pluss::subsp::math::optimization
