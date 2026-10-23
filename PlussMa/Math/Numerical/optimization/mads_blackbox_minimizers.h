#pragma once

#include <pluss/codem/series_factory.h>

#include <cstdint>
#include <functional>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>

namespace pluss::subsp::math::optimization {

enum class BlackboxSolverStatus {
  Converged,
  ReachedIterationBudget,
  ReachedEvaluationBudget,
};

enum class BlackboxSolverDisplay {
  None,
  Iter,
};

enum class MadsPollScheme {
  Ortho2N,
  LtMadsRandom2N,
};

enum class ConstraintHandling {
  Ignore,
  ExtremeBarrier,
  ProgressiveBarrier,
};

enum class MultiObjectiveScalarization {
  WeightedSum,
  Chebyshev,
};

enum class SurrogateMode {
  Off,
  CandidateOrdering,
};

struct BlackboxSolverDiagnostics {
  std::int64_t iterations{0};
  std::int64_t evaluations{0};
  std::string last_phase{};
  double mesh_size{0.0};
  double frame_size{0.0};
};

struct BlackboxAdvancedDiagnostics {
  std::int64_t iterations{0};
  std::int64_t evaluations{0};
  std::int64_t total_evaluations{0};
  std::int64_t cache_hits{0};
  std::int64_t surrogate_orderings{0};
  std::int64_t pareto_size{0};
  std::int64_t objective_dimension{1};
  std::int64_t constraint_dimension{0};
  std::string last_phase{};
  double mesh_size{0.0};
  double frame_size{0.0};
};

struct BlackboxSolverOutcome {
  Series minimizer{pluss::series::undefined()};
  double minimum{0.0};
  BlackboxSolverStatus status{BlackboxSolverStatus::ReachedIterationBudget};
  BlackboxSolverDiagnostics diagnostics{};
};

struct BlackboxAdvancedOutcome {
  Series minimizer{pluss::series::undefined()};
  Series objective_vector{pluss::series::undefined()};
  double scalarized_minimum{0.0};
  BlackboxSolverStatus status{BlackboxSolverStatus::ReachedIterationBudget};
  BlackboxAdvancedDiagnostics diagnostics{};
  Series pareto_points{pluss::series::undefined()};
  Series pareto_objectives{pluss::series::undefined()};
};

struct BlackboxEvaluation {
  Series objectives{pluss::series::undefined()};
  Series constraints{pluss::series::undefined()};
  bool evaluation_ok{true};
  bool count_evaluation{true};
};

struct MadsBlackboxConfig {
  std::int64_t max_iterations{0};
  std::int64_t max_evaluations{0};
  std::int64_t check_every{10};
  std::int64_t search_points{0};
  double relative_improvement_tolerance{1e-8};

  double initial_mesh_size{0.0};
  double initial_frame_size{0.0};
  // After an unsuccessful iteration the poll (frame) size is multiplied by frame_refine_factor and the mesh by
  // mesh_refine_factor; after a success they are enlarged by the expand factors. The mesh refines faster than the frame (the
  // mesh shrinks like the square of the poll size in the MADS theory) and is never larger than the frame, which is what makes
  // the poll directions dense as the iteration converges.
  double mesh_refine_factor{0.25};
  double frame_refine_factor{0.5};
  double mesh_expand_factor{4.0};
  double frame_expand_factor{2.0};
  double mesh_tolerance{1e-8};  // the iteration has converged when the poll (frame) size, the accuracy it can still resolve, is this small
  double min_improvement{0.0};
  bool opportunistic{true};
  MadsPollScheme poll_scheme{MadsPollScheme::Ortho2N};

  Series lower_bounds{pluss::series::undefined()};
  Series upper_bounds{pluss::series::undefined()};

  std::uint64_t rng_seed{5489ULL};
  bool promote_cpu_to_float64{true};
  BlackboxSolverDisplay display{BlackboxSolverDisplay::None};
};

struct MadsBlackboxAdvancedConfig : MadsBlackboxConfig {
  ConstraintHandling constraint_handling{ConstraintHandling::Ignore};
  double constraint_tolerance{1e-12};
  double progressive_barrier_penalty{100.0};

  MultiObjectiveScalarization multiobjective_scalarization{
      MultiObjectiveScalarization::WeightedSum};
  Series objective_weights{pluss::series::undefined()};
  Series objective_reference{pluss::series::undefined()};

  bool enable_cache{true};
  std::int64_t cache_max_entries{200000};
  std::int64_t cache_round_digits{12};
  bool count_cache_hits_in_total_evaluations{true};
  std::int64_t max_total_evaluations{0};

  SurrogateMode surrogate_mode{SurrogateMode::Off};
  std::int64_t surrogate_min_points{24};
  std::int64_t surrogate_max_points{512};
  double surrogate_bandwidth{0.0};
  bool accelerate_heavy_kernels{true};
  bool auto_heavy_platform{true};
  Platform heavy_platform{pluss::pCPU};
  ScalarType heavy_dtype{ScalarType::Float32};

  double pareto_epsilon{1e-12};
  std::int64_t pareto_archive_limit{256};
};

struct CoopMadsConfig {
  MadsBlackboxAdvancedConfig base{};
  std::int64_t cooperative_rounds{4};
  // Share of the budget kept for a final full-space run from the best point (0 = none); blocks cannot see the coupling
  // between variables in different blocks, and this run removes the error that coupling leaves.
  double polish_fraction{0.25};
  std::int64_t subproblem_size{0};
  std::int64_t subproblems_per_round{0};
  bool randomized_subproblems{true};
  std::uint64_t rng_seed{5489ULL};
};

struct PsdMadsConfig {
  MadsBlackboxAdvancedConfig base{};
  std::int64_t rounds{5};
  double polish_fraction{0.25};  // share of the budget kept for a final full-space run from the best point (0 = none)
  std::int64_t min_subproblem_size{2};
  std::int64_t max_subproblem_size{0};
  std::int64_t subproblems_per_round{4};
  bool randomized_subproblems{true};
  std::uint64_t rng_seed{5489ULL};
};

struct DiscoMadsConfig {
  MadsBlackboxAdvancedConfig base{};
  std::int64_t rescue_trials{8};
  // The rescue only runs once this share of the evaluations has failed (after a warm-up of 20 calls); below it, plain MADS
  // copes with the failures and the extra calls would be wasted. 0 rescues every failure.
  double rescue_failure_rate{0.5};
  double rescue_radius{0.25};
  double rescue_radius_decay{0.75};
  std::uint64_t rng_seed{5489ULL};
};

struct VnsMadsConfig {
  MadsBlackboxAdvancedConfig base{};
  std::int64_t neighborhoods{5};
  double initial_kick_scale{0.2};
  double kick_growth{1.7};
  std::uint64_t rng_seed{5489ULL};
};

BlackboxSolverOutcome mesh_adaptive_direct_search_minimizer(
    const std::function<Series(const Series&)>& objective,
    const Series& initial,
    const MadsBlackboxConfig& config = {});

BlackboxSolverOutcome orthomads_minimizer(
    const std::function<Series(const Series&)>& objective,
    const Series& initial,
    const MadsBlackboxConfig& config = {});

BlackboxSolverOutcome ltmads_minimizer(
    const std::function<Series(const Series&)>& objective,
    const Series& initial,
    const MadsBlackboxConfig& config = {});

BlackboxAdvancedOutcome mads_blackbox_minimizer(
    const std::function<BlackboxEvaluation(const Series&)>& blackbox,
    const Series& initial,
    const MadsBlackboxAdvancedConfig& config = {});

BlackboxAdvancedOutcome orthomads_blackbox_minimizer(
    const std::function<BlackboxEvaluation(const Series&)>& blackbox,
    const Series& initial,
    const MadsBlackboxAdvancedConfig& config = {});

BlackboxAdvancedOutcome ltmads_blackbox_minimizer(
    const std::function<BlackboxEvaluation(const Series&)>& blackbox,
    const Series& initial,
    const MadsBlackboxAdvancedConfig& config = {});

BlackboxAdvancedOutcome coopmads_blackbox_minimizer(
    const std::function<BlackboxEvaluation(const Series&)>& blackbox,
    const Series& initial,
    const CoopMadsConfig& config = {});

BlackboxAdvancedOutcome psdmads_blackbox_minimizer(
    const std::function<BlackboxEvaluation(const Series&)>& blackbox,
    const Series& initial,
    const PsdMadsConfig& config = {});

BlackboxAdvancedOutcome discomads_blackbox_minimizer(
    const std::function<BlackboxEvaluation(const Series&)>& blackbox,
    const Series& initial,
    const DiscoMadsConfig& config = {});

BlackboxAdvancedOutcome vnsmads_blackbox_minimizer(
    const std::function<BlackboxEvaluation(const Series&)>& blackbox,
    const Series& initial,
    const VnsMadsConfig& config = {});

template <class Fn, std::enable_if_t<!std::is_same_v<std::decay_t<Fn>, std::function<Series(const Series&)>>, int> = 0>
BlackboxSolverOutcome mesh_adaptive_direct_search_minimizer(
    Fn&& objective,
    const Series& initial,
    const MadsBlackboxConfig& config = {}) {
  auto wrapped = [fn = std::forward<Fn>(objective)](const Series& x) -> Series {
    using Ret = std::remove_cvref_t<decltype(fn(x))>;
    if constexpr (std::is_same_v<Ret, Series>) {
      const Series out = fn(x);
      if (!out.defined() || out.numel() != 1 || out.dtype().is_complex()) {
        ::pluss::throw_error(
            "mesh_adaptive_direct_search_minimizer objective must return real scalar Series");
      }
      return out;
    } else if constexpr (std::is_arithmetic_v<Ret>) {
      return pluss::make_series(static_cast<double>(fn(x)));
    } else {
      static_assert(std::is_arithmetic_v<Ret> || std::is_same_v<Ret, Series>,
                    "mesh_adaptive_direct_search_minimizer objective must return Series or arithmetic scalar");
    }
  };
  std::function<Series(const Series&)> objective_fn(std::move(wrapped));
  return mesh_adaptive_direct_search_minimizer(objective_fn, initial, config);
}

template <class Fn, std::enable_if_t<!std::is_same_v<std::decay_t<Fn>, std::function<Series(const Series&)>>, int> = 0>
BlackboxSolverOutcome orthomads_minimizer(
    Fn&& objective,
    const Series& initial,
    const MadsBlackboxConfig& config = {}) {
  auto wrapped = [fn = std::forward<Fn>(objective)](const Series& x) -> Series {
    using Ret = std::remove_cvref_t<decltype(fn(x))>;
    if constexpr (std::is_same_v<Ret, Series>) {
      const Series out = fn(x);
      if (!out.defined() || out.numel() != 1 || out.dtype().is_complex()) {
        ::pluss::throw_error("orthomads_minimizer objective must return real scalar Series");
      }
      return out;
    } else if constexpr (std::is_arithmetic_v<Ret>) {
      return pluss::make_series(static_cast<double>(fn(x)));
    } else {
      static_assert(std::is_arithmetic_v<Ret> || std::is_same_v<Ret, Series>,
                    "orthomads_minimizer objective must return Series or arithmetic scalar");
    }
  };
  std::function<Series(const Series&)> objective_fn(std::move(wrapped));
  return orthomads_minimizer(objective_fn, initial, config);
}

template <class Fn, std::enable_if_t<!std::is_same_v<std::decay_t<Fn>, std::function<Series(const Series&)>>, int> = 0>
BlackboxSolverOutcome ltmads_minimizer(
    Fn&& objective,
    const Series& initial,
    const MadsBlackboxConfig& config = {}) {
  auto wrapped = [fn = std::forward<Fn>(objective)](const Series& x) -> Series {
    using Ret = std::remove_cvref_t<decltype(fn(x))>;
    if constexpr (std::is_same_v<Ret, Series>) {
      const Series out = fn(x);
      if (!out.defined() || out.numel() != 1 || out.dtype().is_complex()) {
        ::pluss::throw_error("ltmads_minimizer objective must return real scalar Series");
      }
      return out;
    } else if constexpr (std::is_arithmetic_v<Ret>) {
      return pluss::make_series(static_cast<double>(fn(x)));
    } else {
      static_assert(std::is_arithmetic_v<Ret> || std::is_same_v<Ret, Series>,
                    "ltmads_minimizer objective must return Series or arithmetic scalar");
    }
  };
  std::function<Series(const Series&)> objective_fn(std::move(wrapped));
  return ltmads_minimizer(objective_fn, initial, config);
}

template <class Fn, std::enable_if_t<!std::is_same_v<std::decay_t<Fn>, std::function<BlackboxEvaluation(const Series&)>>, int> = 0>
BlackboxAdvancedOutcome mads_blackbox_minimizer(
    Fn&& blackbox,
    const Series& initial,
    const MadsBlackboxAdvancedConfig& config = {}) {
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
      static_assert(std::is_arithmetic_v<Ret> || std::is_same_v<Ret, Series> ||
                        std::is_same_v<Ret, BlackboxEvaluation>,
                    "mads_blackbox_minimizer blackbox must return BlackboxEvaluation, Series, or arithmetic scalar");
    }
  };
  std::function<BlackboxEvaluation(const Series&)> blackbox_fn(std::move(wrapped));
  return mads_blackbox_minimizer(blackbox_fn, initial, config);
}

template <class Fn, std::enable_if_t<!std::is_same_v<std::decay_t<Fn>, std::function<BlackboxEvaluation(const Series&)>>, int> = 0>
BlackboxAdvancedOutcome orthomads_blackbox_minimizer(
    Fn&& blackbox,
    const Series& initial,
    const MadsBlackboxAdvancedConfig& config = {}) {
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
      static_assert(std::is_arithmetic_v<Ret> || std::is_same_v<Ret, Series> ||
                        std::is_same_v<Ret, BlackboxEvaluation>,
                    "orthomads_blackbox_minimizer blackbox must return BlackboxEvaluation, Series, or arithmetic scalar");
    }
  };
  std::function<BlackboxEvaluation(const Series&)> blackbox_fn(std::move(wrapped));
  return orthomads_blackbox_minimizer(blackbox_fn, initial, config);
}

template <class Fn, std::enable_if_t<!std::is_same_v<std::decay_t<Fn>, std::function<BlackboxEvaluation(const Series&)>>, int> = 0>
BlackboxAdvancedOutcome ltmads_blackbox_minimizer(
    Fn&& blackbox,
    const Series& initial,
    const MadsBlackboxAdvancedConfig& config = {}) {
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
      static_assert(std::is_arithmetic_v<Ret> || std::is_same_v<Ret, Series> ||
                        std::is_same_v<Ret, BlackboxEvaluation>,
                    "ltmads_blackbox_minimizer blackbox must return BlackboxEvaluation, Series, or arithmetic scalar");
    }
  };
  std::function<BlackboxEvaluation(const Series&)> blackbox_fn(std::move(wrapped));
  return ltmads_blackbox_minimizer(blackbox_fn, initial, config);
}

template <class Fn, std::enable_if_t<!std::is_same_v<std::decay_t<Fn>, std::function<BlackboxEvaluation(const Series&)>>, int> = 0>
BlackboxAdvancedOutcome coopmads_blackbox_minimizer(
    Fn&& blackbox,
    const Series& initial,
    const CoopMadsConfig& config = {}) {
  auto wrapped = [fn = std::forward<Fn>(blackbox)](const Series& x) -> BlackboxEvaluation {
    using Ret = std::remove_cvref_t<decltype(fn(x))>;
    if constexpr (std::is_same_v<Ret, BlackboxEvaluation>) return fn(x);
    if constexpr (std::is_same_v<Ret, Series>) {
      BlackboxEvaluation out;
      out.objectives = fn(x);
      return out;
    }
    if constexpr (std::is_arithmetic_v<Ret>) {
      BlackboxEvaluation out;
      out.objectives = pluss::make_series(static_cast<double>(fn(x)));
      return out;
    }
    static_assert(std::is_arithmetic_v<Ret> || std::is_same_v<Ret, Series> || std::is_same_v<Ret, BlackboxEvaluation>,
                  "coopmads_blackbox_minimizer blackbox must return BlackboxEvaluation, Series, or arithmetic scalar");
  };
  std::function<BlackboxEvaluation(const Series&)> blackbox_fn(std::move(wrapped));
  return coopmads_blackbox_minimizer(blackbox_fn, initial, config);
}

template <class Fn, std::enable_if_t<!std::is_same_v<std::decay_t<Fn>, std::function<BlackboxEvaluation(const Series&)>>, int> = 0>
BlackboxAdvancedOutcome psdmads_blackbox_minimizer(
    Fn&& blackbox,
    const Series& initial,
    const PsdMadsConfig& config = {}) {
  auto wrapped = [fn = std::forward<Fn>(blackbox)](const Series& x) -> BlackboxEvaluation {
    using Ret = std::remove_cvref_t<decltype(fn(x))>;
    if constexpr (std::is_same_v<Ret, BlackboxEvaluation>) return fn(x);
    if constexpr (std::is_same_v<Ret, Series>) {
      BlackboxEvaluation out;
      out.objectives = fn(x);
      return out;
    }
    if constexpr (std::is_arithmetic_v<Ret>) {
      BlackboxEvaluation out;
      out.objectives = pluss::make_series(static_cast<double>(fn(x)));
      return out;
    }
    static_assert(std::is_arithmetic_v<Ret> || std::is_same_v<Ret, Series> || std::is_same_v<Ret, BlackboxEvaluation>,
                  "psdmads_blackbox_minimizer blackbox must return BlackboxEvaluation, Series, or arithmetic scalar");
  };
  std::function<BlackboxEvaluation(const Series&)> blackbox_fn(std::move(wrapped));
  return psdmads_blackbox_minimizer(blackbox_fn, initial, config);
}

template <class Fn, std::enable_if_t<!std::is_same_v<std::decay_t<Fn>, std::function<BlackboxEvaluation(const Series&)>>, int> = 0>
BlackboxAdvancedOutcome discomads_blackbox_minimizer(
    Fn&& blackbox,
    const Series& initial,
    const DiscoMadsConfig& config = {}) {
  auto wrapped = [fn = std::forward<Fn>(blackbox)](const Series& x) -> BlackboxEvaluation {
    using Ret = std::remove_cvref_t<decltype(fn(x))>;
    if constexpr (std::is_same_v<Ret, BlackboxEvaluation>) return fn(x);
    if constexpr (std::is_same_v<Ret, Series>) {
      BlackboxEvaluation out;
      out.objectives = fn(x);
      return out;
    }
    if constexpr (std::is_arithmetic_v<Ret>) {
      BlackboxEvaluation out;
      out.objectives = pluss::make_series(static_cast<double>(fn(x)));
      return out;
    }
    static_assert(std::is_arithmetic_v<Ret> || std::is_same_v<Ret, Series> || std::is_same_v<Ret, BlackboxEvaluation>,
                  "discomads_blackbox_minimizer blackbox must return BlackboxEvaluation, Series, or arithmetic scalar");
  };
  std::function<BlackboxEvaluation(const Series&)> blackbox_fn(std::move(wrapped));
  return discomads_blackbox_minimizer(blackbox_fn, initial, config);
}

template <class Fn, std::enable_if_t<!std::is_same_v<std::decay_t<Fn>, std::function<BlackboxEvaluation(const Series&)>>, int> = 0>
BlackboxAdvancedOutcome vnsmads_blackbox_minimizer(
    Fn&& blackbox,
    const Series& initial,
    const VnsMadsConfig& config = {}) {
  auto wrapped = [fn = std::forward<Fn>(blackbox)](const Series& x) -> BlackboxEvaluation {
    using Ret = std::remove_cvref_t<decltype(fn(x))>;
    if constexpr (std::is_same_v<Ret, BlackboxEvaluation>) return fn(x);
    if constexpr (std::is_same_v<Ret, Series>) {
      BlackboxEvaluation out;
      out.objectives = fn(x);
      return out;
    }
    if constexpr (std::is_arithmetic_v<Ret>) {
      BlackboxEvaluation out;
      out.objectives = pluss::make_series(static_cast<double>(fn(x)));
      return out;
    }
    static_assert(std::is_arithmetic_v<Ret> || std::is_same_v<Ret, Series> || std::is_same_v<Ret, BlackboxEvaluation>,
                  "vnsmads_blackbox_minimizer blackbox must return BlackboxEvaluation, Series, or arithmetic scalar");
  };
  std::function<BlackboxEvaluation(const Series&)> blackbox_fn(std::move(wrapped));
  return vnsmads_blackbox_minimizer(blackbox_fn, initial, config);
}

} // namespace pluss::subsp::math::optimization
