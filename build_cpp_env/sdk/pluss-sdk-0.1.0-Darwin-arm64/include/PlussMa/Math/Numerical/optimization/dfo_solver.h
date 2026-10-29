#pragma once

#include "dfo_minimizers.h"
#include "mads_blackbox_minimizers.h"
#include "nelder_mead_minimizer.h"
#include "pattern_search_minimizer.h"

#include <cstdint>
#include <functional>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>

namespace pluss::subsp::math::optimization {

enum class DfoSolverKind {
  Auto,
  NelderMead,
  PatternSearch,
  DifferentialEvolution,
  DifferentialEvolutionPrmm,
  ParticleSwarm,
  ParticleSwarmDv,
  OrthoMads,
  LtMads,
  MadsAdvanced,
};

enum class DfoStatus {
  Converged,
  ReachedIterationBudget,
  ReachedEvaluationBudget,
  ReachedTimeBudget,
  InfeasibleStationary,
};

struct DfoConstraintValues {
  Series inequality{pluss::series::undefined()};
  Series equality{pluss::series::undefined()};
};

struct DfoProblem {
  Series initial{pluss::series::undefined()};
  std::function<Series(const Series&)> objective{};
  std::function<Series(const Series&)> constraints{};
  std::function<DfoConstraintValues(const Series&)> constraints_values{};
  std::function<BlackboxEvaluation(const Series&)> blackbox{};
  Series lower_bounds{pluss::series::undefined()};
  Series upper_bounds{pluss::series::undefined()};
};

struct DfoAutoPolicy {
  std::int64_t low_dimension_threshold{10};
  std::int64_t high_dimension_threshold{40};
  bool prefer_pso_for_high_dimension{true};
};

struct DfoOptions {
  DfoSolverKind solver{DfoSolverKind::Auto};

  std::int64_t max_iterations{0};
  std::int64_t max_evaluations{0};
  std::int64_t max_total_evaluations{0};
  std::int64_t check_every{10};
  double relative_improvement_tolerance{0.0};
  std::uint64_t rng_seed{5489ULL};
  bool promote_cpu_to_float64{true};
  bool display_iter{false};

  DfoAutoPolicy auto_policy{};

  NelderMeadConfig nelder_mead{};
  DifferentialEvolutionConfig differential_evolution{};
  DifferentialEvolutionPrmmConfig differential_evolution_prmm{};
  ParticleSwarmConfig particle_swarm{};
  ParticleSwarmDvConfig particle_swarm_dv{};
  PSConfig pattern_search{};
  MadsBlackboxAdvancedConfig mads{};
};

struct DfoDiagnostics {
  std::int64_t iterations{0};
  std::int64_t evaluations{0};
  std::int64_t total_evaluations{0};
  std::int64_t cache_hits{0};
  std::int64_t outer_iterations{0};
  std::int64_t nups_updates{0};
  std::int64_t search_trials{0};
  std::int64_t search_successes{0};
  double mesh_size{0.0};
  double penalty{0.0};
  double max_constraint_violation{0.0};
  std::string last_phase{};
};

struct DfoResult {
  Series minimizer{pluss::series::undefined()};
  Series objective_vector{pluss::series::undefined()};
  // Mesh methods only: the non-dominated points found (rows) and their objective vectors (rows); undefined otherwise.
  Series pareto_points{pluss::series::undefined()};
  Series pareto_objectives{pluss::series::undefined()};
  double minimum{0.0};
  DfoStatus status{DfoStatus::ReachedIterationBudget};
  DfoSolverKind selected_solver{DfoSolverKind::Auto};
  std::string selection_reason{};
  DfoDiagnostics diagnostics{};
};

class DfoBuilder;

class DFO final {
public:
  explicit DFO(DfoOptions options = {}) : options_(std::move(options)) {}

  [[nodiscard]] DfoResult run(const DfoProblem& problem) const;

  [[nodiscard]] static DfoResult solve(
      const DfoProblem& problem,
      const DfoOptions& options = {}) {
    return DFO(options).run(problem);
  }

  [[nodiscard]] static DfoBuilder build();
  [[nodiscard]] static DfoBuilder build(const Series& initial);

private:
  DfoOptions options_{};
};

class DfoBuilder final {
public:
  DfoBuilder() = default;
  explicit DfoBuilder(DfoProblem problem) : problem_(std::move(problem)) {}
  DfoBuilder(DfoProblem problem, DfoOptions options)
      : problem_(std::move(problem)), options_(std::move(options)) {}

  [[nodiscard]] DfoBuilder with_problem(const DfoProblem& problem) const;
  [[nodiscard]] DfoBuilder with_options(const DfoOptions& options) const;
  [[nodiscard]] DfoBuilder with_initial(const Series& initial) const;
  [[nodiscard]] DfoBuilder with_bounds(const Series& lower, const Series& upper) const;
  [[nodiscard]] DfoBuilder with_solver(DfoSolverKind solver) const;
  [[nodiscard]] DfoBuilder with_seed(std::uint64_t seed) const;
  [[nodiscard]] DfoBuilder with_max_iterations(std::int64_t max_iterations) const;
  [[nodiscard]] DfoBuilder with_max_evaluations(std::int64_t max_evaluations) const;
  [[nodiscard]] DfoBuilder with_max_total_evaluations(std::int64_t max_total_evaluations) const;
  [[nodiscard]] DfoBuilder with_check_every(std::int64_t check_every) const;
  [[nodiscard]] DfoBuilder with_tolerance(double tolerance) const;
  [[nodiscard]] DfoBuilder with_display_iter(bool enabled = true) const;
  [[nodiscard]] DfoBuilder with_promote_cpu_to_float64(bool enabled) const;
  [[nodiscard]] DfoBuilder with_auto_policy(const DfoAutoPolicy& policy) const;
  [[nodiscard]] DfoBuilder with_nelder_mead_config(const NelderMeadConfig& cfg) const;
  [[nodiscard]] DfoBuilder with_de_config(const DifferentialEvolutionConfig& cfg) const;
  [[nodiscard]] DfoBuilder with_de_prmm_config(const DifferentialEvolutionPrmmConfig& cfg) const;
  [[nodiscard]] DfoBuilder with_pso_config(const ParticleSwarmConfig& cfg) const;
  [[nodiscard]] DfoBuilder with_pso_dv_config(const ParticleSwarmDvConfig& cfg) const;
  [[nodiscard]] DfoBuilder with_mads_config(const MadsBlackboxAdvancedConfig& cfg) const;

  [[nodiscard]] DfoBuilder with_objective(std::function<Series(const Series&)> objective) const;
  [[nodiscard]] DfoBuilder with_constraints(std::function<Series(const Series&)> constraints) const;
  [[nodiscard]] DfoBuilder with_constraints_values(
      std::function<DfoConstraintValues(const Series&)> constraints_values) const;
  [[nodiscard]] DfoBuilder with_blackbox(std::function<BlackboxEvaluation(const Series&)> blackbox) const;

  template <class Fn, std::enable_if_t<!std::is_same_v<std::decay_t<Fn>, std::function<Series(const Series&)>>, int> = 0>
  [[nodiscard]] DfoBuilder with_objective(Fn&& objective) const {
    auto wrapped = [fn = std::forward<Fn>(objective)](const Series& x) -> Series {
      using Ret = std::remove_cvref_t<decltype(fn(x))>;
      if constexpr (std::is_same_v<Ret, Series>) {
        return fn(x);
      } else if constexpr (std::is_arithmetic_v<Ret>) {
        return pluss::make_series(static_cast<double>(fn(x)));
      } else {
        static_assert(std::is_arithmetic_v<Ret> || std::is_same_v<Ret, Series>,
                      "DfoBuilder::with_objective expects Series or arithmetic scalar return");
      }
    };
    return with_objective(std::function<Series(const Series&)>(std::move(wrapped)));
  }

  template <class Fn, std::enable_if_t<!std::is_same_v<std::decay_t<Fn>, std::function<Series(const Series&)>>, int> = 0>
  [[nodiscard]] DfoBuilder with_constraints(Fn&& constraints) const {
    auto wrapped = [fn = std::forward<Fn>(constraints)](const Series& x) -> Series {
      using Ret = std::remove_cvref_t<decltype(fn(x))>;
      if constexpr (std::is_same_v<Ret, Series>) {
        return fn(x);
      } else if constexpr (std::is_arithmetic_v<Ret>) {
        return pluss::make_series(static_cast<double>(fn(x)));
      } else {
        static_assert(std::is_arithmetic_v<Ret> || std::is_same_v<Ret, Series>,
                      "DfoBuilder::with_constraints expects Series or arithmetic scalar return");
      }
    };
    return with_constraints(std::function<Series(const Series&)>(std::move(wrapped)));
  }

  template <class Fn, std::enable_if_t<!std::is_same_v<std::decay_t<Fn>, std::function<DfoConstraintValues(const Series&)>>, int> = 0>
  [[nodiscard]] DfoBuilder with_constraints_values(Fn&& constraints_values) const {
    auto wrapped = [fn = std::forward<Fn>(constraints_values)](const Series& x) -> DfoConstraintValues {
      using Ret = std::remove_cvref_t<decltype(fn(x))>;
      if constexpr (std::is_same_v<Ret, DfoConstraintValues>) {
        return fn(x);
      } else if constexpr (std::is_same_v<Ret, Series>) {
        DfoConstraintValues out;
        out.inequality = fn(x);
        return out;
      } else if constexpr (std::is_arithmetic_v<Ret>) {
        DfoConstraintValues out;
        out.inequality = pluss::make_series(static_cast<double>(fn(x)));
        return out;
      } else {
        static_assert(std::is_arithmetic_v<Ret> || std::is_same_v<Ret, Series> ||
                          std::is_same_v<Ret, DfoConstraintValues>,
                      "DfoBuilder::with_constraints_values expects DfoConstraintValues, Series, or arithmetic scalar return");
      }
    };
    return with_constraints_values(
        std::function<DfoConstraintValues(const Series&)>(std::move(wrapped)));
  }

  template <class Fn, std::enable_if_t<!std::is_same_v<std::decay_t<Fn>, std::function<BlackboxEvaluation(const Series&)>>, int> = 0>
  [[nodiscard]] DfoBuilder with_blackbox(Fn&& blackbox) const {
    auto wrapped = [fn = std::forward<Fn>(blackbox)](const Series& x) -> BlackboxEvaluation {
      using Ret = std::remove_cvref_t<decltype(fn(x))>;
      if constexpr (std::is_same_v<Ret, BlackboxEvaluation>) {
        return fn(x);
      } else if constexpr (std::is_same_v<Ret, Series>) {
        BlackboxEvaluation out;
        out.objectives = fn(x);
        return out;
      } else if constexpr (std::is_arithmetic_v<Ret>) {
        BlackboxEvaluation out;
        out.objectives = pluss::make_series(static_cast<double>(fn(x)));
        return out;
      } else {
        static_assert(std::is_arithmetic_v<Ret> || std::is_same_v<Ret, Series> || std::is_same_v<Ret, BlackboxEvaluation>,
                      "DfoBuilder::with_blackbox expects BlackboxEvaluation, Series, or arithmetic scalar return");
      }
    };
    return with_blackbox(std::function<BlackboxEvaluation(const Series&)>(std::move(wrapped)));
  }

  [[nodiscard]] DfoResult solve() const;
  [[nodiscard]] const DfoProblem& problem() const noexcept { return problem_; }
  [[nodiscard]] const DfoOptions& options() const noexcept { return options_; }

private:
  DfoProblem problem_{};
  DfoOptions options_{};
};

std::string to_string(DfoSolverKind solver);
std::string to_string(DfoStatus status);

template <class Fn, std::enable_if_t<!std::is_same_v<std::decay_t<Fn>, std::function<Series(const Series&)>>, int> = 0>
DfoResult solve_dfo(
    Fn&& objective,
    const Series& initial,
    const DfoOptions& options = {}) {
  auto wrapped = [fn = std::forward<Fn>(objective)](const Series& x) -> Series {
    using Ret = std::remove_cvref_t<decltype(fn(x))>;
    if constexpr (std::is_same_v<Ret, Series>) {
      return fn(x);
    } else if constexpr (std::is_arithmetic_v<Ret>) {
      return pluss::make_series(static_cast<double>(fn(x)));
    } else {
      static_assert(std::is_arithmetic_v<Ret> || std::is_same_v<Ret, Series>,
                    "solve_dfo objective must return Series or arithmetic scalar");
    }
  };

  DfoProblem problem;
  problem.initial = initial;
  problem.objective = std::move(wrapped);
  return DFO::solve(problem, options);
}

} // namespace pluss::subsp::math::optimization
