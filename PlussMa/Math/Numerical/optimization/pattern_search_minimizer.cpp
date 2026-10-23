#include "pattern_search_minimizer.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <deque>
#if __has_include(<fmt/format.h>)
#include <fmt/format.h>
#endif
#include <iostream>
#include <limits>
#include <numeric>
#include <random>
#include <span>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace pluss::subsp::math::optimization {
namespace {

struct CommonSetup {
  std::vector<std::int64_t> original_shape{};
  std::int64_t dimension{0};
  Config work_cfg{};
  Series flat_initial{pluss::series::undefined()};
};

struct BoundsPack {
  bool enabled{false};
  Series lower{pluss::series::undefined()};
  Series upper{pluss::series::undefined()};
  std::vector<double> lower_host{};
  std::vector<double> upper_host{};
};

struct LinearConstraints {
  bool has_ineq{false};
  bool has_eq{false};
  std::int64_t n_dim{0};
  std::int64_t n_ineq{0};
  std::int64_t n_eq{0};
  std::vector<double> a_ineq{};
  std::vector<double> b_ineq{};
  std::vector<double> a_eq{};
  std::vector<double> b_eq{};
};

struct CandidateEvaluation {
  double objective{std::numeric_limits<double>::infinity()};
  double merit{std::numeric_limits<double>::infinity()};
  double max_violation{std::numeric_limits<double>::infinity()};
  bool feasible{false};
};

struct CachedEvaluation {
  std::vector<double> x{};
  double objective{std::numeric_limits<double>::infinity()};
  double max_violation{std::numeric_limits<double>::infinity()};
  bool feasible{false};
};

struct EvaluationCache {
  std::deque<CachedEvaluation> entries{};
  std::int64_t max_entries{0};
  double tol{1e-12};
};

struct NupsScaleState {
  std::vector<double> axis_scale{};
};

struct NonlinearSnapshot {
  std::vector<double> ineq{};
  std::vector<double> eq{};
  std::vector<double> ineq_plus{};
  double max_violation{0.0};
};

double scalar_as_f64(const Series& x, const char* op) {
  if (!x.defined() || x.numel() != 1) {
    ::pluss::throw_error(std::string(op) + ": expected scalar Series");
  }
  return x.to(ScalarType::Float64).item<double>();
}

std::vector<double> flatten_real_series(
    const Series& x,
    const char* op,
    const char* name) {
  if (!x.defined() || x.numel() == 0) {
    return {};
  }
  if (x.dtype().is_complex()) {
    ::pluss::throw_error(std::string(op) + ": " + name + " must be real");
  }
  const Series flat = x.reshape({x.numel()}).to(pluss::pCPU, ScalarType::Float64);
  return flat.to_vec<double>();
}

NonlinearSnapshot evaluate_nonlinear_snapshot(
    const std::function<PSConstraintValues(const Series&)>& nonlinear_constraints,
    const Series& shaped_x,
    const char* op) {
  NonlinearSnapshot out;
  if (!nonlinear_constraints) {
    return out;
  }

  const PSConstraintValues c = nonlinear_constraints(shaped_x);
  out.ineq = flatten_real_series(c.inequality, op, "nonlinear inequality");
  out.eq = flatten_real_series(c.equality, op, "nonlinear equality");
  out.ineq_plus.resize(out.ineq.size(), 0.0);

  double max_v = 0.0;
  for (std::size_t i = 0; i < out.ineq.size(); ++i) {
    const double v = out.ineq[i];
    if (!std::isfinite(v)) {
      out.max_violation = std::numeric_limits<double>::infinity();
      return out;
    }
    const double vp = std::max(0.0, v);
    out.ineq_plus[i] = vp;
    max_v = std::max(max_v, vp);
  }
  for (const double v : out.eq) {
    if (!std::isfinite(v)) {
      out.max_violation = std::numeric_limits<double>::infinity();
      return out;
    }
    max_v = std::max(max_v, std::abs(v));
  }

  out.max_violation = max_v;
  return out;
}

double augmented_lagrangian_term(
    const NonlinearSnapshot& c,
    const std::vector<double>& lambda_ineq,
    const std::vector<double>& lambda_eq,
    double rho,
    const char* op) {
  if (c.ineq.size() != lambda_ineq.size() || c.eq.size() != lambda_eq.size()) {
    ::pluss::throw_error(std::string(op) + ": nonlinear constraint dimensionality changed across evaluations");
  }

  double term = 0.0;
  for (std::size_t i = 0; i < c.ineq_plus.size(); ++i) {
    const double g = c.ineq_plus[i];
    term += lambda_ineq[i] * g + 0.5 * rho * g * g;
  }
  for (std::size_t i = 0; i < c.eq.size(); ++i) {
    const double h = c.eq[i];
    term += lambda_eq[i] * h + 0.5 * rho * h * h;
  }
  return term;
}

void require_valid_initial(const Series& initial, const char* op) {
  if (!initial.defined()) {
    ::pluss::throw_error(std::string(op) + ": initial point is undefined");
  }
  if (initial.numel() < 1) {
    ::pluss::throw_error(std::string(op) + ": initial point must be non-empty");
  }
  if (!initial.dtype().is_floating() || initial.dtype().is_complex()) {
    ::pluss::throw_error(std::string(op) + ": initial point must be real floating-point");
  }
}

void require_valid_config(const PSConfig& cfg, const char* op) {
  if (cfg.mesh_tolerance <= 0.0 || cfg.step_tolerance <= 0.0 ||
      cfg.value_tolerance <= 0.0 || cfg.constraint_tolerance <= 0.0 ||
      cfg.active_constraint_tolerance <= 0.0) {
    ::pluss::throw_error(std::string(op) + ": tolerances must be > 0");
  }
  if (cfg.initial_mesh_size <= 0.0 || cfg.max_mesh_size <= 0.0) {
    ::pluss::throw_error(std::string(op) + ": mesh sizes must be > 0");
  }
  if (cfg.mesh_expansion < 1.0 || cfg.mesh_contraction <= 0.0 ||
      cfg.mesh_contraction >= 1.0) {
    ::pluss::throw_error(std::string(op) + ": invalid mesh expansion/contraction factors");
  }
  if (cfg.search_expansion < 1.0) {
    ::pluss::throw_error(std::string(op) + ": search_expansion must be >= 1");
  }
  if (cfg.search_trials < 0) {
    ::pluss::throw_error(std::string(op) + ": search_trials must be >= 0");
  }
  if ((cfg.search_strategy == PSSearchStrategy::RandomDirections ||
       cfg.search_strategy == PSSearchStrategy::ExpandAndRandomDirections) &&
      cfg.search_trials < 1) {
    ::pluss::throw_error(std::string(op) + ": random-direction search requires search_trials >= 1");
  }
  if (cfg.initial_penalty <= 0.0 || cfg.penalty_growth < 1.0) {
    ::pluss::throw_error(std::string(op) + ": invalid penalty parameters");
  }
  if (cfg.enable_cache && cfg.cache_max_entries < 1) {
    ::pluss::throw_error(std::string(op) + ": cache_max_entries must be >= 1 when cache is enabled");
  }
  if (cfg.enable_cache && cfg.cache_tolerance <= 0.0) {
    ::pluss::throw_error(std::string(op) + ": cache_tolerance must be > 0 when cache is enabled");
  }
  if (cfg.basis_refresh_interval < 1) {
    ::pluss::throw_error(std::string(op) + ": basis_refresh_interval must be >= 1");
  }
  if (cfg.al_max_outer_iterations < 1) {
    ::pluss::throw_error(std::string(op) + ": al_max_outer_iterations must be >= 1");
  }
  if (cfg.al_penalty_growth < 1.0) {
    ::pluss::throw_error(std::string(op) + ": al_penalty_growth must be >= 1");
  }
  if (cfg.al_violation_reduction_ratio <= 0.0 || cfg.al_violation_reduction_ratio >= 1.0) {
    ::pluss::throw_error(std::string(op) + ": al_violation_reduction_ratio must be in (0,1)");
  }
  if (cfg.al_multiplier_clip <= 0.0) {
    ::pluss::throw_error(std::string(op) + ": al_multiplier_clip must be > 0");
  }
  if (cfg.nups_expansion < 1.0) {
    ::pluss::throw_error(std::string(op) + ": nups_expansion must be >= 1");
  }
  if (cfg.nups_contraction <= 0.0 || cfg.nups_contraction >= 1.0) {
    ::pluss::throw_error(std::string(op) + ": nups_contraction must be in (0,1)");
  }
  if (cfg.nups_decay <= 0.0 || cfg.nups_decay > 1.0) {
    ::pluss::throw_error(std::string(op) + ": nups_decay must be in (0,1]");
  }
  if (cfg.nups_min_scale <= 0.0 || cfg.nups_max_scale < cfg.nups_min_scale) {
    ::pluss::throw_error(std::string(op) + ": invalid nups_min_scale/nups_max_scale");
  }
  if (!(cfg.time_limit_seconds > 0.0 || std::isinf(cfg.time_limit_seconds))) {
    ::pluss::throw_error(std::string(op) + ": time_limit_seconds must be positive or +inf");
  }
}

std::int64_t resolved_max_iterations(
    const PSConfig& cfg,
    std::int64_t dimension) {
  return (cfg.max_iterations > 0) ? cfg.max_iterations : (100 * dimension);
}

std::int64_t resolved_max_evaluations(
    const PSConfig& cfg,
    std::int64_t dimension) {
  return (cfg.max_evaluations > 0) ? cfg.max_evaluations : (2000 * dimension);
}

CommonSetup make_common_setup(const Series& initial, bool promote_cpu_to_float64) {
  CommonSetup out;
  out.original_shape.assign(initial.sizes().begin(), initial.sizes().end());
  out.dimension = initial.numel();

  ScalarType work_dtype = initial.scalar_type();
  if (promote_cpu_to_float64 && initial.platform() == pluss::pCPU &&
      work_dtype != ScalarType::Float64) {
    work_dtype = ScalarType::Float64;
  }

  out.work_cfg = Config{}.with_dtype(work_dtype).with_platform(initial.platform());
  out.flat_initial = initial.reshape({out.dimension})
                         .to(out.work_cfg.platform, out.work_cfg.dtype)
                         .contiguous();
  return out;
}

BoundsPack prepare_bounds(
    const Series& lower,
    const Series& upper,
    const CommonSetup& setup,
    const char* op) {
  BoundsPack bounds;
  const bool lower_defined = lower.defined();
  const bool upper_defined = upper.defined();

  if (!lower_defined && !upper_defined) {
    return bounds;
  }
  if (!lower_defined || !upper_defined) {
    ::pluss::throw_error(std::string(op) + ": both lower_bounds and upper_bounds must be defined");
  }

  const Series flat_lower =
      lower.reshape({setup.dimension}).to(setup.work_cfg.platform, setup.work_cfg.dtype).contiguous();
  const Series flat_upper =
      upper.reshape({setup.dimension}).to(setup.work_cfg.platform, setup.work_cfg.dtype).contiguous();

  if (flat_lower.numel() != setup.dimension || flat_upper.numel() != setup.dimension) {
    ::pluss::throw_error(std::string(op) + ": bounds must match initial point size");
  }
  if (!flat_lower.le(flat_upper).all().item<bool>()) {
    ::pluss::throw_error(std::string(op) + ": expected lower_bounds <= upper_bounds elementwise");
  }

  bounds.enabled = true;
  bounds.lower = flat_lower;
  bounds.upper = flat_upper;
  bounds.lower_host = flat_lower.to(ScalarType::Float64).to_vec<double>();
  bounds.upper_host = flat_upper.to(ScalarType::Float64).to_vec<double>();
  return bounds;
}

std::pair<std::int64_t, std::int64_t> require_matrix_shape(
    const Series& matrix,
    const char* name,
    const char* op) {
  if (!matrix.defined()) {
    ::pluss::throw_error(std::string(op) + ": undefined matrix for " + name);
  }
  if (matrix.sizes().size() != 2) {
    ::pluss::throw_error(std::string(op) + ": " + name + " must be rank-2");
  }
  const auto& sizes = matrix.sizes();
  return {sizes[0], sizes[1]};
}

LinearConstraints prepare_linear_constraints(
    const PSConfig& cfg,
    const CommonSetup& setup,
    const char* op) {
  LinearConstraints out;
  out.n_dim = setup.dimension;

  const bool has_ineq_matrix = cfg.linear_ineq_matrix.defined();
  const bool has_ineq_rhs = cfg.linear_ineq_rhs.defined();
  if (has_ineq_matrix != has_ineq_rhs) {
    ::pluss::throw_error(std::string(op) + ": linear_ineq_matrix and linear_ineq_rhs must be provided together");
  }
  if (has_ineq_matrix) {
    const auto [m, n] = require_matrix_shape(cfg.linear_ineq_matrix, "linear_ineq_matrix", op);
    if (n != setup.dimension) {
      ::pluss::throw_error(std::string(op) + ": linear_ineq_matrix column count must match initial size");
    }
    const Series rhs = cfg.linear_ineq_rhs.reshape({m}).to(ScalarType::Float64);
    if (rhs.numel() != m) {
      ::pluss::throw_error(std::string(op) + ": linear_ineq_rhs length mismatch");
    }
    out.has_ineq = m > 0;
    out.n_ineq = m;
    out.a_ineq = cfg.linear_ineq_matrix.reshape({m * n}).to(ScalarType::Float64).to_vec<double>();
    out.b_ineq = rhs.to_vec<double>();
  }

  const bool has_eq_matrix = cfg.linear_eq_matrix.defined();
  const bool has_eq_rhs = cfg.linear_eq_rhs.defined();
  if (has_eq_matrix != has_eq_rhs) {
    ::pluss::throw_error(std::string(op) + ": linear_eq_matrix and linear_eq_rhs must be provided together");
  }
  if (has_eq_matrix) {
    const auto [m, n] = require_matrix_shape(cfg.linear_eq_matrix, "linear_eq_matrix", op);
    if (n != setup.dimension) {
      ::pluss::throw_error(std::string(op) + ": linear_eq_matrix column count must match initial size");
    }
    const Series rhs = cfg.linear_eq_rhs.reshape({m}).to(ScalarType::Float64);
    if (rhs.numel() != m) {
      ::pluss::throw_error(std::string(op) + ": linear_eq_rhs length mismatch");
    }
    out.has_eq = m > 0;
    out.n_eq = m;
    out.a_eq = cfg.linear_eq_matrix.reshape({m * n}).to(ScalarType::Float64).to_vec<double>();
    out.b_eq = rhs.to_vec<double>();
  }

  return out;
}

Series host_vector_to_flat_series(
    const std::vector<double>& x,
    const CommonSetup& setup) {
  const Config host_cfg = Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);
  return pluss::make_series(x, {setup.dimension}, host_cfg)
      .to(setup.work_cfg.platform, setup.work_cfg.dtype)
      .contiguous();
}

Series host_vector_to_shaped_series(
    const std::vector<double>& x,
    const CommonSetup& setup) {
  return host_vector_to_flat_series(x, setup)
      .reshape(std::span<const std::int64_t>(setup.original_shape.data(), setup.original_shape.size()));
}

void project_to_bounds(
    std::vector<double>* x,
    const BoundsPack& bounds) {
  if (!bounds.enabled) {
    return;
  }
  for (std::size_t i = 0; i < x->size(); ++i) {
    (*x)[i] = std::max(bounds.lower_host[i], std::min((*x)[i], bounds.upper_host[i]));
  }
}

double compute_linear_violation(
    const std::vector<double>& x,
    const LinearConstraints& linear) {
  double violation = 0.0;

  if (linear.has_ineq) {
    for (std::int64_t i = 0; i < linear.n_ineq; ++i) {
      double lhs = 0.0;
      const std::size_t row = static_cast<std::size_t>(i) * static_cast<std::size_t>(linear.n_dim);
      for (std::int64_t j = 0; j < linear.n_dim; ++j) {
        lhs += linear.a_ineq[row + static_cast<std::size_t>(j)] * x[static_cast<std::size_t>(j)];
      }
      violation = std::max(violation, lhs - linear.b_ineq[static_cast<std::size_t>(i)]);
    }
  }

  if (linear.has_eq) {
    for (std::int64_t i = 0; i < linear.n_eq; ++i) {
      double lhs = 0.0;
      const std::size_t row = static_cast<std::size_t>(i) * static_cast<std::size_t>(linear.n_dim);
      for (std::int64_t j = 0; j < linear.n_dim; ++j) {
        lhs += linear.a_eq[row + static_cast<std::size_t>(j)] * x[static_cast<std::size_t>(j)];
      }
      violation = std::max(violation, std::abs(lhs - linear.b_eq[static_cast<std::size_t>(i)]));
    }
  }

  return std::max(0.0, violation);
}

double compute_bound_violation(
    const std::vector<double>& x,
    const BoundsPack& bounds) {
  if (!bounds.enabled) {
    return 0.0;
  }

  double violation = 0.0;
  for (std::size_t i = 0; i < x.size(); ++i) {
    violation = std::max(violation, bounds.lower_host[i] - x[i]);
    violation = std::max(violation, x[i] - bounds.upper_host[i]);
  }
  return std::max(0.0, violation);
}

double max_inequality_violation(const Series& ineq) {
  if (!ineq.defined() || ineq.numel() == 0) {
    return 0.0;
  }
  if (ineq.dtype().is_complex()) {
    return std::numeric_limits<double>::infinity();
  }
  double max_v = 0.0;
  for (const double v : ineq.to(ScalarType::Float64).reshape({ineq.numel()}).to_vec<double>()) {
    if (!std::isfinite(v)) {
      return std::numeric_limits<double>::infinity();
    }
    max_v = std::max(max_v, v);
  }
  return std::max(0.0, max_v);
}

double max_equality_violation(const Series& eq) {
  if (!eq.defined() || eq.numel() == 0) {
    return 0.0;
  }
  if (eq.dtype().is_complex()) {
    return std::numeric_limits<double>::infinity();
  }
  double max_v = 0.0;
  for (const double v : eq.to(ScalarType::Float64).reshape({eq.numel()}).to_vec<double>()) {
    if (!std::isfinite(v)) {
      return std::numeric_limits<double>::infinity();
    }
    max_v = std::max(max_v, std::abs(v));
  }
  return max_v;
}

bool cache_lookup(
    const EvaluationCache& cache,
    const std::vector<double>& x,
    CachedEvaluation* out);

void cache_store(
    EvaluationCache* cache,
    CachedEvaluation entry);

CandidateEvaluation evaluate_candidate(
    const std::function<Series(const Series&)>& objective,
    const PSConfig& cfg,
    const CommonSetup& setup,
    const BoundsPack& bounds,
    const LinearConstraints& linear,
    const std::vector<double>& x,
    double penalty,
    EvaluationCache* cache,
    std::int64_t* evaluations,
    std::int64_t* cache_hits,
    const char* op) {
  CandidateEvaluation out;

  if (cache != nullptr) {
    CachedEvaluation cached;
    if (cache_lookup(*cache, x, &cached)) {
      ++(*cache_hits);
      out.objective = cached.objective;
      out.max_violation = cached.max_violation;
      out.feasible = cached.feasible;
      if (cfg.constraint_policy == PSConstraintPolicy::ExtremeBarrier) {
        out.merit = cached.feasible ? cached.objective : std::numeric_limits<double>::infinity();
      } else {
        out.merit = std::isfinite(cached.objective)
                        ? cached.objective + penalty * cached.max_violation
                        : std::numeric_limits<double>::infinity();
      }
      return out;
    }
  }

  const double bound_v = compute_bound_violation(x, bounds);
  const double linear_v = compute_linear_violation(x, linear);
  double nonlin_v = 0.0;

  if (cfg.nonlinear_constraints) {
    const Series shaped_x = host_vector_to_shaped_series(x, setup);
    const PSConstraintValues c = cfg.nonlinear_constraints(shaped_x);
    nonlin_v = std::max(max_inequality_violation(c.inequality), max_equality_violation(c.equality));
  }

  out.max_violation = std::max({bound_v, linear_v, nonlin_v, 0.0});
  const bool feasible_wrt_constraints = out.max_violation <= cfg.constraint_tolerance;

  if (cfg.constraint_policy == PSConstraintPolicy::ExtremeBarrier &&
      !feasible_wrt_constraints) {
    out.objective = std::numeric_limits<double>::infinity();
    out.merit = std::numeric_limits<double>::infinity();
    out.feasible = false;
    if (cache != nullptr) {
      cache_store(cache, CachedEvaluation{x, out.objective, out.max_violation, out.feasible});
    }
    return out;
  }

  const Series shaped_x = host_vector_to_shaped_series(x, setup);
  const Series y = objective(shaped_x);
  if (!y.defined() || y.numel() != 1 || y.dtype().is_complex()) {
    ::pluss::throw_error(std::string(op) + ": objective must return real scalar Series");
  }
  ++(*evaluations);

  out.objective = scalar_as_f64(y, op);
  if (!std::isfinite(out.objective)) {
    out.objective = std::numeric_limits<double>::infinity();
  }

  if (cfg.constraint_policy == PSConstraintPolicy::ExtremeBarrier) {
    out.merit = feasible_wrt_constraints ? out.objective : std::numeric_limits<double>::infinity();
  } else {
    out.merit = std::isfinite(out.objective)
                    ? out.objective + penalty * out.max_violation
                    : std::numeric_limits<double>::infinity();
  }
  out.feasible = feasible_wrt_constraints && std::isfinite(out.objective);
  if (cache != nullptr) {
    cache_store(cache, CachedEvaluation{x, out.objective, out.max_violation, out.feasible});
  }
  return out;
}

double l2_norm(const std::vector<double>& x) {
  double s = 0.0;
  for (const double v : x) {
    s += v * v;
  }
  return std::sqrt(s);
}

double linf_norm_diff(const std::vector<double>& a, const std::vector<double>& b) {
  double m = 0.0;
  for (std::size_t i = 0; i < a.size(); ++i) {
    m = std::max(m, std::abs(a[i] - b[i]));
  }
  return m;
}

bool cache_lookup(
    const EvaluationCache& cache,
    const std::vector<double>& x,
    CachedEvaluation* out) {
  for (const auto& e : cache.entries) {
    if (e.x.size() != x.size()) {
      continue;
    }
    if (linf_norm_diff(e.x, x) <= cache.tol) {
      *out = e;
      return true;
    }
  }
  return false;
}

void cache_store(
    EvaluationCache* cache,
    CachedEvaluation entry) {
  if (cache->max_entries <= 0) {
    return;
  }
  cache->entries.push_back(std::move(entry));
  while (static_cast<std::int64_t>(cache->entries.size()) > cache->max_entries) {
    cache->entries.pop_front();
  }
}

std::vector<double> make_range_log_scale(
    const std::vector<double>& x,
    const BoundsPack& bounds) {
  const std::size_t n = x.size();
  std::vector<double> scale(n, 1.0);
  const double eps = std::numeric_limits<double>::epsilon();
  for (std::size_t j = 0; j < n; ++j) {
    if (bounds.enabled) {
      const double range = bounds.upper_host[j] - bounds.lower_host[j];
      if (std::isfinite(range) && range > eps) {
        scale[j] = range;
        continue;
      }
    }
    const double ax = std::abs(x[j]);
    if (ax >= eps) {
      scale[j] = std::pow(2.0, std::round(std::log2(ax)));
    } else {
      scale[j] = 1.0;
    }
    if (!std::isfinite(scale[j]) || scale[j] <= 0.0) {
      scale[j] = 1.0;
    }
  }
  return scale;
}

double scale_factor_from_axis(const std::vector<double>& scale) {
  const double eps = std::numeric_limits<double>::epsilon();
  const double nrm = l2_norm(scale);
  return (nrm > eps ? nrm : 1.0);
}

std::pair<std::vector<double>, double> compute_mesh_scale(
    const std::vector<double>& x,
    const BoundsPack& bounds,
    const PSConfig& cfg,
    NupsScaleState* nups_state) {
  if (cfg.mesh_scaling == PSMeshScaling::Off) {
    return {std::vector<double>(x.size(), 1.0), 1.0};
  }

  if (cfg.mesh_scaling == PSMeshScaling::RangeLog) {
    std::vector<double> scale = make_range_log_scale(x, bounds);
    return {scale, scale_factor_from_axis(scale)};
  }

  if (nups_state->axis_scale.size() != x.size()) {
    nups_state->axis_scale = make_range_log_scale(x, bounds);
  }
  return {nups_state->axis_scale, scale_factor_from_axis(nups_state->axis_scale)};
}

bool update_nups_scale_state(
    const PSConfig& cfg,
    const std::vector<double>& used_direction,
    bool improved,
    NupsScaleState* nups_state) {
  if (cfg.mesh_scaling != PSMeshScaling::NupsDynamic) {
    return false;
  }
  if (nups_state->axis_scale.empty()) {
    return false;
  }

  bool changed = false;
  for (std::size_t j = 0; j < nups_state->axis_scale.size(); ++j) {
    const double old = nups_state->axis_scale[j];
    double next = old;
    const double activity =
        (j < used_direction.size()) ? std::abs(used_direction[j]) : 0.0;

    if (improved && activity > 1e-12) {
      const double gain = 1.0 + (cfg.nups_expansion - 1.0) * std::min(1.0, activity);
      next *= gain;
    } else {
      next *= improved ? cfg.nups_decay : cfg.nups_contraction;
    }

    if (!std::isfinite(next)) {
      next = old;
    }
    next = std::clamp(next, cfg.nups_min_scale, cfg.nups_max_scale);
    if (std::abs(next - old) > 1e-14) {
      changed = true;
    }
    nups_state->axis_scale[j] = next;
  }
  return changed;
}

bool is_coordinate_direction(const std::vector<double>& d) {
  int nnz = 0;
  for (const double v : d) {
    if (std::abs(v) > 1e-12) {
      ++nnz;
      if (nnz > 1) {
        return false;
      }
    }
  }
  return nnz == 1;
}

double row_dot(
    const std::vector<double>& row_major,
    std::int64_t row,
    std::int64_t n_dim,
    const std::vector<double>& x) {
  double y = 0.0;
  const std::size_t offset = static_cast<std::size_t>(row) * static_cast<std::size_t>(n_dim);
  for (std::int64_t j = 0; j < n_dim; ++j) {
    y += row_major[offset + static_cast<std::size_t>(j)] * x[static_cast<std::size_t>(j)];
  }
  return y;
}

std::vector<std::vector<double>> make_raw_poll_directions(
    const std::vector<std::vector<double>>& basis,
    PSPollPattern pattern) {
  const std::int64_t n_dim = static_cast<std::int64_t>(basis.size());
  std::vector<std::vector<double>> dirs;
  if (n_dim == 0) {
    return dirs;
  }
  if (pattern == PSPollPattern::PositiveBasisNp1) {
    dirs.reserve(static_cast<std::size_t>(n_dim + 1));
    for (std::int64_t j = 0; j < n_dim; ++j) {
      dirs.push_back(basis[static_cast<std::size_t>(j)]);
    }
    std::vector<double> d(static_cast<std::size_t>(n_dim), 0.0);
    for (std::int64_t j = 0; j < n_dim; ++j) {
      const auto& bj = basis[static_cast<std::size_t>(j)];
      for (std::int64_t k = 0; k < n_dim; ++k) {
        d[static_cast<std::size_t>(k)] -= bj[static_cast<std::size_t>(k)];
      }
    }
    dirs.push_back(std::move(d));
  } else {
    dirs.reserve(static_cast<std::size_t>(2 * n_dim));
    for (std::int64_t j = 0; j < n_dim; ++j) {
      std::vector<double> p = basis[static_cast<std::size_t>(j)];
      std::vector<double> m = p;
      for (double& v : m) {
        v = -v;
      }
      dirs.push_back(std::move(p));
      dirs.push_back(std::move(m));
    }
  }
  return dirs;
}

bool normalize_direction(std::vector<double>* d) {
  const double nrm = l2_norm(*d);
  if (nrm <= 1e-14) {
    return false;
  }
  for (double& v : *d) {
    v /= nrm;
  }
  return true;
}

std::vector<std::vector<double>> make_identity_basis(std::int64_t n_dim) {
  std::vector<std::vector<double>> basis;
  basis.reserve(static_cast<std::size_t>(n_dim));
  for (std::int64_t j = 0; j < n_dim; ++j) {
    std::vector<double> e(static_cast<std::size_t>(n_dim), 0.0);
    e[static_cast<std::size_t>(j)] = 1.0;
    basis.push_back(std::move(e));
  }
  return basis;
}

std::vector<std::vector<double>> make_random_orthonormal_basis(
    std::int64_t n_dim,
    std::mt19937_64* rng) {
  std::vector<std::vector<double>> basis;
  basis.reserve(static_cast<std::size_t>(n_dim));
  std::normal_distribution<double> normal(0.0, 1.0);

  for (std::int64_t j = 0; j < n_dim; ++j) {
    bool accepted = false;
    for (int attempt = 0; attempt < 6 && !accepted; ++attempt) {
      std::vector<double> v(static_cast<std::size_t>(n_dim), 0.0);
      for (double& vi : v) {
        vi = normal(*rng);
      }

      for (const auto& q : basis) {
        const double coeff = std::inner_product(v.begin(), v.end(), q.begin(), 0.0);
        for (std::size_t k = 0; k < v.size(); ++k) {
          v[k] -= coeff * q[k];
        }
      }

      if (normalize_direction(&v)) {
        basis.push_back(std::move(v));
        accepted = true;
      }
    }

    if (accepted) {
      continue;
    }

    std::vector<double> fallback(static_cast<std::size_t>(n_dim), 0.0);
    fallback[static_cast<std::size_t>(j)] = 1.0;
    for (const auto& q : basis) {
      const double coeff = std::inner_product(fallback.begin(), fallback.end(), q.begin(), 0.0);
      for (std::size_t k = 0; k < fallback.size(); ++k) {
        fallback[k] -= coeff * q[k];
      }
    }
    if (!normalize_direction(&fallback)) {
      return make_identity_basis(n_dim);
    }
    basis.push_back(std::move(fallback));
  }

  return basis;
}

void maybe_refresh_poll_basis(
    const PSConfig& cfg,
    std::int64_t n_dim,
    std::int64_t iter,
    std::mt19937_64* rng,
    std::vector<std::vector<double>>* basis,
    std::int64_t* last_refresh_iter) {
  if (cfg.basis_family == PSBasisFamily::Canonical) {
    if (static_cast<std::int64_t>(basis->size()) != n_dim) {
      *basis = make_identity_basis(n_dim);
    }
    return;
  }

  const bool refresh_needed =
      (*last_refresh_iter < 0) ||
      ((iter - *last_refresh_iter) >= cfg.basis_refresh_interval) ||
      (static_cast<std::int64_t>(basis->size()) != n_dim);
  if (refresh_needed) {
    *basis = make_random_orthonormal_basis(n_dim, rng);
    *last_refresh_iter = iter;
  }
}

bool solve_linear_system(
    std::vector<double>* a,
    std::vector<double>* b,
    std::int64_t n) {
  for (std::int64_t k = 0; k < n; ++k) {
    std::int64_t pivot = k;
    double pivot_abs = std::abs((*a)[static_cast<std::size_t>(k) * static_cast<std::size_t>(n) +
                                 static_cast<std::size_t>(k)]);
    for (std::int64_t i = k + 1; i < n; ++i) {
      const double v = std::abs((*a)[static_cast<std::size_t>(i) * static_cast<std::size_t>(n) +
                                     static_cast<std::size_t>(k)]);
      if (v > pivot_abs) {
        pivot = i;
        pivot_abs = v;
      }
    }
    if (pivot_abs <= 1e-14) {
      return false;
    }
    if (pivot != k) {
      for (std::int64_t j = k; j < n; ++j) {
        std::swap((*a)[static_cast<std::size_t>(k) * static_cast<std::size_t>(n) +
                       static_cast<std::size_t>(j)],
                  (*a)[static_cast<std::size_t>(pivot) * static_cast<std::size_t>(n) +
                       static_cast<std::size_t>(j)]);
      }
      std::swap((*b)[static_cast<std::size_t>(k)], (*b)[static_cast<std::size_t>(pivot)]);
    }

    const double diag = (*a)[static_cast<std::size_t>(k) * static_cast<std::size_t>(n) +
                             static_cast<std::size_t>(k)];
    for (std::int64_t i = k + 1; i < n; ++i) {
      const double f = (*a)[static_cast<std::size_t>(i) * static_cast<std::size_t>(n) +
                            static_cast<std::size_t>(k)] /
                       diag;
      (*a)[static_cast<std::size_t>(i) * static_cast<std::size_t>(n) +
           static_cast<std::size_t>(k)] = 0.0;
      for (std::int64_t j = k + 1; j < n; ++j) {
        (*a)[static_cast<std::size_t>(i) * static_cast<std::size_t>(n) +
             static_cast<std::size_t>(j)] -=
            f * (*a)[static_cast<std::size_t>(k) * static_cast<std::size_t>(n) +
                     static_cast<std::size_t>(j)];
      }
      (*b)[static_cast<std::size_t>(i)] -= f * (*b)[static_cast<std::size_t>(k)];
    }
  }

  for (std::int64_t i = n - 1; i >= 0; --i) {
    double s = (*b)[static_cast<std::size_t>(i)];
    for (std::int64_t j = i + 1; j < n; ++j) {
      s -= (*a)[static_cast<std::size_t>(i) * static_cast<std::size_t>(n) +
                static_cast<std::size_t>(j)] *
           (*b)[static_cast<std::size_t>(j)];
    }
    const double diag = (*a)[static_cast<std::size_t>(i) * static_cast<std::size_t>(n) +
                             static_cast<std::size_t>(i)];
    if (std::abs(diag) <= 1e-14) {
      return false;
    }
    (*b)[static_cast<std::size_t>(i)] = s / diag;
  }
  return true;
}

std::vector<std::vector<double>> active_constraint_normals(
    const std::vector<double>& x,
    const LinearConstraints& linear,
    const BoundsPack& bounds,
    double active_tol) {
  std::vector<std::vector<double>> normals;
  normals.reserve(static_cast<std::size_t>(linear.n_eq + linear.n_ineq + linear.n_dim));

  if (linear.has_eq) {
    for (std::int64_t i = 0; i < linear.n_eq; ++i) {
      std::vector<double> row(static_cast<std::size_t>(linear.n_dim), 0.0);
      const std::size_t off = static_cast<std::size_t>(i) * static_cast<std::size_t>(linear.n_dim);
      for (std::int64_t j = 0; j < linear.n_dim; ++j) {
        row[static_cast<std::size_t>(j)] = linear.a_eq[off + static_cast<std::size_t>(j)];
      }
      normals.push_back(std::move(row));
    }
  }

  if (linear.has_ineq) {
    for (std::int64_t i = 0; i < linear.n_ineq; ++i) {
      const double lhs = row_dot(linear.a_ineq, i, linear.n_dim, x);
      const double slack = linear.b_ineq[static_cast<std::size_t>(i)] - lhs;
      if (slack <= active_tol) {
        std::vector<double> row(static_cast<std::size_t>(linear.n_dim), 0.0);
        const std::size_t off = static_cast<std::size_t>(i) * static_cast<std::size_t>(linear.n_dim);
        for (std::int64_t j = 0; j < linear.n_dim; ++j) {
          row[static_cast<std::size_t>(j)] = linear.a_ineq[off + static_cast<std::size_t>(j)];
        }
        normals.push_back(std::move(row));
      }
    }
  }

  if (bounds.enabled) {
    for (std::int64_t j = 0; j < linear.n_dim; ++j) {
      const std::size_t idx = static_cast<std::size_t>(j);
      if (std::abs(x[idx] - bounds.lower_host[idx]) <= active_tol ||
          std::abs(x[idx] - bounds.upper_host[idx]) <= active_tol) {
        std::vector<double> row(static_cast<std::size_t>(linear.n_dim), 0.0);
        row[idx] = 1.0;
        normals.push_back(std::move(row));
      }
    }
  }

  return normals;
}

std::vector<std::vector<double>> equality_constraint_normals(
    const LinearConstraints& linear) {
  std::vector<std::vector<double>> normals;
  if (!linear.has_eq) {
    return normals;
  }

  normals.reserve(static_cast<std::size_t>(linear.n_eq));
  for (std::int64_t i = 0; i < linear.n_eq; ++i) {
    std::vector<double> row(static_cast<std::size_t>(linear.n_dim), 0.0);
    const std::size_t off = static_cast<std::size_t>(i) * static_cast<std::size_t>(linear.n_dim);
    for (std::int64_t j = 0; j < linear.n_dim; ++j) {
      row[static_cast<std::size_t>(j)] = linear.a_eq[off + static_cast<std::size_t>(j)];
    }
    normals.push_back(std::move(row));
  }
  return normals;
}

std::vector<std::vector<double>> active_inward_directions(
    const std::vector<double>& x,
    const LinearConstraints& linear,
    const BoundsPack& bounds,
    double active_tol) {
  std::vector<std::vector<double>> inward;
  inward.reserve(static_cast<std::size_t>(linear.n_ineq + 2 * linear.n_dim));

  if (linear.has_ineq) {
    for (std::int64_t i = 0; i < linear.n_ineq; ++i) {
      const double lhs = row_dot(linear.a_ineq, i, linear.n_dim, x);
      const double slack = linear.b_ineq[static_cast<std::size_t>(i)] - lhs;
      if (slack > active_tol) {
        continue;
      }
      std::vector<double> d(static_cast<std::size_t>(linear.n_dim), 0.0);
      const std::size_t off = static_cast<std::size_t>(i) * static_cast<std::size_t>(linear.n_dim);
      for (std::int64_t j = 0; j < linear.n_dim; ++j) {
        d[static_cast<std::size_t>(j)] = -linear.a_ineq[off + static_cast<std::size_t>(j)];
      }
      inward.push_back(std::move(d));
    }
  }

  if (bounds.enabled) {
    for (std::int64_t j = 0; j < linear.n_dim; ++j) {
      const std::size_t idx = static_cast<std::size_t>(j);
      if (std::abs(x[idx] - bounds.lower_host[idx]) <= active_tol) {
        std::vector<double> d(static_cast<std::size_t>(linear.n_dim), 0.0);
        d[idx] = 1.0;
        inward.push_back(std::move(d));
      }
      if (std::abs(x[idx] - bounds.upper_host[idx]) <= active_tol) {
        std::vector<double> d(static_cast<std::size_t>(linear.n_dim), 0.0);
        d[idx] = -1.0;
        inward.push_back(std::move(d));
      }
    }
  }

  return inward;
}

// Generators of the cone of feasible directions at x. With the outward normals n_i of the active constraints (the rows of
// A x <= b, the bound rows -e_j / +e_j, and the equality rows, which are always active), the direction d_i that moves
// strictly into the i-th active inequality (n_i . d_i = -1) while staying on every other active constraint
// (n_j . d_i = 0) is d_i = -N^T (N N^T)^{-1} e_i. Together with a basis of the tangent space of the active constraints,
// these d_i positively span the feasible cone, so polling along them cannot miss a feasible descent direction. Returns
// nothing when the active normals are linearly dependent (the system is singular).
std::vector<std::vector<double>> feasible_cone_generators(
    const std::vector<double>& x,
    const LinearConstraints& linear,
    const BoundsPack& bounds,
    double active_tol) {
  std::vector<std::vector<double>> rows;
  std::vector<std::size_t> inequality_rows;
  const std::size_t n = static_cast<std::size_t>(linear.n_dim);

  if (linear.has_eq) {
    for (std::int64_t i = 0; i < linear.n_eq; ++i) {
      std::vector<double> r(n, 0.0);
      for (std::size_t j = 0; j < n; ++j) r[j] = linear.a_eq[static_cast<std::size_t>(i) * n + j];
      rows.push_back(std::move(r));
    }
  }
  if (linear.has_ineq) {
    for (std::int64_t i = 0; i < linear.n_ineq; ++i) {
      const double slack = linear.b_ineq[static_cast<std::size_t>(i)] - row_dot(linear.a_ineq, i, linear.n_dim, x);
      if (slack > active_tol) continue;
      std::vector<double> r(n, 0.0);
      for (std::size_t j = 0; j < n; ++j) r[j] = linear.a_ineq[static_cast<std::size_t>(i) * n + j];
      inequality_rows.push_back(rows.size());
      rows.push_back(std::move(r));
    }
  }
  if (bounds.enabled) {
    for (std::size_t j = 0; j < n; ++j) {
      if (std::abs(x[j] - bounds.lower_host[j]) <= active_tol) {
        std::vector<double> r(n, 0.0);
        r[j] = -1.0;
        inequality_rows.push_back(rows.size());
        rows.push_back(std::move(r));
      }
      if (std::abs(x[j] - bounds.upper_host[j]) <= active_tol) {
        std::vector<double> r(n, 0.0);
        r[j] = 1.0;
        inequality_rows.push_back(rows.size());
        rows.push_back(std::move(r));
      }
    }
  }

  std::vector<std::vector<double>> generators;
  const std::int64_t k = static_cast<std::int64_t>(rows.size());
  if (inequality_rows.empty() || k > static_cast<std::int64_t>(n)) {
    return generators;
  }
  for (const std::size_t target : inequality_rows) {
    std::vector<double> gram(static_cast<std::size_t>(k * k), 0.0);
    std::vector<double> y(static_cast<std::size_t>(k), 0.0);
    for (std::int64_t a = 0; a < k; ++a) {
      for (std::int64_t b = 0; b < k; ++b) {
        gram[static_cast<std::size_t>(a * k + b)] =
            std::inner_product(rows[static_cast<std::size_t>(a)].begin(), rows[static_cast<std::size_t>(a)].end(),
                               rows[static_cast<std::size_t>(b)].begin(), 0.0);
      }
    }
    y[target] = 1.0;
    if (!solve_linear_system(&gram, &y, k)) continue;
    std::vector<double> d(n, 0.0);
    for (std::int64_t a = 0; a < k; ++a) {
      for (std::size_t j = 0; j < n; ++j) d[j] -= y[static_cast<std::size_t>(a)] * rows[static_cast<std::size_t>(a)][j];
    }
    generators.push_back(std::move(d));
  }
  return generators;
}

bool same_direction(
    const std::vector<double>& lhs,
    const std::vector<double>& rhs,
    double cosine_tol) {
  const double dot = std::inner_product(lhs.begin(), lhs.end(), rhs.begin(), 0.0);
  return dot >= cosine_tol;
}

void append_unique_direction(
    std::vector<std::vector<double>>* dirs,
    std::vector<double> d,
    double cosine_tol) {
  if (!normalize_direction(&d)) {
    return;
  }
  for (const auto& existing : *dirs) {
    if (same_direction(existing, d, cosine_tol)) {
      return;
    }
  }
  dirs->push_back(std::move(d));
}

std::vector<double> project_to_active_tangent(
    const std::vector<double>& direction,
    const std::vector<std::vector<double>>& normals) {
  if (normals.empty()) {
    return direction;
  }

  const std::int64_t k = static_cast<std::int64_t>(normals.size());
  const std::int64_t n = static_cast<std::int64_t>(direction.size());
  std::vector<double> ad(static_cast<std::size_t>(k), 0.0);
  std::vector<double> gram(static_cast<std::size_t>(k * k), 0.0);

  for (std::int64_t i = 0; i < k; ++i) {
    ad[static_cast<std::size_t>(i)] = std::inner_product(
        normals[static_cast<std::size_t>(i)].begin(),
        normals[static_cast<std::size_t>(i)].end(),
        direction.begin(),
        0.0);
    for (std::int64_t j = 0; j < k; ++j) {
      gram[static_cast<std::size_t>(i) * static_cast<std::size_t>(k) + static_cast<std::size_t>(j)] =
          std::inner_product(
              normals[static_cast<std::size_t>(i)].begin(),
              normals[static_cast<std::size_t>(i)].end(),
              normals[static_cast<std::size_t>(j)].begin(),
              0.0);
    }
    gram[static_cast<std::size_t>(i) * static_cast<std::size_t>(k) + static_cast<std::size_t>(i)] += 1e-12;
  }

  if (!solve_linear_system(&gram, &ad, k)) {
    return direction;
  }

  std::vector<double> projected = direction;
  for (std::int64_t i = 0; i < k; ++i) {
    const double yi = ad[static_cast<std::size_t>(i)];
    for (std::int64_t j = 0; j < n; ++j) {
      projected[static_cast<std::size_t>(j)] -=
          yi * normals[static_cast<std::size_t>(i)][static_cast<std::size_t>(j)];
    }
  }
  return projected;
}

double ratio_test_alpha(
    const std::vector<double>& x,
    const std::vector<double>& d,
    const LinearConstraints& linear,
    const BoundsPack& bounds,
    double tol) {
  double alpha = 1.0;

  if (linear.has_ineq) {
    for (std::int64_t i = 0; i < linear.n_ineq; ++i) {
      const double denom = row_dot(linear.a_ineq, i, linear.n_dim, d);
      if (denom > tol) {
        const double num = linear.b_ineq[static_cast<std::size_t>(i)] -
                           row_dot(linear.a_ineq, i, linear.n_dim, x);
        alpha = std::min(alpha, num / denom);
      }
    }
  }

  if (bounds.enabled) {
    for (std::int64_t j = 0; j < linear.n_dim; ++j) {
      const std::size_t idx = static_cast<std::size_t>(j);
      if (d[idx] < -tol) {
        alpha = std::min(alpha, (bounds.lower_host[idx] - x[idx]) / d[idx]);
      } else if (d[idx] > tol) {
        alpha = std::min(alpha, (bounds.upper_host[idx] - x[idx]) / d[idx]);
      }
    }
  }

  if (!std::isfinite(alpha) || alpha <= tol) {
    return 0.0;
  }
  return std::min(1.0, alpha);
}

double l2_norm_diff(const std::vector<double>& a, const std::vector<double>& b);

bool propose_trial_from_direction(
    const std::vector<double>& x,
    const std::vector<double>& direction,
    const std::vector<double>& scale,
    double mesh_scale_factor,
    const LinearConstraints& linear,
    const BoundsPack& bounds,
    double step_tol,
    double active_tol,
    std::vector<double>* trial,
    std::vector<double>* used_direction) {
  const double nrm = l2_norm(direction);
  if (nrm <= 1e-14) {
    return false;
  }

  // With bounds the poll is made in scaled coordinates (x_j / range_j): every direction is stretched by the per-variable
  // range, so a basis that is orthonormal in scaled coordinates stays well conditioned when the variables live on very
  // different ranges.
  // Without bounds the scale is only a magnitude guess taken from the current point, which is too crude to stretch a
  // rotated basis with; there a non-coordinate direction keeps the single uniform factor.
  const bool per_axis = bounds.enabled || is_coordinate_direction(direction);
  std::vector<double> scaled(direction.size(), 0.0);
  for (std::size_t j = 0; j < direction.size(); ++j) {
    scaled[j] = direction[j] * (per_axis ? scale[j] : mesh_scale_factor);
  }

  const double alpha = ratio_test_alpha(x, scaled, linear, bounds, active_tol * 1e-3);
  if (alpha <= 0.0) {
    return false;
  }

  trial->assign(x.begin(), x.end());
  used_direction->assign(scaled.begin(), scaled.end());
  for (std::size_t j = 0; j < trial->size(); ++j) {
    (*trial)[j] += alpha * (*used_direction)[j];
  }
  project_to_bounds(trial, bounds);

  if (l2_norm_diff(*trial, x) <= step_tol * (1.0 + l2_norm(x))) {
    return false;
  }

  const double step_norm = l2_norm_diff(*trial, x);
  if (step_norm > 0.0) {
    for (std::size_t j = 0; j < used_direction->size(); ++j) {
      (*used_direction)[j] = ((*trial)[j] - x[j]) / step_norm;
    }
  }
  return true;
}

void restore_linear_feasibility(
    std::vector<double>* x,
    const LinearConstraints& linear,
    const BoundsPack& bounds,
    double tol) {
  if (!linear.has_eq && !linear.has_ineq) {
    project_to_bounds(x, bounds);
    return;
  }

  project_to_bounds(x, bounds);
  for (int pass = 0; pass < 12; ++pass) {
    bool changed = false;

    if (linear.has_eq) {
      for (std::int64_t i = 0; i < linear.n_eq; ++i) {
        const double r = row_dot(linear.a_eq, i, linear.n_dim, *x) -
                         linear.b_eq[static_cast<std::size_t>(i)];
        if (std::abs(r) > tol) {
          double denom = 0.0;
          const std::size_t off = static_cast<std::size_t>(i) * static_cast<std::size_t>(linear.n_dim);
          for (std::int64_t j = 0; j < linear.n_dim; ++j) {
            const double aij = linear.a_eq[off + static_cast<std::size_t>(j)];
            denom += aij * aij;
          }
          if (denom > 1e-14) {
            const double scale = r / denom;
            for (std::int64_t j = 0; j < linear.n_dim; ++j) {
              (*x)[static_cast<std::size_t>(j)] -=
                  scale * linear.a_eq[off + static_cast<std::size_t>(j)];
            }
            changed = true;
          }
        }
      }
    }

    if (linear.has_ineq) {
      for (std::int64_t i = 0; i < linear.n_ineq; ++i) {
        const double v = row_dot(linear.a_ineq, i, linear.n_dim, *x) -
                         linear.b_ineq[static_cast<std::size_t>(i)];
        if (v > tol) {
          double denom = 0.0;
          const std::size_t off = static_cast<std::size_t>(i) * static_cast<std::size_t>(linear.n_dim);
          for (std::int64_t j = 0; j < linear.n_dim; ++j) {
            const double aij = linear.a_ineq[off + static_cast<std::size_t>(j)];
            denom += aij * aij;
          }
          if (denom > 1e-14) {
            const double scale = v / denom;
            for (std::int64_t j = 0; j < linear.n_dim; ++j) {
              (*x)[static_cast<std::size_t>(j)] -=
                  scale * linear.a_ineq[off + static_cast<std::size_t>(j)];
            }
            changed = true;
          }
        }
      }
    }

    project_to_bounds(x, bounds);
    if (!changed) {
      break;
    }
  }
}

std::vector<std::vector<double>> make_poll_directions(
    const std::vector<double>& x,
    const std::vector<std::vector<double>>& raw_dirs,
    PSDirectionFamily family,
    const LinearConstraints& linear,
    const BoundsPack& bounds,
    double active_tol) {
  const auto normals = active_constraint_normals(x, linear, bounds, active_tol);
  std::vector<std::vector<double>> tangent_dirs;
  tangent_dirs.reserve(raw_dirs.size());

  if (normals.empty()) {
    for (const auto& d : raw_dirs) {
      append_unique_direction(&tangent_dirs, d, 1.0 - 1e-12);
    }
  } else {
    for (const auto& d : raw_dirs) {
      std::vector<double> p = project_to_active_tangent(d, normals);
      append_unique_direction(&tangent_dirs, std::move(p), 1.0 - 1e-12);
    }
  }

  if (tangent_dirs.empty()) {
    // The active constraints leave no tangent direction (a vertex of the feasible region). The polls must still keep
    // the equality constraints exactly satisfied, so project onto the equality tangent space only; the ratio test then
    // rejects the directions that point out through an active inequality or bound and keeps those that point in.
    const auto eq_only = equality_constraint_normals(linear);
    for (const auto& d : raw_dirs) {
      append_unique_direction(&tangent_dirs, project_to_active_tangent(d, eq_only), 1.0 - 1e-12);
    }
    if (tangent_dirs.empty()) {
      for (const auto& d : raw_dirs) {
        append_unique_direction(&tangent_dirs, d, 1.0 - 1e-12);
      }
    }
  }

  if (family == PSDirectionFamily::ProjectedTangent) {
    return tangent_dirs;
  }

  std::vector<std::vector<double>> augmented = tangent_dirs;
  const auto eq_normals = equality_constraint_normals(linear);
  const auto inward = active_inward_directions(x, linear, bounds, active_tol);
  std::vector<std::vector<double>> inward_projected;
  inward_projected.reserve(inward.size());

  for (const auto& inward_dir : inward) {
    std::vector<double> d = project_to_active_tangent(inward_dir, eq_normals);
    if (normalize_direction(&d)) {
      append_unique_direction(&augmented, d, 1.0 - 1e-8);
      inward_projected.push_back(std::move(d));
    }
  }

  if (!inward_projected.empty()) {
    const std::size_t blend_limit = std::min<std::size_t>(tangent_dirs.size(), 8);
    const std::size_t inward_limit = std::min<std::size_t>(inward_projected.size(), 3);
    for (std::size_t i = 0; i < blend_limit; ++i) {
      for (std::size_t j = 0; j < inward_limit; ++j) {
        std::vector<double> blended(tangent_dirs[i].size(), 0.0);
        for (std::size_t k = 0; k < blended.size(); ++k) {
          blended[k] = tangent_dirs[i][k] + 0.5 * inward_projected[j][k];
        }
        blended = project_to_active_tangent(blended, eq_normals);
        append_unique_direction(&augmented, std::move(blended), 1.0 - 1e-8);
      }
    }
  }

  for (auto& g : feasible_cone_generators(x, linear, bounds, active_tol)) {
    if (normalize_direction(&g)) {
      append_unique_direction(&augmented, std::move(g), 1.0 - 1e-8);
    }
  }

  return augmented;
}

double l2_norm_diff(const std::vector<double>& a, const std::vector<double>& b) {
  double s = 0.0;
  for (std::size_t i = 0; i < a.size(); ++i) {
    const double d = a[i] - b[i];
    s += d * d;
  }
  return std::sqrt(s);
}

std::vector<std::vector<double>> order_directions(
    std::vector<std::vector<double>> dirs,
    PSPollingOrder order,
    const std::vector<double>& success_direction,
    std::mt19937_64* rng) {
  if (order == PSPollingOrder::Random) {
    std::shuffle(dirs.begin(), dirs.end(), *rng);
    return dirs;
  }
  if (order != PSPollingOrder::SuccessFirst || success_direction.empty()) {
    return dirs;
  }

  const double n = l2_norm(success_direction);
  if (n <= std::numeric_limits<double>::epsilon()) {
    return dirs;
  }

  std::stable_sort(dirs.begin(), dirs.end(), [&](const auto& lhs, const auto& rhs) {
    double dot_l = 0.0;
    double dot_r = 0.0;
    for (std::size_t i = 0; i < lhs.size(); ++i) {
      dot_l += lhs[i] * success_direction[i];
      dot_r += rhs[i] * success_direction[i];
    }
    return dot_l > dot_r;
  });
  return dirs;
}

std::string iter_line(
    std::int64_t iter,
    std::int64_t evals,
    double f,
    double merit,
    double cv,
    double mesh,
    std::string_view phase) {
#if __has_include(<fmt/format.h>)
  return fmt::format(
      "iter={} evals={} f={:.6g} merit={:.6g} cv={:.3g} mesh={:.3g} step={}",
      iter,
      evals,
      f,
      merit,
      cv,
      mesh,
      phase);
#else
  std::ostringstream oss;
  oss << "iter=" << iter
      << " evals=" << evals
      << " f=" << f
      << " merit=" << merit
      << " cv=" << cv
      << " mesh=" << mesh
      << " step=" << phase;
  return oss.str();
#endif
}

void maybe_print_iteration(
    PSDisplay display,
    std::int64_t iter,
    std::int64_t evals,
    const CandidateEvaluation& eval,
    double mesh,
    std::string_view phase) {
  if (display != PSDisplay::Iter) {
    return;
  }
  std::cout << iter_line(iter, evals, eval.objective, eval.merit, eval.max_violation, mesh, phase) << "\n";
}

bool is_better(
    const CandidateEvaluation& a,
    const CandidateEvaluation& b,
    double min_improvement) {
  return a.merit + min_improvement < b.merit;
}

} // namespace

PSOutcome pattern_search_minimizer(
    const std::function<Series(const Series&)>& objective,
    const Series& initial,
    const PSConfig& config) {
  constexpr const char* op = "pattern_search_minimizer";
  require_valid_initial(initial, op);
  require_valid_config(config, op);

  if (config.constraint_policy == PSConstraintPolicy::AugmentedLagrangian &&
      !config.nonlinear_constraints) {
    PSConfig fallback = config;
    fallback.constraint_policy = PSConstraintPolicy::MeritPenalty;
    return pattern_search_minimizer(objective, initial, fallback);
  }

  if (config.constraint_policy == PSConstraintPolicy::AugmentedLagrangian &&
      static_cast<bool>(config.nonlinear_constraints)) {
    const std::int64_t dimension = initial.numel();
    std::int64_t remaining_iterations = resolved_max_iterations(config, dimension);
    std::int64_t remaining_evaluations = resolved_max_evaluations(config, dimension);
    const auto outer_t0 = std::chrono::steady_clock::now();

    Series outer_initial = initial;
    const NonlinearSnapshot init_snapshot =
        evaluate_nonlinear_snapshot(config.nonlinear_constraints, outer_initial, op);
    std::vector<double> lambda_ineq(init_snapshot.ineq.size(), 0.0);
    std::vector<double> lambda_eq(init_snapshot.eq.size(), 0.0);
    double rho = std::max(config.initial_penalty, 1e-8);
    double prev_outer_cv = std::numeric_limits<double>::infinity();
    double prev_outer_f = std::numeric_limits<double>::infinity();
    std::vector<double> prev_outer_x;

    bool have_best = false;
    bool converged = false;
    bool timed_out = false;
    bool exhausted_iterations = false;
    bool exhausted_evaluations = false;
    PSOutcome best_out;
    std::int64_t total_iterations = 0;
    std::int64_t total_evaluations = 0;
    std::int64_t total_cache_hits = 0;
    std::int64_t total_nups_updates = 0;
    std::int64_t total_search_trials = 0;
    std::int64_t total_search_successes = 0;
    std::int64_t outer_iterations = 0;

    for (std::int64_t outer = 0;
         outer < config.al_max_outer_iterations &&
         remaining_iterations > 0 &&
         remaining_evaluations > 0;
         ++outer) {
      if (std::isfinite(config.time_limit_seconds)) {
        const double elapsed = std::chrono::duration<double>(
            std::chrono::steady_clock::now() - outer_t0).count();
        if (elapsed >= config.time_limit_seconds) {
          timed_out = true;
          break;
        }
      }

      const std::vector<double> lambda_ineq_outer = lambda_ineq;
      const std::vector<double> lambda_eq_outer = lambda_eq;
      const double rho_outer = rho;

      PSConfig inner_cfg = config;
      inner_cfg.constraint_policy = PSConstraintPolicy::MeritPenalty;
      inner_cfg.nonlinear_constraints = {};
      // Each round gets a share of what is left, sized as if at most 12 rounds were planned, so that allowing more rounds
      // (a high al_max_outer_iterations) does not starve the early ones; any round past the twelfth may use all that remains.
      const std::int64_t outer_left =
          std::max<std::int64_t>(1, std::min<std::int64_t>(12, config.al_max_outer_iterations) - outer);
      inner_cfg.max_iterations = std::max<std::int64_t>(1, remaining_iterations / std::max<std::int64_t>(1, outer_left));
      inner_cfg.max_evaluations = std::max<std::int64_t>(1, remaining_evaluations / std::max<std::int64_t>(1, outer_left));
      if (std::isfinite(config.time_limit_seconds)) {
        const double elapsed = std::chrono::duration<double>(
            std::chrono::steady_clock::now() - outer_t0).count();
        const double remaining_time = config.time_limit_seconds - elapsed;
        if (remaining_time <= 0.0) {
          timed_out = true;
          break;
        }
        inner_cfg.time_limit_seconds = remaining_time;
      }
      if (outer > 0 && inner_cfg.display == PSDisplay::Iter) {
        inner_cfg.display = PSDisplay::None;
      }

      const auto augmented_objective =
          [&](const Series& x) -> Series {
        const double f = scalar_as_f64(objective(x), op);
        const NonlinearSnapshot c =
            evaluate_nonlinear_snapshot(config.nonlinear_constraints, x, op);
        const double aug = augmented_lagrangian_term(
            c,
            lambda_ineq_outer,
            lambda_eq_outer,
            rho_outer,
            op);
        return pluss::make_series(f + aug);
      };

      PSOutcome inner_out = pattern_search_minimizer(augmented_objective, outer_initial, inner_cfg);
      ++outer_iterations;
      total_iterations += inner_out.diagnostics.iterations;
      total_cache_hits += inner_out.diagnostics.cache_hits;
      total_nups_updates += inner_out.diagnostics.nups_updates;
      total_search_trials += inner_out.diagnostics.search_trials;
      total_search_successes += inner_out.diagnostics.search_successes;
      total_evaluations += inner_out.diagnostics.evaluations;
      remaining_iterations = std::max<std::int64_t>(
          0, remaining_iterations - inner_out.diagnostics.iterations);
      remaining_evaluations = std::max<std::int64_t>(
          0, remaining_evaluations - inner_out.diagnostics.evaluations);

      double f_true = inner_out.minimum;
      if (remaining_evaluations > 0) {
        f_true = scalar_as_f64(objective(inner_out.minimizer), op);
        --remaining_evaluations;
        ++total_evaluations;
      } else {
        exhausted_evaluations = true;
      }

      const NonlinearSnapshot c_final =
          evaluate_nonlinear_snapshot(config.nonlinear_constraints, inner_out.minimizer, op);
      if (c_final.ineq.size() != lambda_ineq.size() || c_final.eq.size() != lambda_eq.size()) {
        ::pluss::throw_error(std::string(op) + ": nonlinear constraint dimensionality changed across outer iterations");
      }

      const double cv = std::max(inner_out.diagnostics.max_constraint_violation, c_final.max_violation);
      const bool feasible = cv <= config.constraint_tolerance;

      if (!have_best) {
        have_best = true;
        best_out = inner_out;
        best_out.minimum = f_true;
        best_out.diagnostics.max_constraint_violation = cv;
      } else {
        const bool best_feasible = best_out.diagnostics.max_constraint_violation <= config.constraint_tolerance;
        const bool take_if_feasible_upgrade = feasible && !best_feasible;
        const bool take_if_better_feasible = feasible && best_feasible &&
            (f_true + config.value_tolerance < best_out.minimum);
        const bool take_if_less_infeasible = !feasible && !best_feasible &&
            (cv + config.constraint_tolerance < best_out.diagnostics.max_constraint_violation);
        const bool take_if_tied_cv = !feasible && !best_feasible &&
            (std::abs(cv - best_out.diagnostics.max_constraint_violation) <= config.constraint_tolerance) &&
            (f_true + config.value_tolerance < best_out.minimum);
        if (take_if_feasible_upgrade || take_if_better_feasible || take_if_less_infeasible || take_if_tied_cv) {
          best_out = inner_out;
          best_out.minimum = f_true;
          best_out.diagnostics.max_constraint_violation = cv;
        }
      }

      double max_dual_delta = 0.0;
      for (std::size_t i = 0; i < lambda_eq.size(); ++i) {
        const double old = lambda_eq[i];
        lambda_eq[i] = std::clamp(
            lambda_eq[i] + rho * c_final.eq[i],
            -config.al_multiplier_clip,
            config.al_multiplier_clip);
        max_dual_delta = std::max(max_dual_delta, std::abs(lambda_eq[i] - old));
      }
      for (std::size_t i = 0; i < lambda_ineq.size(); ++i) {
        const double old = lambda_ineq[i];
        lambda_ineq[i] = std::clamp(
            std::max(0.0, lambda_ineq[i] + rho * c_final.ineq[i]),
            0.0,
            config.al_multiplier_clip);
        max_dual_delta = std::max(max_dual_delta, std::abs(lambda_ineq[i] - old));
      }

      const std::vector<double> x_outer = inner_out.minimizer
          .reshape({inner_out.minimizer.numel()})
          .to(ScalarType::Float64)
          .to_vec<double>();
      const bool primal_ok = cv <= config.constraint_tolerance;
      const bool step_small = !prev_outer_x.empty() &&
          (l2_norm_diff(x_outer, prev_outer_x) <=
           config.step_tolerance * (1.0 + l2_norm(x_outer)));
      const bool value_small = std::isfinite(prev_outer_f) &&
          (std::abs(f_true - prev_outer_f) <=
           config.value_tolerance * (1.0 + std::abs(f_true)));
      const bool dual_small = max_dual_delta <= std::max(config.value_tolerance, 1e-10);
      if (primal_ok && (step_small || value_small || dual_small)) {
        converged = true;
        break;
      }

      if (std::isfinite(prev_outer_cv) &&
          cv > config.al_violation_reduction_ratio * prev_outer_cv) {
        rho *= config.al_penalty_growth;
      }
      prev_outer_cv = cv;
      prev_outer_f = f_true;
      prev_outer_x = x_outer;
      outer_initial = inner_out.minimizer;

      if (remaining_iterations <= 0) {
        exhausted_iterations = true;
        break;
      }
      if (remaining_evaluations <= 0) {
        exhausted_evaluations = true;
        break;
      }
    }

    if (!have_best) {
      ::pluss::throw_error(std::string(op) + ": augmented Lagrangian loop failed to produce an iterate");
    }

    best_out.diagnostics.iterations = total_iterations;
    best_out.diagnostics.evaluations = total_evaluations;
    best_out.diagnostics.cache_hits = total_cache_hits;
    best_out.diagnostics.nups_updates = total_nups_updates;
    best_out.diagnostics.search_trials = total_search_trials;
    best_out.diagnostics.search_successes = total_search_successes;
    best_out.diagnostics.outer_iterations = outer_iterations;
    best_out.diagnostics.penalty = rho;
    best_out.diagnostics.last_step = "outer_al";
    if (converged) {
      best_out.status = PSStatus::Converged;
    } else if (timed_out) {
      best_out.status = PSStatus::ReachedTimeBudget;
    } else if (exhausted_evaluations) {
      best_out.status = PSStatus::ReachedEvaluationBudget;
    } else if (exhausted_iterations) {
      best_out.status = PSStatus::ReachedIterationBudget;
    } else if (best_out.diagnostics.max_constraint_violation > config.constraint_tolerance) {
      best_out.status = PSStatus::InfeasibleStationary;
    }
    return best_out;
  }

  const CommonSetup setup = make_common_setup(initial, config.promote_cpu_to_float64);
  const BoundsPack bounds = prepare_bounds(config.lower_bounds, config.upper_bounds, setup, op);
  const LinearConstraints linear = prepare_linear_constraints(config, setup, op);

  const std::int64_t max_iterations = resolved_max_iterations(config, setup.dimension);
  const std::int64_t max_evaluations = resolved_max_evaluations(config, setup.dimension);

  std::mt19937_64 rng(config.rng_seed);
  std::vector<std::vector<double>> poll_basis = make_identity_basis(setup.dimension);
  std::int64_t last_basis_refresh_iter = -1;
  double mesh = std::min(config.initial_mesh_size, config.max_mesh_size);
  double penalty = config.initial_penalty;

  std::vector<double> incumbent = setup.flat_initial.to(ScalarType::Float64).to_vec<double>();
  project_to_bounds(&incumbent, bounds);
  restore_linear_feasibility(&incumbent, linear, bounds, config.constraint_tolerance);

  std::int64_t evaluations = 0;
  std::int64_t cache_hits = 0;
  EvaluationCache eval_cache;
  eval_cache.max_entries = config.enable_cache ? config.cache_max_entries : 0;
  eval_cache.tol = config.cache_tolerance;
  EvaluationCache* cache_ptr = config.enable_cache ? &eval_cache : nullptr;
  CandidateEvaluation incumbent_eval = evaluate_candidate(
      objective,
      config,
      setup,
      bounds,
      linear,
      incumbent,
      penalty,
      cache_ptr,
      &evaluations,
      &cache_hits,
      op);

  std::vector<double> best_feasible_x{};
  double best_feasible_objective = std::numeric_limits<double>::infinity();
  if (incumbent_eval.feasible) {
    best_feasible_x = incumbent;
    best_feasible_objective = incumbent_eval.objective;
  }

  std::vector<double> last_success_direction;
  NupsScaleState nups_state;
  std::int64_t iterations = 0;
  std::int64_t nups_updates = 0;
  std::int64_t search_trials_total = 0;
  std::int64_t search_successes = 0;
  double delta_x = std::numeric_limits<double>::infinity();
  double delta_f = std::numeric_limits<double>::infinity();
  std::string last_step = "bootstrap";

  bool converged = false;
  bool timed_out = false;
  bool infeasible_stationary = false;

  const auto t0 = std::chrono::steady_clock::now();
  double rel_window_best = best_feasible_objective;

  maybe_print_iteration(config.display, iterations, evaluations, incumbent_eval, mesh, last_step);

  while (iterations < max_iterations && evaluations < max_evaluations) {
    const auto now = std::chrono::steady_clock::now();
    const double elapsed =
        std::chrono::duration<double>(now - t0).count();
    if (elapsed > config.time_limit_seconds) {
      timed_out = true;
      break;
    }

    if (iterations > 0 && incumbent_eval.feasible && mesh <= config.mesh_tolerance &&
        (delta_x <= config.step_tolerance || delta_f <= config.value_tolerance)) {
      converged = true;
      break;
    }

    if (iterations > 0 && !incumbent_eval.feasible && mesh <= config.mesh_tolerance) {
      infeasible_stationary = true;
      break;
    }

    if (config.check_every > 0 && config.relative_improvement_tolerance > 0.0 &&
        iterations > 0 && (iterations % config.check_every) == 0 &&
        std::isfinite(best_feasible_objective)) {
      const double rel = std::abs(best_feasible_objective - rel_window_best) /
          (std::numeric_limits<double>::epsilon() + std::abs(best_feasible_objective));
      if (best_feasible_objective < rel_window_best) {
        rel_window_best = best_feasible_objective;
      }
      if (rel <= config.relative_improvement_tolerance && incumbent_eval.feasible) {
        converged = true;
        break;
      }
    }

    bool improved = false;
    CandidateEvaluation best_eval = incumbent_eval;
    std::vector<double> best_x = incumbent;
    std::vector<double> best_dir;
    last_step = "refine_mesh";
    const auto [mesh_scale, mesh_scale_factor] =
        compute_mesh_scale(incumbent, bounds, config, &nups_state);

    auto evaluate_search_direction =
        [&](const std::vector<double>& direction, const char* step_name) -> bool {
      if (evaluations >= max_evaluations) {
        return false;
      }
      std::vector<double> trial;
      std::vector<double> used_direction;
      if (!propose_trial_from_direction(
              incumbent,
              direction,
              mesh_scale,
              mesh_scale_factor,
              linear,
              bounds,
              config.step_tolerance,
              config.active_constraint_tolerance,
              &trial,
              &used_direction)) {
        return false;
      }

      ++search_trials_total;
      CandidateEvaluation trial_eval = evaluate_candidate(
          objective,
          config,
          setup,
          bounds,
          linear,
          trial,
          penalty,
          cache_ptr,
          &evaluations,
          &cache_hits,
          op);

      if (!is_better(trial_eval, best_eval, config.min_improvement)) {
        return false;
      }

      improved = true;
      best_eval = trial_eval;
      best_x = trial;
      best_dir = used_direction;
      last_step = step_name;
      ++search_successes;
      return true;
    };

    const bool run_expand_search =
        (config.search_strategy == PSSearchStrategy::ExpandAlongSuccess ||
         config.search_strategy == PSSearchStrategy::ExpandAndRandomDirections);
    const bool run_random_search =
        (config.search_strategy == PSSearchStrategy::RandomDirections ||
         config.search_strategy == PSSearchStrategy::ExpandAndRandomDirections);

    if (run_expand_search && !last_success_direction.empty() &&
        evaluations < max_evaluations) {
      std::vector<double> expanded(last_success_direction.size(), 0.0);
      for (std::size_t j = 0; j < expanded.size(); ++j) {
        expanded[j] = config.search_expansion * mesh * last_success_direction[j];
      }
      (void)evaluate_search_direction(expanded, "search_expand");
    }

    if (run_random_search &&
        (!improved || config.search_complete) &&
        evaluations < max_evaluations) {
      std::normal_distribution<double> normal(0.0, 1.0);
      const std::size_t n = incumbent.size();
      for (std::int64_t k = 0; k < config.search_trials && evaluations < max_evaluations; ++k) {
        if (improved && !config.search_complete) {
          break;
        }

        std::vector<double> direction(n, 0.0);
        for (double& v : direction) {
          v = normal(rng);
        }
        const double nrm = l2_norm(direction);
        if (nrm <= 1e-14) {
          continue;
        }
        const double alpha = config.search_expansion * mesh / nrm;
        for (double& v : direction) {
          v *= alpha;
        }

        const bool accepted = evaluate_search_direction(direction, "search_random");
        if (accepted && !config.search_complete) {
          break;
        }
      }
    }

    if (!improved && evaluations < max_evaluations) {
      maybe_refresh_poll_basis(
          config,
          setup.dimension,
          iterations,
          &rng,
          &poll_basis,
          &last_basis_refresh_iter);
      const auto raw_dirs = make_raw_poll_directions(poll_basis, config.poll_pattern);
      auto dirs = make_poll_directions(
          incumbent,
          raw_dirs,
          config.direction_family,
          linear,
          bounds,
          config.active_constraint_tolerance);
      dirs = order_directions(dirs, config.polling_order, last_success_direction, &rng);

      for (const auto& dir : dirs) {
        if (evaluations >= max_evaluations) {
          break;
        }

        std::vector<double> step(dir.size(), 0.0);
        for (std::size_t j = 0; j < step.size(); ++j) {
          step[j] = mesh * dir[j];
        }

        std::vector<double> trial;
        std::vector<double> used_direction;
        if (!propose_trial_from_direction(
                incumbent,
                step,
                mesh_scale,
                mesh_scale_factor,
                linear,
                bounds,
                config.step_tolerance,
                config.active_constraint_tolerance,
                &trial,
                &used_direction)) {
          continue;
        }

        CandidateEvaluation trial_eval = evaluate_candidate(
            objective,
            config,
            setup,
            bounds,
            linear,
            trial,
            penalty,
            cache_ptr,
            &evaluations,
            &cache_hits,
            op);

        if (is_better(trial_eval, best_eval, config.min_improvement)) {
          improved = true;
          best_eval = trial_eval;
          best_x = trial;
          best_dir = used_direction;
          last_step = "successful_poll";
          if (!config.complete_poll) {
            break;
          }
        }
      }
    }

    const std::vector<double> prev_x = incumbent;
    const double prev_f = incumbent_eval.objective;

    if (improved) {
      incumbent = std::move(best_x);
      incumbent_eval = best_eval;
      last_success_direction = std::move(best_dir);
      if (update_nups_scale_state(config, last_success_direction, true, &nups_state)) {
        ++nups_updates;
      }
      mesh = std::min(config.max_mesh_size, mesh * config.mesh_expansion);

      if (incumbent_eval.feasible && incumbent_eval.objective < best_feasible_objective) {
        best_feasible_objective = incumbent_eval.objective;
        best_feasible_x = incumbent;
      }
    } else {
      mesh *= config.mesh_contraction;
      last_success_direction.clear();
      if (update_nups_scale_state(config, {}, false, &nups_state)) {
        ++nups_updates;
      }
      if (config.constraint_policy == PSConstraintPolicy::MeritPenalty &&
          incumbent_eval.max_violation > config.constraint_tolerance) {
        penalty *= config.penalty_growth;
      }
    }

    delta_x = l2_norm_diff(incumbent, prev_x);
    delta_f = std::abs(incumbent_eval.objective - prev_f);
    ++iterations;

    maybe_print_iteration(config.display, iterations, evaluations, incumbent_eval, mesh, last_step);
  }

  PSStatus status = PSStatus::ReachedIterationBudget;
  if (converged) {
    status = PSStatus::Converged;
  } else if (infeasible_stationary) {
    status = PSStatus::InfeasibleStationary;
  } else if (timed_out) {
    status = PSStatus::ReachedTimeBudget;
  } else if (evaluations >= max_evaluations) {
    status = PSStatus::ReachedEvaluationBudget;
  }

  std::vector<double> final_x = incumbent;
  double final_f = incumbent_eval.objective;
  double final_cv = incumbent_eval.max_violation;
  if (!best_feasible_x.empty() &&
      (!incumbent_eval.feasible || best_feasible_objective <= incumbent_eval.objective)) {
    final_x = best_feasible_x;
    final_f = best_feasible_objective;
    final_cv = 0.0;
  }

  const Series final_flat = host_vector_to_flat_series(final_x, setup);

  PSOutcome out;
  out.minimizer = final_flat
      .reshape(std::span<const std::int64_t>(setup.original_shape.data(), setup.original_shape.size()))
      .to(initial.platform(), initial.scalar_type());
  out.minimum = final_f;
  out.status = status;
  out.diagnostics.iterations = iterations;
  out.diagnostics.evaluations = evaluations;
  out.diagnostics.cache_hits = cache_hits;
  out.diagnostics.nups_updates = nups_updates;
  out.diagnostics.search_trials = search_trials_total;
  out.diagnostics.search_successes = search_successes;
  out.diagnostics.last_step = last_step;
  out.diagnostics.mesh_size = mesh;
  out.diagnostics.penalty = penalty;
  out.diagnostics.max_constraint_violation = final_cv;
  return out;
}

std::string to_string(PSStatus status) {
  switch (status) {
    case PSStatus::Converged:
      return "Converged";
    case PSStatus::ReachedIterationBudget:
      return "ReachedIterationBudget";
    case PSStatus::ReachedEvaluationBudget:
      return "ReachedEvaluationBudget";
    case PSStatus::ReachedTimeBudget:
      return "ReachedTimeBudget";
    case PSStatus::InfeasibleStationary:
      return "InfeasibleStationary";
  }
  return "ReachedIterationBudget";
}

} // namespace pluss::subsp::math::optimization
