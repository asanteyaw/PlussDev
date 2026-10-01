#pragma once

#include <pluss/codem/series_factory.h>

#include <cstdint>
#include <functional>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>

namespace pluss::subsp::math::optimization {

enum class SolverStatus {
  Converged,
  ReachedIterationBudget,
  ReachedEvaluationBudget,
};

enum class SolverDisplay {
  None,
  Iter,
};

struct NelderMeadConfig {
  double position_tolerance{1e-4};
  double value_tolerance{1e-4};
  std::int64_t max_iterations{0};
  std::int64_t max_evaluations{0};
  double reflection{1.0};
  double expansion{2.0};
  double contraction{0.5};
  double shrink{0.5};
  double simplex_delta{0.05};
  double simplex_delta_zero{0.00025};
  bool promote_cpu_to_float64{true};
  SolverDisplay display{SolverDisplay::None};
};

struct NelderMeadDiagnostics {
  std::int64_t iterations{0};
  std::int64_t evaluations{0};
  std::string last_step{};
};

struct NelderMeadOutcome {
  Series minimizer{pluss::series::undefined()};
  double minimum{0.0};
  SolverStatus status{SolverStatus::ReachedIterationBudget};
  NelderMeadDiagnostics diagnostics{};
};

NelderMeadOutcome nelder_mead_minimizer(
    const std::function<Series(const Series&)>& objective,
    const Series& initial,
    const NelderMeadConfig& config = {});

template <class Fn, std::enable_if_t<!std::is_same_v<std::decay_t<Fn>, std::function<Series(const Series&)>>, int> = 0>
NelderMeadOutcome nelder_mead_minimizer(
    Fn&& objective,
    const Series& initial,
    const NelderMeadConfig& config = {}) {
  auto wrapped = [fn = std::forward<Fn>(objective)](const Series& x) -> Series {
    using Ret = std::remove_cvref_t<decltype(fn(x))>;
    if constexpr (std::is_same_v<Ret, Series>) {
      const Series out = fn(x);
      if (!out.defined() || out.numel() != 1) {
        ::pluss::throw_error(
            "subsp::math::optimization::nelder_mead_minimizer: objective must return a scalar value");
      }
      if (out.dtype().is_complex()) {
        ::pluss::throw_error(
            "subsp::math::optimization::nelder_mead_minimizer: objective must return a real scalar");
      }
      return out;
    } else if constexpr (std::is_arithmetic_v<Ret>) {
      return pluss::make_series(static_cast<double>(fn(x)));
    } else {
      static_assert(std::is_arithmetic_v<Ret> || std::is_same_v<Ret, Series>,
                    "nelder_mead_minimizer objective must return Series or arithmetic scalar");
    }
  };

  std::function<Series(const Series&)> objective_fn(std::move(wrapped));
  return nelder_mead_minimizer(objective_fn, initial, config);
}

} // namespace pluss::subsp::math::optimization
