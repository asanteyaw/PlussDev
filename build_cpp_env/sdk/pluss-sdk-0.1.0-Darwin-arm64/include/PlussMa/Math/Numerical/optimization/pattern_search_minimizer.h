#pragma once

#include <pluss/codem/series_factory.h>

#include <cstdint>
#include <functional>
#include <limits>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>

namespace pluss::subsp::math::optimization {

struct PSConstraintValues {
  Series inequality{pluss::series::undefined()};
  Series equality{pluss::series::undefined()};
};

enum class PSStatus {
  Converged,
  ReachedIterationBudget,
  ReachedEvaluationBudget,
  ReachedTimeBudget,
  InfeasibleStationary,
};

enum class PSDisplay {
  None,
  Iter,
};

enum class PSPollPattern {
  PositiveBasis2N,
  PositiveBasisNp1,
};

enum class PSPollingOrder {
  Consecutive,
  Random,
  SuccessFirst,
};

enum class PSSearchStrategy {
  Off,
  ExpandAlongSuccess,
  RandomDirections,
  ExpandAndRandomDirections,
};

enum class PSConstraintPolicy {
  ExtremeBarrier,
  MeritPenalty,
  AugmentedLagrangian,
};

enum class PSMeshScaling {
  Off,
  RangeLog,
  NupsDynamic,
};

enum class PSDirectionFamily {
  ProjectedTangent,
  GSSAugmented,
};

enum class PSBasisFamily {
  Canonical,
  RandomOrtho,
};

struct PSConfig {
  double mesh_tolerance{1e-6};
  double step_tolerance{1e-6};
  double value_tolerance{1e-6};
  double constraint_tolerance{1e-6};
  double active_constraint_tolerance{1e-3};

  std::int64_t max_iterations{0};
  std::int64_t max_evaluations{0};
  double time_limit_seconds{std::numeric_limits<double>::infinity()};

  double initial_mesh_size{1.0};
  double max_mesh_size{std::numeric_limits<double>::infinity()};
  double mesh_expansion{2.0};
  double mesh_contraction{0.5};

  std::int64_t check_every{0};
  double relative_improvement_tolerance{0.0};
  double min_improvement{0.0};

  PSPollPattern poll_pattern{PSPollPattern::PositiveBasis2N};
  PSPollingOrder polling_order{PSPollingOrder::Consecutive};
  bool complete_poll{false};
  PSDirectionFamily direction_family{PSDirectionFamily::GSSAugmented};
  PSBasisFamily basis_family{PSBasisFamily::Canonical};
  std::int64_t basis_refresh_interval{1};

  PSMeshScaling mesh_scaling{PSMeshScaling::RangeLog};
  double nups_expansion{1.6};
  double nups_contraction{0.7};
  double nups_decay{0.98};
  double nups_min_scale{1e-3};
  double nups_max_scale{1e6};

  PSSearchStrategy search_strategy{PSSearchStrategy::ExpandAlongSuccess};
  double search_expansion{2.0};
  std::int64_t search_trials{12};
  bool search_complete{false};

  PSConstraintPolicy constraint_policy{PSConstraintPolicy::MeritPenalty};
  double initial_penalty{10.0};
  double penalty_growth{10.0};
  std::int64_t al_max_outer_iterations{6};
  double al_penalty_growth{5.0};
  double al_violation_reduction_ratio{0.5};
  double al_multiplier_clip{1e8};

  bool enable_cache{true};
  std::int64_t cache_max_entries{10000};
  double cache_tolerance{1e-12};

  Series lower_bounds{pluss::series::undefined()};
  Series upper_bounds{pluss::series::undefined()};

  Series linear_ineq_matrix{pluss::series::undefined()};
  Series linear_ineq_rhs{pluss::series::undefined()};
  Series linear_eq_matrix{pluss::series::undefined()};
  Series linear_eq_rhs{pluss::series::undefined()};

  std::function<PSConstraintValues(const Series&)> nonlinear_constraints{};

  std::uint64_t rng_seed{5489ULL};
  bool promote_cpu_to_float64{true};
  PSDisplay display{PSDisplay::None};
};

struct PSDiagnostics {
  std::int64_t iterations{0};
  std::int64_t evaluations{0};
  std::int64_t cache_hits{0};
  std::int64_t nups_updates{0};
  std::int64_t search_trials{0};
  std::int64_t search_successes{0};
  std::int64_t outer_iterations{0};
  std::string last_step{};
  double mesh_size{0.0};
  double penalty{0.0};
  double max_constraint_violation{0.0};
};

struct PSOutcome {
  Series minimizer{pluss::series::undefined()};
  double minimum{0.0};
  PSStatus status{PSStatus::ReachedIterationBudget};
  PSDiagnostics diagnostics{};
};

PSOutcome pattern_search_minimizer(
    const std::function<Series(const Series&)>& objective,
    const Series& initial,
    const PSConfig& config = {});

std::string to_string(PSStatus status);

template <class Fn, std::enable_if_t<!std::is_same_v<std::decay_t<Fn>, std::function<Series(const Series&)>>, int> = 0>
PSOutcome pattern_search_minimizer(
    Fn&& objective,
    const Series& initial,
    const PSConfig& config = {}) {
  auto wrapped = [fn = std::forward<Fn>(objective)](const Series& x) -> Series {
    using Ret = std::remove_cvref_t<decltype(fn(x))>;
    if constexpr (std::is_same_v<Ret, Series>) {
      const Series out = fn(x);
      if (!out.defined() || out.numel() != 1 || out.dtype().is_complex()) {
        ::pluss::throw_error("pattern_search_minimizer objective must return real scalar Series");
      }
      return out;
    } else if constexpr (std::is_arithmetic_v<Ret>) {
      return pluss::make_series(static_cast<double>(fn(x)));
    } else {
      static_assert(std::is_arithmetic_v<Ret> || std::is_same_v<Ret, Series>,
                    "pattern_search_minimizer objective must return Series or arithmetic scalar");
    }
  };
  std::function<Series(const Series&)> objective_fn(std::move(wrapped));
  return pattern_search_minimizer(objective_fn, initial, config);
}

} // namespace pluss::subsp::math::optimization
