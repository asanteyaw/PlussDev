#include "nelder_mead_minimizer.h"

#include "pluss/tils/aux/UseAux.h"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <limits>
#include <numeric>
#include <span>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace pluss::subsp::math::optimization {
namespace {

struct SimplexState {
  std::vector<Series> vertices{};
  std::vector<Series> values{};
  std::int64_t iterations{0};
  std::int64_t evaluations{0};
  std::string last_step{};
};

double scalar_as_f64(const Series& x, const char* op) {
  if (!x.defined() || x.numel() != 1) {
    ::pluss::throw_error(std::string(op) + ": expected scalar Series");
  }
  return x.to(ScalarType::Float64).item<double>();
}

void require_valid_initial(const Series& initial) {
  if (!initial.defined()) {
    ::pluss::throw_error("subsp::math::optimization::nelder_mead_minimizer: initial point is undefined");
  }
  if (initial.numel() < 1) {
    ::pluss::throw_error("subsp::math::optimization::nelder_mead_minimizer: initial point must be non-empty");
  }
  if (!initial.dtype().is_floating() || initial.dtype().is_complex()) {
    ::pluss::throw_error(
        "subsp::math::optimization::nelder_mead_minimizer: initial point must be real floating-point");
  }
}

double machine_gap(double x) {
  const double ax = std::abs(x);
  const double next = std::nextafter(ax, std::numeric_limits<double>::infinity());
  return next - ax;
}

void maybe_print_iteration(const SimplexState& state, const NelderMeadConfig& config) {
  if (config.display != SolverDisplay::Iter) {
    return;
  }
  std::cout << "iter=" << state.iterations
            << " evals=" << state.evaluations
            << " f=" << scalar_as_f64(state.values.front(), "nelder_mead_minimizer")
            << " phase=" << state.last_step << "\n";
}

SimplexState sort_state(SimplexState state) {
  std::vector<std::size_t> order(state.values.size());
  std::iota(order.begin(), order.end(), std::size_t{0});
  std::stable_sort(order.begin(), order.end(), [&](std::size_t a, std::size_t b) {
    return scalar_as_f64(state.values[a], "nelder_mead_minimizer") <
           scalar_as_f64(state.values[b], "nelder_mead_minimizer");
  });

  const auto old_vertices = state.vertices;
  const auto old_values = state.values;
  std::vector<Series> sorted_vertices;
  sorted_vertices.reserve(order.size());
  std::vector<Series> sorted_values;
  sorted_values.reserve(order.size());

  for (const auto i : order) {
    sorted_vertices.push_back(old_vertices[i]);
    sorted_values.push_back(old_values[i]);
  }

  state.vertices = std::move(sorted_vertices);
  state.values = std::move(sorted_values);
  return state;
}

Series evaluate(
    const std::function<Series(const Series&)>& objective,
    const Series& flat_x,
    const std::vector<std::int64_t>& original_shape) {
  const Series x = flat_x.reshape(std::span<const std::int64_t>(original_shape.data(), original_shape.size()));
  const Series value = objective(x);
  if (!value.defined() || value.numel() != 1) {
    ::pluss::throw_error(
        "subsp::math::optimization::nelder_mead_minimizer: objective must return a scalar Series");
  }
  if (value.dtype().is_complex()) {
    ::pluss::throw_error(
        "subsp::math::optimization::nelder_mead_minimizer: objective must return a real scalar Series");
  }

  const double host_value = scalar_as_f64(value, "nelder_mead_minimizer");
  // +Inf is a legitimate value ("this point is not allowed"); the simplex simply moves away from it. NaN and -Inf are not.
  if (std::isnan(host_value) || host_value == -std::numeric_limits<double>::infinity()) {
    ::pluss::throw_error(
        "subsp::math::optimization::nelder_mead_minimizer: objective returned NaN or -Inf");
  }

  return value.to(flat_x.platform(), flat_x.scalar_type());
}

SimplexState initialize_state(
    const std::function<Series(const Series&)>& objective,
    const Series& flat_initial,
    const Config& cfg,
    const std::vector<std::int64_t>& original_shape,
    const NelderMeadConfig& config) {
  const std::int64_t n = flat_initial.numel();
  const Config idx_cfg = Config{}.with_dtype(ScalarType::Int64).with_platform(flat_initial.platform());

  SimplexState state;
  state.vertices.reserve(static_cast<std::size_t>(n + 1));
  state.values.reserve(static_cast<std::size_t>(n + 1));

  state.vertices.push_back(flat_initial);
  state.values.push_back(evaluate(objective, flat_initial, original_shape));
  state.evaluations = 1;

  for (std::int64_t j = 0; j < n; ++j) {
    Series candidate = flat_initial.clone();
    const Series idx = pluss::series::arange(j, j + 1, idx_cfg);
    const Series current = candidate.take(idx);

    Series stepped = current.mul(1.0 + config.simplex_delta);
    if (scalar_as_f64(current, "nelder_mead_minimizer") == 0.0) {
      stepped = pluss::series::full({1}, config.simplex_delta_zero, cfg);
    }

    candidate = candidate.put(idx, stepped, false);

    state.vertices.push_back(candidate);
    state.values.push_back(evaluate(objective, candidate, original_shape));
    ++state.evaluations;
  }

  state.iterations = 1;
  state.last_step = "seed";
  return sort_state(std::move(state));
}

bool has_converged(const SimplexState& state, const NelderMeadConfig& config) {
  const Series& best_x = state.vertices.front();
  const Series& best_value = state.values.front();

  // A simplex whose best vertex is still +Inf has found no allowed point yet; it cannot have converged.
  if (std::isinf(scalar_as_f64(best_value, "nelder_mead_minimizer"))) return false;

  double max_value_delta = 0.0;
  for (std::size_t i = 1; i < state.values.size(); ++i) {
    const Series delta = state.values[i].sub(best_value).abs();
    max_value_delta = std::max(max_value_delta, scalar_as_f64(delta, "nelder_mead_minimizer"));
  }

  double max_position_delta = 0.0;
  for (std::size_t i = 1; i < state.vertices.size(); ++i) {
    const Series delta = state.vertices[i].sub(best_x).abs().amax();
    max_position_delta = std::max(max_position_delta, scalar_as_f64(delta, "nelder_mead_minimizer"));
  }

  // Match MATLAB fminsearch's eps(max(v(:,1))) scaling in the position test.
  const double best_coord = scalar_as_f64(best_x.amax(), "nelder_mead_minimizer");
  const double best_value_scalar = scalar_as_f64(best_value, "nelder_mead_minimizer");
  const double position_limit = std::max(config.position_tolerance, 10.0 * machine_gap(best_coord));
  const double value_limit = std::max(config.value_tolerance, 10.0 * machine_gap(best_value_scalar));
  return max_position_delta <= position_limit && max_value_delta <= value_limit;
}

SimplexState advance_state(
    const std::function<Series(const Series&)>& objective,
    const SimplexState& state,
    const std::vector<std::int64_t>& original_shape,
    const NelderMeadConfig& config) {
  const std::int64_t n = static_cast<std::int64_t>(state.vertices.size()) - 1;
  SimplexState next = state;

  Series centroid = state.vertices.front();
  for (std::int64_t i = 1; i < n; ++i) {
    centroid = centroid.add(state.vertices[static_cast<std::size_t>(i)]);
  }
  centroid = centroid.div(static_cast<double>(n));

  const Series& worst = state.vertices.back();
  const Series reflected = centroid.mul(1.0 + config.reflection).sub(worst.mul(config.reflection));
  const Series reflected_value = evaluate(objective, reflected, original_shape);
  const double reflected_scalar = scalar_as_f64(reflected_value, "nelder_mead_minimizer");
  ++next.evaluations;

  const double best_scalar = scalar_as_f64(state.values.front(), "nelder_mead_minimizer");
  const double second_worst_scalar =
      scalar_as_f64(state.values[static_cast<std::size_t>(n - 1)], "nelder_mead_minimizer");
  const double worst_scalar = scalar_as_f64(state.values.back(), "nelder_mead_minimizer");

  if (reflected_scalar < best_scalar) {
    const Series expanded =
        centroid.mul(1.0 + config.reflection * config.expansion).sub(
            worst.mul(config.reflection * config.expansion));
    const Series expanded_value = evaluate(objective, expanded, original_shape);
    const double expanded_scalar = scalar_as_f64(expanded_value, "nelder_mead_minimizer");
    ++next.evaluations;

    if (expanded_scalar < reflected_scalar) {
      next.vertices[static_cast<std::size_t>(n)] = expanded;
      next.values[static_cast<std::size_t>(n)] = expanded_value;
      next.last_step = "expand";
    } else {
      next.vertices[static_cast<std::size_t>(n)] = reflected;
      next.values[static_cast<std::size_t>(n)] = reflected_value;
      next.last_step = "reflect";
    }
  } else if (reflected_scalar < second_worst_scalar) {
    next.vertices[static_cast<std::size_t>(n)] = reflected;
    next.values[static_cast<std::size_t>(n)] = reflected_value;
    next.last_step = "reflect";
  } else {
    bool apply_shrink = false;

    if (reflected_scalar < worst_scalar) {
      const Series outside =
          centroid.mul(1.0 + config.contraction * config.reflection).sub(
              worst.mul(config.contraction * config.reflection));
      const Series outside_value = evaluate(objective, outside, original_shape);
      const double outside_scalar = scalar_as_f64(outside_value, "nelder_mead_minimizer");
      ++next.evaluations;

      if (outside_scalar <= reflected_scalar) {
        next.vertices[static_cast<std::size_t>(n)] = outside;
        next.values[static_cast<std::size_t>(n)] = outside_value;
        next.last_step = "adjust_outer";
      } else {
        apply_shrink = true;
      }
    } else {
      const Series inside = centroid.mul(1.0 - config.contraction).add(worst.mul(config.contraction));
      const Series inside_value = evaluate(objective, inside, original_shape);
      const double inside_scalar = scalar_as_f64(inside_value, "nelder_mead_minimizer");
      ++next.evaluations;

      if (inside_scalar < worst_scalar) {
        next.vertices[static_cast<std::size_t>(n)] = inside;
        next.values[static_cast<std::size_t>(n)] = inside_value;
        next.last_step = "adjust_inner";
      } else {
        apply_shrink = true;
      }
    }

    if (apply_shrink) {
      const Series& best = state.vertices.front();
      for (std::int64_t j = 1; j <= n; ++j) {
        const Series shrunk =
            best.add(state.vertices[static_cast<std::size_t>(j)].sub(best).mul(config.shrink));
        next.vertices[static_cast<std::size_t>(j)] = shrunk;
        next.values[static_cast<std::size_t>(j)] = evaluate(objective, shrunk, original_shape);
        ++next.evaluations;
      }
      next.last_step = "recenter";
    }
  }

  next.iterations = state.iterations + 1;
  return sort_state(std::move(next));
}

} // namespace

NelderMeadOutcome nelder_mead_minimizer(
    const std::function<Series(const Series&)>& objective,
    const Series& initial,
    const NelderMeadConfig& config) {
  require_valid_initial(initial);

  const std::int64_t n = initial.numel();
  const std::int64_t max_iterations =
      (config.max_iterations > 0) ? config.max_iterations : (200 * n);
  const std::int64_t max_evaluations =
      (config.max_evaluations > 0) ? config.max_evaluations : (200 * n);

  ScalarType work_dtype = initial.scalar_type();
  if (config.promote_cpu_to_float64 && initial.platform() == pluss::pCPU &&
      work_dtype != ScalarType::Float64) {
    work_dtype = ScalarType::Float64;
  }
  const Config work_cfg = Config{}.with_dtype(work_dtype).with_platform(initial.platform());

  const std::vector<std::int64_t> original_shape(initial.sizes().begin(), initial.sizes().end());
  const Series flat_initial = initial.reshape({n}).to(initial.platform(), work_dtype).contiguous();

  SimplexState state = initialize_state(objective, flat_initial, work_cfg, original_shape, config);
  maybe_print_iteration(state, config);
  bool converged = false;

  while (state.iterations < max_iterations && state.evaluations < max_evaluations) {
    if (has_converged(state, config)) {
      converged = true;
      break;
    }
    state = advance_state(objective, state, original_shape, config);
    maybe_print_iteration(state, config);
  }

  NelderMeadOutcome out;
  out.minimizer = state.vertices.front()
                      .reshape(std::span<const std::int64_t>(original_shape.data(), original_shape.size()))
                      .to(initial.platform(), initial.scalar_type());
  out.minimum = scalar_as_f64(state.values.front(), "nelder_mead_minimizer");
  out.status = pluss::tils::suppose(converged)
                   .then([&](bool ok) {
                     if (ok) return SolverStatus::Converged;
                     return (state.evaluations >= max_evaluations)
                                ? SolverStatus::ReachedEvaluationBudget
                                : SolverStatus::ReachedIterationBudget;
                   })
                   .value();
  out.diagnostics.iterations = state.iterations;
  out.diagnostics.evaluations = state.evaluations;
  out.diagnostics.last_step = state.last_step;
  return out;
}

} // namespace pluss::subsp::math::optimization
