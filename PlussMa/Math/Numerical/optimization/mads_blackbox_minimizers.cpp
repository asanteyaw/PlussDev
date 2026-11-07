#include "mads_blackbox_minimizers.h"

#include "pluss/tils/auxiliary/UseAux.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <deque>
#include <iomanip>
#include <iostream>
#include <limits>
#include <numeric>
#include <random>
#include <span>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace pluss::subsp::math::optimization {
namespace {

struct BoundsPack {
  bool enabled{false};
  Series lower{pluss::series::undefined()};
  Series upper{pluss::series::undefined()};
};

struct CommonSetup {
  std::vector<std::int64_t> original_shape{};
  std::int64_t dimension{0};
  Config work_cfg{};
  Series flat_initial{pluss::series::undefined()};
};

struct RngState {
  std::uint64_t seed{5489ULL};
  std::uint64_t counter{0ULL};

  std::uint64_t next() {
    const std::uint64_t phi = 0x9E3779B97F4A7C15ULL;
    ++counter;
    return seed + phi * counter;
  }
};

struct MadsState {
  Series incumbent{pluss::series::undefined()};
  double best_value{std::numeric_limits<double>::infinity()};
  std::int64_t iterations{0};
  std::int64_t evaluations{0};
  double mesh_size{0.0};
  double frame_size{0.0};
  double check_best{std::numeric_limits<double>::infinity()};
  std::string last_phase{};
};

struct CandidateBatchOutcome {
  Series best_point{pluss::series::undefined()};
  double best_value{std::numeric_limits<double>::infinity()};
  std::int64_t evaluations{0};
  bool improved{false};
};

double scalar_as_f64(const Series& x, const char* op) {
  if (!x.defined() || x.numel() != 1) {
    ::pluss::throw_error(std::string(op) + ": expected scalar Series");
  }
  return x.to(ScalarType::Float64).host<double>().front();
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
  out.flat_initial = initial.reshape({out.dimension}).to(out.work_cfg.platform, out.work_cfg.dtype).contiguous();
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
    ::pluss::throw_error(std::string(op) + ": bounds must have same number of elements as initial point");
  }
  if (!flat_lower.le(flat_upper).all().item<bool>()) {
    ::pluss::throw_error(std::string(op) + ": expected lower_bounds <= upper_bounds elementwise");
  }

  bounds.enabled = true;
  bounds.lower = flat_lower;
  bounds.upper = flat_upper;
  return bounds;
}

Series project_to_bounds_and_mesh(
    const Series& x,
    const BoundsPack& bounds,
    const Series& anchor,
    double mesh_size) {
  Series out = x;
  if (bounds.enabled) {
    out = out.maximum(bounds.lower).minimum(bounds.upper);
  }
  if (mesh_size > 0.0) {
    out = out.sub(anchor).div(mesh_size).round().mul(mesh_size).add(anchor);
    if (bounds.enabled) {
      out = out.maximum(bounds.lower).minimum(bounds.upper);
    }
  }
  return out;
}

double eval_candidate(
    const std::function<Series(const Series&)>& objective,
    const Series& candidate_flat,
    const std::vector<std::int64_t>& original_shape,
    const char* op) {
  const Series x = candidate_flat.reshape(
      std::span<const std::int64_t>(original_shape.data(), original_shape.size()));
  const Series out = objective(x);
  if (!out.defined() || out.numel() != 1 || out.dtype().is_complex()) {
    ::pluss::throw_error(std::string(op) + ": objective must return real scalar Series");
  }
  const double value = scalar_as_f64(out, op);
  return std::isfinite(value) ? value : std::numeric_limits<double>::infinity();
}

bool converged_by_relative_improvement(
    std::int64_t iter,
    std::int64_t check_every,
    double current_best,
    double* check_best,
    double tol) {
  if (check_every < 1 || iter % check_every != 0) {
    return false;
  }
  const double rel_change = std::abs(current_best - *check_best) /
      (std::numeric_limits<double>::epsilon() + std::abs(current_best));
  if (current_best < *check_best) {
    *check_best = current_best;
  }
  return rel_change <= tol;
}

void maybe_print_iteration(const MadsState& state, BlackboxSolverDisplay display) {
  if (display != BlackboxSolverDisplay::Iter) {
    return;
  }
  std::ostringstream oss;
  oss << "iter=" << state.iterations
      << " evals=" << state.evaluations
      << " f=" << state.best_value
      << " phase=" << state.last_phase
      << " mesh=" << state.mesh_size
      << " frame=" << state.frame_size;
  std::cout << oss.str() << "\n";
}

// Radical inverse of `index` in the given prime base: the van der Corput digit reversal that Halton sequences are built from.
double radical_inverse(std::int64_t index, std::int64_t base) {
  double result = 0.0;
  double fraction = 1.0 / static_cast<double>(base);
  while (index > 0) {
    result += fraction * static_cast<double>(index % base);
    index /= base;
    fraction /= static_cast<double>(base);
  }
  return result;
}

std::int64_t nth_prime(std::int64_t n) {  // n = 0 is 2
  std::int64_t count = -1;
  for (std::int64_t candidate = 2;; ++candidate) {
    bool prime = true;
    for (std::int64_t d = 2; d * d <= candidate; ++d) {
      if (candidate % d == 0) { prime = false; break; }
    }
    if (prime && ++count == n) return candidate;
  }
}

// The poll directions of OrthoMADS: an orthonormal basis (and its negatives), different at every iteration. A point of the
// Halton sequence gives a direction u in the unit sphere, and the Householder reflection  Q = I - 2 u u'  turns the
// coordinate axes into an orthonormal basis that is turned a little further at each iteration, so that the directions
// polled become dense in the sphere as the poll size shrinks. A fixed set of coordinate axes would be coordinate
// pattern search: it cannot move along a diagonal constraint boundary, whatever the mesh.
Series poll_offsets_ortho_2n(std::int64_t n_dim, double frame_size, const Config& cfg, std::int64_t halton_index) {
  std::vector<double> u(static_cast<std::size_t>(n_dim));
  double norm2 = 0.0;
  for (std::int64_t i = 0; i < n_dim; ++i) {
    u[static_cast<std::size_t>(i)] = 2.0 * radical_inverse(halton_index, nth_prime(i)) - 1.0;
    norm2 += u[static_cast<std::size_t>(i)] * u[static_cast<std::size_t>(i)];
  }
  if (norm2 < 1e-12) {
    std::fill(u.begin(), u.end(), 0.0);
    u[0] = 1.0;
    norm2 = 1.0;
  }
  const double norm = std::sqrt(norm2);
  for (double& v : u) v /= norm;
  std::vector<double> offsets(static_cast<std::size_t>(2 * n_dim * n_dim), 0.0);
  for (std::int64_t i = 0; i < n_dim; ++i) {      // row i of Q is the image of axis i (Q is symmetric)
    for (std::int64_t j = 0; j < n_dim; ++j) {
      const double q = (i == j ? 1.0 : 0.0) - 2.0 * u[static_cast<std::size_t>(i)] * u[static_cast<std::size_t>(j)];
      offsets[static_cast<std::size_t>(i * n_dim + j)] = frame_size * q;
      offsets[static_cast<std::size_t>((i + n_dim) * n_dim + j)] = -frame_size * q;
    }
  }
  return pluss::make_series(offsets, {2 * n_dim, n_dim}, cfg).to(cfg.platform, cfg.dtype);
}

Series poll_offsets_ltmads_random_2n(
    std::int64_t n_dim,
    double frame_size,
    const Config& cfg,
    std::mt19937_64* host_rng) {
  std::normal_distribution<double> gauss(0.0, 1.0);

  std::vector<std::vector<double>> q(
      static_cast<std::size_t>(n_dim),
      std::vector<double>(static_cast<std::size_t>(n_dim), 0.0));
  for (std::int64_t i = 0; i < n_dim; ++i) {
    for (std::int64_t j = 0; j < n_dim; ++j) {
      q[static_cast<std::size_t>(i)][static_cast<std::size_t>(j)] = gauss(*host_rng);
    }
  }

  // Modified Gram-Schmidt.
  for (std::int64_t i = 0; i < n_dim; ++i) {
    auto& vi = q[static_cast<std::size_t>(i)];
    for (std::int64_t k = 0; k < i; ++k) {
      const auto& vk = q[static_cast<std::size_t>(k)];
      double dot = 0.0;
      for (std::int64_t j = 0; j < n_dim; ++j) {
        dot += vi[static_cast<std::size_t>(j)] * vk[static_cast<std::size_t>(j)];
      }
      for (std::int64_t j = 0; j < n_dim; ++j) {
        vi[static_cast<std::size_t>(j)] -= dot * vk[static_cast<std::size_t>(j)];
      }
    }
    double norm = 0.0;
    for (double v : vi) {
      norm += v * v;
    }
    norm = std::sqrt(norm);
    if (norm < 1e-12) {
      std::fill(vi.begin(), vi.end(), 0.0);
      vi[static_cast<std::size_t>(i)] = 1.0;
      norm = 1.0;
    }
    for (double& v : vi) {
      v /= norm;
    }
  }

  std::vector<double> offsets(
      static_cast<std::size_t>(2 * n_dim * n_dim),
      0.0);
  for (std::int64_t i = 0; i < n_dim; ++i) {
    for (std::int64_t j = 0; j < n_dim; ++j) {
      const double v = frame_size * q[static_cast<std::size_t>(i)][static_cast<std::size_t>(j)];
      offsets[static_cast<std::size_t>(i * n_dim + j)] = v;
      offsets[static_cast<std::size_t>((i + n_dim) * n_dim + j)] = -v;
    }
  }

  return pluss::make_series(offsets, {2 * n_dim, n_dim}, cfg)
      .to(cfg.platform, cfg.dtype);
}

CandidateBatchOutcome evaluate_offsets(
    const std::function<Series(const Series&)>& objective,
    const Series& incumbent,
    double incumbent_value,
    const Series& offsets,
    const BoundsPack& bounds,
    const Series& anchor,
    double mesh_size,
    const std::vector<std::int64_t>& original_shape,
    bool opportunistic,
    double min_improvement,
    std::int64_t max_evaluations,
    const MadsState& state,
    const char* op) {
  CandidateBatchOutcome out;
  out.best_point = incumbent;
  out.best_value = incumbent_value;
  out.evaluations = state.evaluations;

  const std::int64_t n_rows = offsets.sizes()[0];
  const std::int64_t n_dim = offsets.sizes()[1];
  const Series base = incumbent.reshape({1, n_dim}).expand(std::vector<std::int64_t>{n_rows, n_dim});
  const Series candidates = base.add(offsets);

  for (std::int64_t i = 0; i < n_rows; ++i) {
    if (out.evaluations >= max_evaluations) {
      break;
    }
    const Series raw = candidates.select(0, i);
    const Series point = project_to_bounds_and_mesh(raw, bounds, anchor, mesh_size);
    const double value = eval_candidate(objective, point, original_shape, op);
    ++out.evaluations;

    // An improvement smaller than rounding error in the objective is not progress; without this floor the last digits of a
    // flat minimum keep registering as successes, the frame keeps growing back, and the run never reaches `mesh_tolerance`.
    const double rounding_floor = std::isfinite(out.best_value)
        ? 8.0 * std::numeric_limits<double>::epsilon() * std::abs(out.best_value) : 0.0;
    if (value + std::max(min_improvement, rounding_floor) < out.best_value) {
      out.best_value = value;
      out.best_point = point;
      out.improved = true;
      if (opportunistic) {
        break;
      }
    }
  }

  return out;
}

double initial_mesh_guess(const BoundsPack& bounds, const Series& flat_initial) {
  if (bounds.enabled) {
    const Series span = bounds.upper.sub(bounds.lower).abs().mean();
    return std::max(1e-6, scalar_as_f64(span, "mesh_adaptive_direct_search_minimizer") * 0.05);
  }
  const Series scale = flat_initial.abs().mean();
  return std::max(1e-6, scalar_as_f64(scale, "mesh_adaptive_direct_search_minimizer") * 0.1 + 0.01);
}

MadsState initialize_state(
    const std::function<Series(const Series&)>& objective,
    const CommonSetup& setup,
    const BoundsPack& bounds,
    const MadsBlackboxConfig& config,
    const char* op) {
  MadsState state;
  state.mesh_size =
      (config.initial_mesh_size > 0.0) ? config.initial_mesh_size
                                       : initial_mesh_guess(bounds, setup.flat_initial);
  state.frame_size =
      (config.initial_frame_size > 0.0) ? config.initial_frame_size
                                        : (2.0 * state.mesh_size);
  state.incumbent =
      project_to_bounds_and_mesh(setup.flat_initial, bounds, setup.flat_initial, state.mesh_size);
  state.best_value = eval_candidate(objective, state.incumbent, setup.original_shape, op);
  state.evaluations = 1;
  state.check_best = state.best_value;
  state.last_phase = "bootstrap";
  return state;
}

BlackboxSolverOutcome build_outcome(
    const MadsState& state,
    const CommonSetup& setup,
    const BlackboxSolverStatus status) {
  BlackboxSolverOutcome out;
  out.minimizer = state.incumbent
      .reshape(std::span<const std::int64_t>(setup.original_shape.data(), setup.original_shape.size()))
      .to(setup.flat_initial.platform(), setup.flat_initial.scalar_type());
  out.minimum = state.best_value;
  out.status = status;
  out.diagnostics.iterations = state.iterations;
  out.diagnostics.evaluations = state.evaluations;
  out.diagnostics.last_phase = state.last_phase;
  out.diagnostics.mesh_size = state.mesh_size;
  out.diagnostics.frame_size = state.frame_size;
  return out;
}

} // namespace

BlackboxSolverOutcome mesh_adaptive_direct_search_minimizer(
    const std::function<Series(const Series&)>& objective,
    const Series& initial,
    const MadsBlackboxConfig& config) {
  constexpr const char* op = "mesh_adaptive_direct_search_minimizer";
  require_valid_initial(initial, op);

  if (config.mesh_refine_factor <= 0.0 || config.mesh_refine_factor >= 1.0 ||
      config.frame_refine_factor <= 0.0 || config.frame_refine_factor >= 1.0) {
    ::pluss::throw_error(std::string(op) + ": refine factors must be in (0,1)");
  }
  if (config.mesh_expand_factor < 1.0 || config.frame_expand_factor < 1.0) {
    ::pluss::throw_error(std::string(op) + ": expand factors must be >= 1");
  }
  if (config.mesh_tolerance <= 0.0) {
    ::pluss::throw_error(std::string(op) + ": mesh_tolerance must be > 0");
  }

  const CommonSetup setup = make_common_setup(initial, config.promote_cpu_to_float64);
  const BoundsPack bounds = prepare_bounds(config.lower_bounds, config.upper_bounds, setup, op);

  const std::int64_t n_dim = setup.dimension;
  const std::int64_t max_iterations =
      (config.max_iterations > 0) ? config.max_iterations : (250 * n_dim);
  const std::int64_t max_evaluations =
      (config.max_evaluations > 0) ? config.max_evaluations : (500 * n_dim);

  MadsState state = initialize_state(objective, setup, bounds, config, op);
  maybe_print_iteration(state, config.display);

  RngState series_rng{config.rng_seed, 0ULL};
  std::mt19937_64 host_rng(config.rng_seed ^ 0xD1B54A32D192ED03ULL);

  bool converged = false;
  while (state.iterations < max_iterations && state.evaluations < max_evaluations) {
    MadsState next = state;
    ++next.iterations;

    bool improved = false;
    if (config.search_points > 0 && next.evaluations < max_evaluations) {
      const Series search_offsets = pluss::series::rand(
                                        {config.search_points, n_dim},
                                        setup.work_cfg,
                                        series_rng.next())
                                        .mul(2.0 * next.frame_size)
                                        .sub(next.frame_size);

      const CandidateBatchOutcome search_out = evaluate_offsets(
          objective,
          next.incumbent,
          next.best_value,
          search_offsets,
          bounds,
          next.incumbent,
          next.mesh_size,
          setup.original_shape,
          config.opportunistic,
          config.min_improvement,
          max_evaluations,
          next,
          op);

      next.evaluations = search_out.evaluations;
      if (search_out.improved) {
        next.incumbent = search_out.best_point;
        next.best_value = search_out.best_value;
        improved = true;
        next.last_phase = "accept_search";
      }
    }

    if (!improved && next.evaluations < max_evaluations) {
      Series poll_offsets = pluss::series::undefined();
      if (config.poll_scheme == MadsPollScheme::Ortho2N) {
        poll_offsets = poll_offsets_ortho_2n(n_dim, next.frame_size, setup.work_cfg, next.iterations);
      } else {
        poll_offsets = poll_offsets_ltmads_random_2n(
            n_dim, next.frame_size, setup.work_cfg, &host_rng);
      }

      const CandidateBatchOutcome poll_out = evaluate_offsets(
          objective,
          next.incumbent,
          next.best_value,
          poll_offsets,
          bounds,
          next.incumbent,
          next.mesh_size,
          setup.original_shape,
          config.opportunistic,
          config.min_improvement,
          max_evaluations,
          next,
          op);

      next.evaluations = poll_out.evaluations;
      if (poll_out.improved) {
        next.incumbent = poll_out.best_point;
        next.best_value = poll_out.best_value;
        improved = true;
        next.last_phase = "accept_poll";
      }
    }

    if (improved) {
      next.frame_size *= config.frame_expand_factor;
      next.mesh_size = std::min(next.mesh_size * config.mesh_expand_factor, next.frame_size);
    } else {
      next.frame_size *= config.frame_refine_factor;
      next.mesh_size = std::min(next.mesh_size * config.mesh_refine_factor, next.frame_size);
      next.last_phase = "refine_mesh";
    }

    maybe_print_iteration(next, config.display);

    // check_every < 1 switches the improvement-window test off, leaving the frame size as the only convergence test
    const bool rel_conv = config.check_every < 1 || converged_by_relative_improvement(
        next.iterations,
        config.check_every,
        next.best_value,
        &next.check_best,
        config.relative_improvement_tolerance);
    const bool mesh_conv = next.frame_size <= config.mesh_tolerance;  // the poll size is the accuracy reached
    state = std::move(next);
    if (rel_conv && mesh_conv) {
      converged = true;
      break;
    }
  }

  const BlackboxSolverStatus status = pluss::tils::suppose(converged)
      .then([&](bool ok) {
        if (ok) {
          return BlackboxSolverStatus::Converged;
        }
        return (state.evaluations >= max_evaluations)
            ? BlackboxSolverStatus::ReachedEvaluationBudget
            : BlackboxSolverStatus::ReachedIterationBudget;
      })
      .value();

  return build_outcome(state, setup, status);
}

BlackboxSolverOutcome orthomads_minimizer(
    const std::function<Series(const Series&)>& objective,
    const Series& initial,
    const MadsBlackboxConfig& config) {
  MadsBlackboxConfig cfg = config;
  cfg.poll_scheme = MadsPollScheme::Ortho2N;
  return mesh_adaptive_direct_search_minimizer(objective, initial, cfg);
}

BlackboxSolverOutcome ltmads_minimizer(
    const std::function<Series(const Series&)>& objective,
    const Series& initial,
    const MadsBlackboxConfig& config) {
  MadsBlackboxConfig cfg = config;
  cfg.poll_scheme = MadsPollScheme::LtMadsRandom2N;
  return mesh_adaptive_direct_search_minimizer(objective, initial, cfg);
}

namespace {

struct AdvancedEvalRecord {
  Series objectives{pluss::series::undefined()};
  Series constraints{pluss::series::undefined()};
  double merit{std::numeric_limits<double>::infinity()};
  double violation{std::numeric_limits<double>::infinity()};
  bool feasible{false};
  bool evaluation_ok{false};
  bool count_evaluation{true};
};

struct AdvancedState {
  Series incumbent{pluss::series::undefined()};
  Series incumbent_objectives{pluss::series::undefined()};
  double best_merit{std::numeric_limits<double>::infinity()};
  double check_best{std::numeric_limits<double>::infinity()};
  double mesh_size{0.0};
  double frame_size{0.0};
  std::int64_t iterations{0};
  std::int64_t evaluations{0};
  std::int64_t total_evaluations{0};
  std::int64_t cache_hits{0};
  std::int64_t surrogate_orderings{0};
  std::int64_t objective_dim{-1};
  std::int64_t constraint_dim{-1};
  std::string last_phase{"bootstrap"};
};

struct AdvancedBatchOutcome {
  Series best_point{pluss::series::undefined()};
  Series best_objectives{pluss::series::undefined()};
  double best_merit{std::numeric_limits<double>::infinity()};
  bool improved{false};
};

double positive_violation_sum(
    const Series& constraints,
    double tolerance,
    const char* op) {
  if (!constraints.defined()) {
    return 0.0;
  }
  const Series c = constraints.to(ScalarType::Float64).contiguous();
  if (c.numel() == 0) {
    return 0.0;
  }
  const std::vector<double> values = c.host<double>().array().data;
  double sum = 0.0;
  for (double v : values) {
    if (!std::isfinite(v)) {
      ::pluss::throw_error(std::string(op) + ": constraint values must be finite");
    }
    const double pos = v - tolerance;
    if (pos > 0.0) {
      sum += pos;
    }
  }
  return sum;
}

std::string point_cache_key(const Series& point, std::int64_t round_digits) {
  const Series p = point.to(ScalarType::Float64).contiguous();
  const std::vector<double> v = p.host<double>().array().data;
  const std::int64_t d = std::clamp<std::int64_t>(round_digits, 0, 15);
  const double scale = std::pow(10.0, static_cast<double>(d));
  std::ostringstream oss;
  oss << std::fixed << std::setprecision(static_cast<int>(d));
  for (double x : v) {
    const double q = std::round(x * scale) / scale;
    oss << q << ";";
  }
  return oss.str();
}

bool dominates(
    const std::vector<double>& a,
    const std::vector<double>& b,
    double eps) {
  if (a.size() != b.size()) {
    return false;
  }
  bool strict = false;
  for (std::size_t i = 0; i < a.size(); ++i) {
    if (a[i] > b[i] + eps) {
      return false;
    }
    if (a[i] + eps < b[i]) {
      strict = true;
    }
  }
  return strict;
}

Config select_heavy_cfg(
    const MadsBlackboxAdvancedConfig& config,
    const Config& work_cfg) {
  Config heavy = Config{}.with_dtype(config.heavy_dtype).with_platform(work_cfg.platform);
  if (!config.accelerate_heavy_kernels) {
    return Config{}.with_dtype(work_cfg.dtype).with_platform(work_cfg.platform);
  }

  if (config.auto_heavy_platform) {
    if (pluss::platform_available(pluss::pMPS)) {
      heavy = heavy.with_platform(pluss::pMPS);
    } else {
      heavy = heavy.with_platform(work_cfg.platform);
    }
  } else {
    heavy = heavy.with_platform(
        pluss::platform_available(config.heavy_platform) ? config.heavy_platform : work_cfg.platform);
  }

  if (heavy.platform == pluss::pMPS) {
    heavy = heavy.with_dtype(ScalarType::Float32);
  }
  return heavy;
}

void merge_advanced_diagnostics(
    BlackboxAdvancedDiagnostics* accum,
    const BlackboxAdvancedDiagnostics& add) {
  accum->iterations += add.iterations;
  accum->evaluations += add.evaluations;
  accum->total_evaluations += add.total_evaluations;
  accum->cache_hits += add.cache_hits;
  accum->surrogate_orderings += add.surrogate_orderings;
  accum->pareto_size = std::max(accum->pareto_size, add.pareto_size);
  accum->objective_dimension = std::max(accum->objective_dimension, add.objective_dimension);
  accum->constraint_dimension = std::max(accum->constraint_dimension, add.constraint_dimension);
  accum->mesh_size = add.mesh_size;
  accum->frame_size = add.frame_size;
  accum->last_phase = add.last_phase;
}

Series subset_from_indices(
    const Series& full_flat,
    const std::vector<std::int64_t>& indices,
    const Config& cfg) {
  const Series cpu = full_flat.to(ScalarType::Float64).contiguous();
  const std::vector<double> src = cpu.host<double>().array().data;
  std::vector<double> sub(indices.size(), 0.0);
  for (std::size_t i = 0; i < indices.size(); ++i) {
    sub[i] = src[static_cast<std::size_t>(indices[i])];
  }
  return pluss::make_series(
             sub,
             {static_cast<std::int64_t>(indices.size())},
             Config{}.with_dtype(ScalarType::Float64).with_platform(cfg.platform))
      .to(cfg.platform, cfg.dtype);
}

Series inject_from_indices(
    const Series& full_flat,
    const std::vector<std::int64_t>& indices,
    const Series& sub_flat,
    const Config& cfg) {
  const std::vector<double> dst0 = full_flat.to(ScalarType::Float64).contiguous().host<double>().array().data;
  const std::vector<double> src = sub_flat.to(ScalarType::Float64).contiguous().host<double>().array().data;
  std::vector<double> dst = dst0;
  for (std::size_t i = 0; i < indices.size(); ++i) {
    dst[static_cast<std::size_t>(indices[i])] = src[i];
  }
  return pluss::make_series(
             dst,
             {static_cast<std::int64_t>(dst.size())},
             Config{}.with_dtype(ScalarType::Float64).with_platform(cfg.platform))
      .to(cfg.platform, cfg.dtype);
}

Series project_flat_to_bounds(
    const Series& x_flat,
    const Series& lower,
    const Series& upper) {
  Series out = x_flat;
  if (lower.defined() && upper.defined()) {
    out = out.maximum(lower).minimum(upper);
  }
  return out;
}

} // namespace

BlackboxAdvancedOutcome mads_blackbox_minimizer(
    const std::function<BlackboxEvaluation(const Series&)>& blackbox,
    const Series& initial,
    const MadsBlackboxAdvancedConfig& config) {
  constexpr const char* op = "mads_blackbox_minimizer";
  require_valid_initial(initial, op);

  if (config.mesh_refine_factor <= 0.0 || config.mesh_refine_factor >= 1.0 ||
      config.frame_refine_factor <= 0.0 || config.frame_refine_factor >= 1.0) {
    ::pluss::throw_error(std::string(op) + ": refine factors must be in (0,1)");
  }
  if (config.mesh_expand_factor < 1.0 || config.frame_expand_factor < 1.0) {
    ::pluss::throw_error(std::string(op) + ": expand factors must be >= 1");
  }
  if (config.mesh_tolerance <= 0.0) {
    ::pluss::throw_error(std::string(op) + ": mesh_tolerance must be > 0");
  }
  if (config.progressive_barrier_penalty < 0.0) {
    ::pluss::throw_error(std::string(op) + ": progressive_barrier_penalty must be >= 0");
  }
  if (config.constraint_tolerance < 0.0) {
    ::pluss::throw_error(std::string(op) + ": constraint_tolerance must be >= 0");
  }

  const CommonSetup setup = make_common_setup(initial, config.promote_cpu_to_float64);
  const BoundsPack bounds = prepare_bounds(config.lower_bounds, config.upper_bounds, setup, op);

  const std::int64_t n_dim = setup.dimension;
  const std::int64_t max_iterations =
      (config.max_iterations > 0) ? config.max_iterations : (250 * n_dim);
  const std::int64_t max_evaluations =
      (config.max_evaluations > 0) ? config.max_evaluations : (500 * n_dim);
  const std::int64_t max_total_evaluations =
      (config.max_total_evaluations > 0) ? config.max_total_evaluations : (4 * max_evaluations);

  AdvancedState state;
  state.mesh_size =
      (config.initial_mesh_size > 0.0) ? config.initial_mesh_size
                                       : initial_mesh_guess(bounds, setup.flat_initial);
  state.frame_size =
      (config.initial_frame_size > 0.0) ? config.initial_frame_size
                                        : (2.0 * state.mesh_size);
  state.incumbent =
      project_to_bounds_and_mesh(setup.flat_initial, bounds, setup.flat_initial, state.mesh_size);

  std::unordered_map<std::string, AdvancedEvalRecord> cache;
  std::deque<std::string> cache_fifo;
  std::vector<std::vector<double>> train_x;
  std::vector<double> train_y;
  std::vector<double> objective_weights;
  std::vector<double> objective_reference;
  std::vector<Series> pareto_points;
  std::vector<Series> pareto_objectives;
  std::vector<std::vector<double>> pareto_values;
  const Config heavy_cfg = select_heavy_cfg(config, setup.work_cfg);

  auto ensure_multiobjective_vectors = [&](std::int64_t obj_dim) {
    if (!objective_weights.empty()) {
      return;
    }
    if (obj_dim < 1) {
      ::pluss::throw_error(std::string(op) + ": objective dimension must be >= 1");
    }
    if (config.objective_weights.defined()) {
      const Series w = config.objective_weights.reshape({config.objective_weights.numel()})
                           .to(ScalarType::Float64)
                           .contiguous();
      if (w.numel() != obj_dim) {
        ::pluss::throw_error(std::string(op) + ": objective_weights size mismatch");
      }
      objective_weights = w.host<double>().array().data;
    } else {
      objective_weights.assign(static_cast<std::size_t>(obj_dim), 1.0 / static_cast<double>(obj_dim));
    }

    if (config.objective_reference.defined()) {
      const Series r = config.objective_reference.reshape({config.objective_reference.numel()})
                           .to(ScalarType::Float64)
                           .contiguous();
      if (r.numel() != obj_dim) {
        ::pluss::throw_error(std::string(op) + ": objective_reference size mismatch");
      }
      objective_reference = r.host<double>().array().data;
    } else {
      objective_reference.assign(static_cast<std::size_t>(obj_dim), 0.0);
    }
  };

  auto scalarize_objectives = [&](const std::vector<double>& obj_values) {
    if (obj_values.empty()) {
      ::pluss::throw_error(std::string(op) + ": objective vector must be non-empty");
    }
    if (obj_values.size() == 1) {
      return obj_values.front();
    }
    ensure_multiobjective_vectors(static_cast<std::int64_t>(obj_values.size()));
    if (config.multiobjective_scalarization == MultiObjectiveScalarization::WeightedSum) {
      double s = 0.0;
      for (std::size_t i = 0; i < obj_values.size(); ++i) {
        s += objective_weights[i] * obj_values[i];
      }
      return s;
    }

    double worst = -std::numeric_limits<double>::infinity();
    for (std::size_t i = 0; i < obj_values.size(); ++i) {
      const double v = objective_weights[i] * std::abs(obj_values[i] - objective_reference[i]);
      worst = std::max(worst, v);
    }
    return worst;
  };

  auto maybe_update_pareto = [&](const Series& point, const std::vector<double>& obj_values, bool feasible) {
    if (!feasible || obj_values.empty()) {
      return;
    }

    bool dominated = false;
    for (const auto& old : pareto_values) {
      if (dominates(old, obj_values, config.pareto_epsilon)) {
        dominated = true;
        break;
      }
      // A point with the same objective vector as an archived one adds nothing; without this the archive fills with
      // repeats of the point the search keeps returning to, and the size limit then pushes the real front out.
      bool same = old.size() == obj_values.size();
      for (std::size_t k = 0; same && k < old.size(); ++k) {
        same = std::abs(old[k] - obj_values[k]) <= config.pareto_epsilon;
      }
      if (same) {
        dominated = true;
        break;
      }
    }
    if (dominated) {
      return;
    }

    std::vector<std::size_t> keep;
    keep.reserve(pareto_values.size());
    for (std::size_t i = 0; i < pareto_values.size(); ++i) {
      if (!dominates(obj_values, pareto_values[i], config.pareto_epsilon)) {
        keep.push_back(i);
      }
    }

    std::vector<Series> new_points;
    std::vector<Series> new_objs;
    std::vector<std::vector<double>> new_vals;
    new_points.reserve(keep.size() + 1);
    new_objs.reserve(keep.size() + 1);
    new_vals.reserve(keep.size() + 1);
    for (std::size_t idx : keep) {
      new_points.push_back(pareto_points[idx]);
      new_objs.push_back(pareto_objectives[idx]);
      new_vals.push_back(pareto_values[idx]);
    }
    new_points.push_back(point.to(ScalarType::Float64));
    new_objs.push_back(pluss::make_series(
                           obj_values,
                           {static_cast<std::int64_t>(obj_values.size())},
                           Config{}.with_dtype(ScalarType::Float64).with_platform(point.platform())));
    new_vals.push_back(obj_values);

    if (config.pareto_archive_limit > 0 &&
        static_cast<std::int64_t>(new_points.size()) > config.pareto_archive_limit) {
      // Over the limit: drop the most crowded entry, the one whose nearest neighbour in objective space is closest, so the
      // archive keeps its spread along the front. (Dropping the oldest instead would leave only the cluster of points
      // the search converged on.)
      std::size_t crowded = 0;
      double crowded_gap = std::numeric_limits<double>::infinity();
      for (std::size_t i = 0; i < new_vals.size(); ++i) {
        double nearest = std::numeric_limits<double>::infinity();
        for (std::size_t j = 0; j < new_vals.size(); ++j) {
          if (i == j) continue;
          double d2 = 0.0;
          for (std::size_t k = 0; k < new_vals[i].size(); ++k) {
            const double dk = new_vals[i][k] - new_vals[j][k];
            d2 += dk * dk;
          }
          nearest = std::min(nearest, d2);
        }
        if (nearest < crowded_gap) {
          crowded_gap = nearest;
          crowded = i;
        }
      }
      new_points.erase(new_points.begin() + static_cast<std::ptrdiff_t>(crowded));
      new_objs.erase(new_objs.begin() + static_cast<std::ptrdiff_t>(crowded));
      new_vals.erase(new_vals.begin() + static_cast<std::ptrdiff_t>(crowded));
    }

    pareto_points = std::move(new_points);
    pareto_objectives = std::move(new_objs);
    pareto_values = std::move(new_vals);
  };

  auto evaluate_point = [&](const Series& point) -> AdvancedEvalRecord {
    AdvancedEvalRecord out;
    const bool use_cache = config.enable_cache;
    std::string key;
    if (use_cache) {
      key = point_cache_key(point, config.cache_round_digits);
      if (const auto it = cache.find(key); it != cache.end()) {
        out = it->second;
        ++state.cache_hits;
        if (config.count_cache_hits_in_total_evaluations) {
          ++state.total_evaluations;
        }
        return out;
      }
    }

    const Series shaped = point.reshape(
        std::span<const std::int64_t>(setup.original_shape.data(), setup.original_shape.size()));
    const BlackboxEvaluation raw = blackbox(shaped);
    ++state.total_evaluations;
    if (raw.count_evaluation) {
      ++state.evaluations;
    }

    if (!raw.objectives.defined() || raw.objectives.numel() < 1 || raw.objectives.dtype().is_complex()) {
      ::pluss::throw_error(std::string(op) + ": blackbox must provide real objective values");
    }

    out.objectives = raw.objectives
                         .reshape({raw.objectives.numel()})
                         .to(ScalarType::Float64)
                         .contiguous();
    if (state.objective_dim < 0) {
      state.objective_dim = out.objectives.numel();
    } else if (state.objective_dim != out.objectives.numel()) {
      ::pluss::throw_error(std::string(op) + ": objective dimension changed across evaluations");
    }

    if (raw.constraints.defined()) {
      if (raw.constraints.dtype().is_complex()) {
        ::pluss::throw_error(std::string(op) + ": constraints must be real-valued");
      }
      out.constraints = raw.constraints
                            .reshape({raw.constraints.numel()})
                            .to(ScalarType::Float64)
                            .contiguous();
      if (state.constraint_dim < 0) {
        state.constraint_dim = out.constraints.numel();
      } else if (state.constraint_dim != out.constraints.numel()) {
        ::pluss::throw_error(std::string(op) + ": constraint dimension changed across evaluations");
      }
    } else if (state.constraint_dim < 0) {
      state.constraint_dim = 0;
    }

    out.evaluation_ok = raw.evaluation_ok;
    out.count_evaluation = raw.count_evaluation;
    out.violation = positive_violation_sum(out.constraints, config.constraint_tolerance, op);
    out.feasible = out.evaluation_ok;
    if (config.constraint_handling != ConstraintHandling::Ignore) {
      out.feasible = out.feasible && (out.violation <= 0.0);
    }

    const std::vector<double> obj_values = out.objectives.host<double>().array().data;
    const double scalarized = scalarize_objectives(obj_values);
    if (!std::isfinite(scalarized) || !out.evaluation_ok) {
      out.merit = std::numeric_limits<double>::infinity();
    } else if (config.constraint_handling == ConstraintHandling::Ignore) {
      out.merit = scalarized;
    } else if (config.constraint_handling == ConstraintHandling::ExtremeBarrier) {
      out.merit = out.feasible ? scalarized : std::numeric_limits<double>::infinity();
    } else {
      out.merit = scalarized + config.progressive_barrier_penalty * out.violation;
    }

    if (std::isfinite(out.merit)) {
      const std::vector<double> point_vec = point.to(ScalarType::Float64).host<double>().array().data;
      train_x.push_back(point_vec);
      train_y.push_back(out.merit);
      if (config.surrogate_max_points > 0 &&
          static_cast<std::int64_t>(train_x.size()) > config.surrogate_max_points) {
        train_x.erase(train_x.begin());
        train_y.erase(train_y.begin());
      }
    }
    maybe_update_pareto(point, obj_values, out.feasible);

    if (use_cache) {
      cache.emplace(key, out);
      cache_fifo.push_back(key);
      if (config.cache_max_entries > 0 &&
          static_cast<std::int64_t>(cache_fifo.size()) > config.cache_max_entries) {
        cache.erase(cache_fifo.front());
        cache_fifo.pop_front();
      }
    }

    return out;
  };

  auto reorder_offsets_by_surrogate = [&](const Series& offsets, const Series& anchor) {
    if (config.surrogate_mode == SurrogateMode::Off) {
      return offsets;
    }
    if (static_cast<std::int64_t>(train_x.size()) < std::max<std::int64_t>(1, config.surrogate_min_points)) {
      return offsets;
    }

    const std::int64_t n_rows = offsets.sizes()[0];
    const std::int64_t n_cols = offsets.sizes()[1];
    if (n_rows < 2) {
      return offsets;
    }

    const Series base = anchor.reshape({1, n_cols}).expand(std::vector<std::int64_t>{n_rows, n_cols});
    // Cast to the heavy dtype BEFORE moving to the heavy platform: an accelerator that has no float64 rejects a float64 transfer.
    const Series candidates = base.add(offsets)
                                .to(heavy_cfg.dtype)
                                .to(heavy_cfg.platform)
                                .contiguous();

    std::vector<double> train_flat(
        static_cast<std::size_t>(train_x.size() * static_cast<std::size_t>(n_cols)),
        0.0);
    for (std::size_t i = 0; i < train_x.size(); ++i) {
      for (std::int64_t j = 0; j < n_cols; ++j) {
        train_flat[i * static_cast<std::size_t>(n_cols) + static_cast<std::size_t>(j)] =
            train_x[i][static_cast<std::size_t>(j)];
      }
    }
    const Series train_points = pluss::make_series(
                                    train_flat,
                                    {static_cast<std::int64_t>(train_x.size()), n_cols},
                                    Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU))
                                    .to(heavy_cfg.dtype)
                                    .to(heavy_cfg.platform)
                                    .contiguous();
    const Series train_merit = pluss::make_series(
                                   train_y,
                                   {static_cast<std::int64_t>(train_y.size()), 1},
                                   Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU))
                                   .to(heavy_cfg.dtype)
                                   .to(heavy_cfg.platform)
                                   .contiguous();

    const double bw = (config.surrogate_bandwidth > 0.0)
        ? config.surrogate_bandwidth
        : std::max(1e-9, state.frame_size);
    const double bw2 = bw * bw;

    const Series x2 = candidates.square().sum(1, true);
    const Series t2 = train_points.square().sum(1, true).transpose(0, 1);
    const Series dots = candidates.matmul(train_points.transpose(0, 1));
    const Series dist2 = x2.add(t2).sub(dots.mul(2.0)).maximum(0.0);
    const Series weights = dist2.div(-2.0 * bw2).exp();
    const Series wsum = weights.sum(1, true).maximum(1e-20);
    const Series predicted = weights.matmul(train_merit).div(wsum).reshape({n_rows});

    const Series order_series = predicted.to(pluss::pCPU).to(ScalarType::Float64).argsort(0, false);  // leave the accelerator before widening
    const std::vector<std::int64_t> order =
        order_series.to(ScalarType::Int64).host<std::int64_t>().array().data;

    const Series offsets_cpu = offsets.to(ScalarType::Float64).contiguous();
    const std::vector<double> in = offsets_cpu.host<double>().array().data;
    std::vector<double> out(in.size(), 0.0);
    for (std::int64_t i = 0; i < n_rows; ++i) {
      const std::int64_t src = order[static_cast<std::size_t>(i)];
      for (std::int64_t j = 0; j < n_cols; ++j) {
        out[static_cast<std::size_t>(i * n_cols + j)] =
            in[static_cast<std::size_t>(src * n_cols + j)];
      }
    }

    ++state.surrogate_orderings;
    return pluss::make_series(out, {n_rows, n_cols}, setup.work_cfg)
        .to(setup.work_cfg.platform, setup.work_cfg.dtype);
  };

  const AdvancedEvalRecord initial_eval = evaluate_point(state.incumbent);
  state.best_merit = initial_eval.merit;
  state.check_best = state.best_merit;
  state.incumbent_objectives = initial_eval.objectives.to(setup.work_cfg.platform, setup.work_cfg.dtype);
  maybe_print_iteration(
      MadsState{
          .incumbent = state.incumbent,
          .best_value = state.best_merit,
          .iterations = state.iterations,
          .evaluations = state.evaluations,
          .mesh_size = state.mesh_size,
          .frame_size = state.frame_size,
          .check_best = state.check_best,
          .last_phase = state.last_phase},
      config.display);

  RngState series_rng{config.rng_seed, 0ULL};
  std::mt19937_64 host_rng(config.rng_seed ^ 0xD1B54A32D192ED03ULL);

  auto evaluate_offsets_batch = [&](const Series& offsets, const Series& anchor) {
    AdvancedBatchOutcome out;
    out.best_point = state.incumbent;
    out.best_objectives = state.incumbent_objectives;
    out.best_merit = state.best_merit;

    const std::int64_t n_rows = offsets.sizes()[0];
    const std::int64_t n_dim_local = offsets.sizes()[1];
    const Series base = anchor.reshape({1, n_dim_local}).expand(std::vector<std::int64_t>{n_rows, n_dim_local});
    const Series candidates = base.add(offsets);

    for (std::int64_t i = 0; i < n_rows; ++i) {
      if (state.evaluations >= max_evaluations || state.total_evaluations >= max_total_evaluations) {
        break;
      }
      const Series candidate = project_to_bounds_and_mesh(
          candidates.select(0, i), bounds, anchor, state.mesh_size);
      const AdvancedEvalRecord eval = evaluate_point(candidate);
      const double merit_floor = std::isfinite(out.best_merit)
          ? 8.0 * std::numeric_limits<double>::epsilon() * std::abs(out.best_merit) : 0.0;
      if (eval.merit + std::max(config.min_improvement, merit_floor) < out.best_merit) {
        out.best_merit = eval.merit;
        out.best_point = candidate;
        out.best_objectives = eval.objectives.to(setup.work_cfg.platform, setup.work_cfg.dtype);
        out.improved = true;
        if (config.opportunistic) {
          break;
        }
      }
    }
    return out;
  };

  bool converged = false;
  while (state.iterations < max_iterations &&
         state.evaluations < max_evaluations &&
         state.total_evaluations < max_total_evaluations) {
    ++state.iterations;
    bool improved = false;

    if (config.search_points > 0 &&
        state.evaluations < max_evaluations &&
        state.total_evaluations < max_total_evaluations) {
      Series search_offsets = pluss::series::rand(
                                  {config.search_points, n_dim},
                                  setup.work_cfg,
                                  series_rng.next())
                                  .mul(2.0 * state.frame_size)
                                  .sub(state.frame_size);
      search_offsets = reorder_offsets_by_surrogate(search_offsets, state.incumbent);

      const AdvancedBatchOutcome search_out = evaluate_offsets_batch(search_offsets, state.incumbent);
      if (search_out.improved) {
        improved = true;
        state.incumbent = search_out.best_point;
        state.incumbent_objectives = search_out.best_objectives;
        state.best_merit = search_out.best_merit;
        state.last_phase = "accept_search";
      }
    }

    if (!improved &&
        state.evaluations < max_evaluations &&
        state.total_evaluations < max_total_evaluations) {
      Series poll_offsets = pluss::series::undefined();
      if (config.poll_scheme == MadsPollScheme::Ortho2N) {
        poll_offsets = poll_offsets_ortho_2n(n_dim, state.frame_size, setup.work_cfg, state.iterations);
      } else {
        poll_offsets = poll_offsets_ltmads_random_2n(
            n_dim, state.frame_size, setup.work_cfg, &host_rng);
      }
      poll_offsets = reorder_offsets_by_surrogate(poll_offsets, state.incumbent);

      const AdvancedBatchOutcome poll_out = evaluate_offsets_batch(poll_offsets, state.incumbent);
      if (poll_out.improved) {
        improved = true;
        state.incumbent = poll_out.best_point;
        state.incumbent_objectives = poll_out.best_objectives;
        state.best_merit = poll_out.best_merit;
        state.last_phase = "accept_poll";
      }
    }

    if (improved) {
      state.frame_size *= config.frame_expand_factor;
      state.mesh_size = std::min(state.mesh_size * config.mesh_expand_factor, state.frame_size);
    } else {
      state.frame_size *= config.frame_refine_factor;
      state.mesh_size = std::min(state.mesh_size * config.mesh_refine_factor, state.frame_size);
      state.last_phase = "refine_mesh";
    }

    maybe_print_iteration(
        MadsState{
            .incumbent = state.incumbent,
            .best_value = state.best_merit,
            .iterations = state.iterations,
            .evaluations = state.evaluations,
            .mesh_size = state.mesh_size,
            .frame_size = state.frame_size,
            .check_best = state.check_best,
            .last_phase = state.last_phase},
        config.display);

    // check_every < 1 switches the improvement-window test off, leaving the frame size as the only convergence test
    const bool rel_conv = config.check_every < 1 || converged_by_relative_improvement(
        state.iterations,
        config.check_every,
        state.best_merit,
        &state.check_best,
        config.relative_improvement_tolerance);
    const bool mesh_conv = state.frame_size <= config.mesh_tolerance;  // the poll size is the accuracy reached
    if (rel_conv && mesh_conv) {
      converged = true;
      break;
    }
  }

  BlackboxAdvancedOutcome out;
  out.minimizer = state.incumbent
      .reshape(std::span<const std::int64_t>(setup.original_shape.data(), setup.original_shape.size()))
      .to(setup.flat_initial.platform(), setup.flat_initial.scalar_type());
  out.objective_vector = state.incumbent_objectives.to(ScalarType::Float64);
  out.scalarized_minimum = state.best_merit;
  out.status = pluss::tils::suppose(converged)
      .then([&](bool ok) {
        if (ok) {
          return BlackboxSolverStatus::Converged;
        }
        if (state.evaluations >= max_evaluations || state.total_evaluations >= max_total_evaluations) {
          return BlackboxSolverStatus::ReachedEvaluationBudget;
        }
        return BlackboxSolverStatus::ReachedIterationBudget;
      })
      .value();

  out.diagnostics.iterations = state.iterations;
  out.diagnostics.evaluations = state.evaluations;
  out.diagnostics.total_evaluations = state.total_evaluations;
  out.diagnostics.cache_hits = state.cache_hits;
  out.diagnostics.surrogate_orderings = state.surrogate_orderings;
  out.diagnostics.pareto_size = static_cast<std::int64_t>(pareto_points.size());
  out.diagnostics.objective_dimension = std::max<std::int64_t>(1, state.objective_dim);
  out.diagnostics.constraint_dimension = std::max<std::int64_t>(0, state.constraint_dim);
  out.diagnostics.last_phase = state.last_phase;
  out.diagnostics.mesh_size = state.mesh_size;
  out.diagnostics.frame_size = state.frame_size;

  if (!pareto_points.empty()) {
    const std::int64_t rows = static_cast<std::int64_t>(pareto_points.size());
    const std::int64_t obj_dim = static_cast<std::int64_t>(pareto_values.front().size());
    std::vector<double> points_buf;
    points_buf.reserve(static_cast<std::size_t>(rows * n_dim));
    for (const Series& p : pareto_points) {
      const std::vector<double> v = p.to(ScalarType::Float64).host<double>().array().data;
      points_buf.insert(points_buf.end(), v.begin(), v.end());
    }
    std::vector<double> obj_buf;
    obj_buf.reserve(static_cast<std::size_t>(rows * obj_dim));
    for (const auto& v : pareto_values) {
      obj_buf.insert(obj_buf.end(), v.begin(), v.end());
    }
    out.pareto_points = pluss::make_series(
                            points_buf,
                            {rows, n_dim},
                            Config{}.with_dtype(ScalarType::Float64).with_platform(setup.flat_initial.platform()))
                            .to(setup.flat_initial.platform(), setup.flat_initial.scalar_type());
    out.pareto_objectives = pluss::make_series(
        obj_buf,
        {rows, obj_dim},
        Config{}.with_dtype(ScalarType::Float64).with_platform(setup.flat_initial.platform()));
  }

  return out;
}

BlackboxAdvancedOutcome orthomads_blackbox_minimizer(
    const std::function<BlackboxEvaluation(const Series&)>& blackbox,
    const Series& initial,
    const MadsBlackboxAdvancedConfig& config) {
  MadsBlackboxAdvancedConfig cfg = config;
  cfg.poll_scheme = MadsPollScheme::Ortho2N;
  return mads_blackbox_minimizer(blackbox, initial, cfg);
}

BlackboxAdvancedOutcome ltmads_blackbox_minimizer(
    const std::function<BlackboxEvaluation(const Series&)>& blackbox,
    const Series& initial,
    const MadsBlackboxAdvancedConfig& config) {
  MadsBlackboxAdvancedConfig cfg = config;
  cfg.poll_scheme = MadsPollScheme::LtMadsRandom2N;
  return mads_blackbox_minimizer(blackbox, initial, cfg);
}

namespace {

double outcome_score(const BlackboxAdvancedOutcome& out) {
  if (!std::isfinite(out.scalarized_minimum)) {
    return std::numeric_limits<double>::infinity();
  }
  return out.scalarized_minimum;
}


// A decomposition solver spends part of the budget on its blocks and the rest on a final full-space run from the best point,
// because blocks cannot see the coupling between variables in different blocks; that last run is what removes the error the
// coupling leaves behind. The budgets are resolved first, so that an unset budget is divided too, not multiplied.
struct DecompositionBudget {
  std::int64_t iterations{0};
  std::int64_t evaluations{0};
  std::int64_t total{0};
  MadsBlackboxAdvancedConfig polish{};
  bool has_polish{false};
};

DecompositionBudget split_decomposition_budget(const MadsBlackboxAdvancedConfig& base, std::int64_t n, double polish_fraction) {
  const double frac = std::clamp(polish_fraction, 0.0, 0.9);
  const std::int64_t evals = (base.max_evaluations > 0) ? base.max_evaluations : (500 * n);
  const std::int64_t iters = (base.max_iterations > 0) ? base.max_iterations : (250 * n);
  const std::int64_t total = (base.max_total_evaluations > 0) ? base.max_total_evaluations : (4 * evals);
  DecompositionBudget out;
  out.iterations = std::max<std::int64_t>(1, static_cast<std::int64_t>(std::llround(static_cast<double>(iters) * (1.0 - frac))));
  out.evaluations = std::max<std::int64_t>(1, static_cast<std::int64_t>(std::llround(static_cast<double>(evals) * (1.0 - frac))));
  out.total = std::max<std::int64_t>(1, static_cast<std::int64_t>(std::llround(static_cast<double>(total) * (1.0 - frac))));
  out.polish = base;
  out.polish.max_iterations = std::max<std::int64_t>(1, iters - out.iterations);
  out.polish.max_evaluations = std::max<std::int64_t>(1, evals - out.evaluations);
  out.polish.max_total_evaluations = std::max<std::int64_t>(1, total - out.total);
  out.has_polish = frac > 0.0;
  return out;
}

bool better_outcome(const BlackboxAdvancedOutcome& a, const BlackboxAdvancedOutcome& b) {
  const double sa = outcome_score(a);
  const double sb = outcome_score(b);
  if (std::isfinite(sa) != std::isfinite(sb)) {
    return std::isfinite(sa);
  }
  if (sa < sb) {
    return true;
  }
  if (sa > sb) {
    return false;
  }
  return a.diagnostics.evaluations < b.diagnostics.evaluations;
}

BlackboxAdvancedOutcome evaluate_single_point(
    const std::function<BlackboxEvaluation(const Series&)>& blackbox,
    const Series& point,
    const MadsBlackboxAdvancedConfig& base_cfg) {
  MadsBlackboxAdvancedConfig cfg = base_cfg;
  cfg.max_iterations = 0;
  cfg.max_evaluations = 1;
  cfg.max_total_evaluations = 1;
  cfg.search_points = 0;
  cfg.surrogate_mode = SurrogateMode::Off;
  return mads_blackbox_minimizer(blackbox, point, cfg);
}

} // namespace

BlackboxAdvancedOutcome coopmads_blackbox_minimizer(
    const std::function<BlackboxEvaluation(const Series&)>& blackbox,
    const Series& initial,
    const CoopMadsConfig& config) {
  constexpr const char* op = "coopmads_blackbox_minimizer";
  require_valid_initial(initial, op);

  const CommonSetup setup = make_common_setup(initial, config.base.promote_cpu_to_float64);
  const BoundsPack bounds = prepare_bounds(config.base.lower_bounds, config.base.upper_bounds, setup, op);
  const std::int64_t n = setup.dimension;
  const std::int64_t rounds = std::max<std::int64_t>(1, config.cooperative_rounds);
  const std::int64_t subsize = std::clamp<std::int64_t>(
      (config.subproblem_size > 0)
          ? config.subproblem_size
          : std::max<std::int64_t>(1, n / std::max<std::int64_t>(1, config.subproblems_per_round > 0 ? config.subproblems_per_round : 2)),
      1,
      n);
  const std::int64_t subproblems_per_round = std::max<std::int64_t>(
      1,
      (config.subproblems_per_round > 0)
          ? config.subproblems_per_round
          : ((n + subsize - 1) / subsize));

  const DecompositionBudget budget = split_decomposition_budget(config.base, n, config.polish_fraction);
  Series incumbent = setup.flat_initial;
  if (bounds.enabled) {
    incumbent = project_flat_to_bounds(incumbent, bounds.lower, bounds.upper);
  }
  BlackboxAdvancedOutcome best = evaluate_single_point(
      blackbox,
      incumbent.reshape(std::span<const std::int64_t>(setup.original_shape.data(), setup.original_shape.size())),
      config.base);

  BlackboxAdvancedDiagnostics agg{};
  merge_advanced_diagnostics(&agg, best.diagnostics);

  std::mt19937_64 rng(config.rng_seed);
  std::vector<std::int64_t> order(static_cast<std::size_t>(n), 0);
  std::iota(order.begin(), order.end(), 0);

  for (std::int64_t r = 0; r < rounds; ++r) {
    if (config.randomized_subproblems) {
      std::shuffle(order.begin(), order.end(), rng);
    }

    for (std::int64_t b = 0; b < subproblems_per_round; ++b) {
      const std::int64_t start = (b * subsize) % n;
      const std::int64_t stop = std::min<std::int64_t>(n, start + subsize);
      std::vector<std::int64_t> idx;
      idx.reserve(static_cast<std::size_t>(stop - start));
      for (std::int64_t i = start; i < stop; ++i) {
        idx.push_back(order[static_cast<std::size_t>(i)]);
      }
      if (idx.empty()) {
        continue;
      }

      const Series anchor_full = incumbent;
      const Series sub_initial = subset_from_indices(anchor_full, idx, setup.work_cfg);
      MadsBlackboxAdvancedConfig sub_cfg = config.base;
      sub_cfg.max_iterations = std::max<std::int64_t>(1, budget.iterations / (rounds * subproblems_per_round));
      sub_cfg.max_evaluations = std::max<std::int64_t>(1, budget.evaluations / (rounds * subproblems_per_round));
      sub_cfg.max_total_evaluations = std::max<std::int64_t>(1, budget.total / (rounds * subproblems_per_round));
      if (bounds.enabled) {
        sub_cfg.lower_bounds = subset_from_indices(bounds.lower, idx, setup.work_cfg);
        sub_cfg.upper_bounds = subset_from_indices(bounds.upper, idx, setup.work_cfg);
      }

      auto sub_blackbox = [&](const Series& subx) -> BlackboxEvaluation {
        const Series sub_flat = subx.reshape({subx.numel()}).to(setup.work_cfg.platform, setup.work_cfg.dtype).contiguous();
        Series full_flat = inject_from_indices(anchor_full, idx, sub_flat, setup.work_cfg);
        if (bounds.enabled) {
          full_flat = project_flat_to_bounds(full_flat, bounds.lower, bounds.upper);
        }
        const Series full = full_flat.reshape(
            std::span<const std::int64_t>(setup.original_shape.data(), setup.original_shape.size()));
        return blackbox(full);
      };

      const BlackboxAdvancedOutcome sub_out = mads_blackbox_minimizer(sub_blackbox, sub_initial, sub_cfg);
      merge_advanced_diagnostics(&agg, sub_out.diagnostics);

      incumbent = inject_from_indices(
          anchor_full,
          idx,
          sub_out.minimizer.reshape({sub_out.minimizer.numel()}).to(setup.work_cfg.platform, setup.work_cfg.dtype),
          setup.work_cfg);
      if (bounds.enabled) {
        incumbent = project_flat_to_bounds(incumbent, bounds.lower, bounds.upper);
      }

      const BlackboxAdvancedOutcome point_out = evaluate_single_point(
          blackbox,
          incumbent.reshape(std::span<const std::int64_t>(setup.original_shape.data(), setup.original_shape.size())),
          config.base);
      merge_advanced_diagnostics(&agg, point_out.diagnostics);
      if (better_outcome(point_out, best)) {
        best = point_out;
      }
    }
  }

  if (budget.has_polish) {
    const Series start = best.minimizer;
    const BlackboxAdvancedOutcome polished = mads_blackbox_minimizer(blackbox, start, budget.polish);
    merge_advanced_diagnostics(&agg, polished.diagnostics);
    if (better_outcome(polished, best)) {
      best = polished;
    }
  }

  best.diagnostics = agg;
  best.diagnostics.last_phase = "coop_complete";
  return best;
}

BlackboxAdvancedOutcome psdmads_blackbox_minimizer(
    const std::function<BlackboxEvaluation(const Series&)>& blackbox,
    const Series& initial,
    const PsdMadsConfig& config) {
  constexpr const char* op = "psdmads_blackbox_minimizer";
  require_valid_initial(initial, op);

  const CommonSetup setup = make_common_setup(initial, config.base.promote_cpu_to_float64);
  const BoundsPack bounds = prepare_bounds(config.base.lower_bounds, config.base.upper_bounds, setup, op);
  const std::int64_t n = setup.dimension;
  const std::int64_t rounds = std::max<std::int64_t>(1, config.rounds);
  const std::int64_t min_sz = std::clamp<std::int64_t>(config.min_subproblem_size, 1, n);
  const std::int64_t max_sz = std::clamp<std::int64_t>(
      (config.max_subproblem_size > 0) ? config.max_subproblem_size : std::max<std::int64_t>(min_sz, n / 2),
      min_sz,
      n);
  const std::int64_t subproblems_per_round = std::max<std::int64_t>(1, config.subproblems_per_round);

  const DecompositionBudget budget = split_decomposition_budget(config.base, n, config.polish_fraction);
  Series incumbent = setup.flat_initial;
  if (bounds.enabled) {
    incumbent = project_flat_to_bounds(incumbent, bounds.lower, bounds.upper);
  }
  BlackboxAdvancedOutcome best = evaluate_single_point(
      blackbox,
      incumbent.reshape(std::span<const std::int64_t>(setup.original_shape.data(), setup.original_shape.size())),
      config.base);
  BlackboxAdvancedDiagnostics agg{};
  merge_advanced_diagnostics(&agg, best.diagnostics);

  std::mt19937_64 rng(config.rng_seed);
  std::uniform_int_distribution<std::int64_t> size_dist(min_sz, max_sz);
  std::vector<std::int64_t> all_idx(static_cast<std::size_t>(n), 0);
  std::iota(all_idx.begin(), all_idx.end(), 0);

  for (std::int64_t r = 0; r < rounds; ++r) {
    for (std::int64_t s = 0; s < subproblems_per_round; ++s) {
      const std::int64_t subsize = size_dist(rng);
      if (config.randomized_subproblems) {
        std::shuffle(all_idx.begin(), all_idx.end(), rng);
      }
      std::vector<std::int64_t> idx(
          all_idx.begin(),
          all_idx.begin() + static_cast<std::ptrdiff_t>(subsize));

      const Series anchor_full = incumbent;
      const Series sub_initial = subset_from_indices(anchor_full, idx, setup.work_cfg);
      MadsBlackboxAdvancedConfig sub_cfg = config.base;
      sub_cfg.max_iterations = std::max<std::int64_t>(1, budget.iterations / (rounds * subproblems_per_round));
      sub_cfg.max_evaluations = std::max<std::int64_t>(1, budget.evaluations / (rounds * subproblems_per_round));
      sub_cfg.max_total_evaluations = std::max<std::int64_t>(1, budget.total / (rounds * subproblems_per_round));
      if (bounds.enabled) {
        sub_cfg.lower_bounds = subset_from_indices(bounds.lower, idx, setup.work_cfg);
        sub_cfg.upper_bounds = subset_from_indices(bounds.upper, idx, setup.work_cfg);
      }

      auto sub_blackbox = [&](const Series& subx) -> BlackboxEvaluation {
        const Series sub_flat = subx.reshape({subx.numel()}).to(setup.work_cfg.platform, setup.work_cfg.dtype).contiguous();
        Series full_flat = inject_from_indices(anchor_full, idx, sub_flat, setup.work_cfg);
        if (bounds.enabled) {
          full_flat = project_flat_to_bounds(full_flat, bounds.lower, bounds.upper);
        }
        const Series full = full_flat.reshape(
            std::span<const std::int64_t>(setup.original_shape.data(), setup.original_shape.size()));
        return blackbox(full);
      };

      const BlackboxAdvancedOutcome sub_out = mads_blackbox_minimizer(sub_blackbox, sub_initial, sub_cfg);
      merge_advanced_diagnostics(&agg, sub_out.diagnostics);

      incumbent = inject_from_indices(
          anchor_full,
          idx,
          sub_out.minimizer.reshape({sub_out.minimizer.numel()}).to(setup.work_cfg.platform, setup.work_cfg.dtype),
          setup.work_cfg);
      if (bounds.enabled) {
        incumbent = project_flat_to_bounds(incumbent, bounds.lower, bounds.upper);
      }

      const BlackboxAdvancedOutcome point_out = evaluate_single_point(
          blackbox,
          incumbent.reshape(std::span<const std::int64_t>(setup.original_shape.data(), setup.original_shape.size())),
          config.base);
      merge_advanced_diagnostics(&agg, point_out.diagnostics);
      if (better_outcome(point_out, best)) {
        best = point_out;
      }
    }
  }

  if (budget.has_polish) {
    const Series start = best.minimizer;
    const BlackboxAdvancedOutcome polished = mads_blackbox_minimizer(blackbox, start, budget.polish);
    merge_advanced_diagnostics(&agg, polished.diagnostics);
    if (better_outcome(polished, best)) {
      best = polished;
    }
  }

  best.diagnostics = agg;
  best.diagnostics.last_phase = "psd_complete";
  return best;
}

BlackboxAdvancedOutcome discomads_blackbox_minimizer(
    const std::function<BlackboxEvaluation(const Series&)>& blackbox,
    const Series& initial,
    const DiscoMadsConfig& config) {
  constexpr const char* op = "discomads_blackbox_minimizer";
  require_valid_initial(initial, op);

  const CommonSetup setup = make_common_setup(initial, config.base.promote_cpu_to_float64);
  const BoundsPack bounds = prepare_bounds(config.base.lower_bounds, config.base.upper_bounds, setup, op);
  const std::int64_t n = setup.dimension;

  // A failed evaluation stays a failure for the search: the value of a different point is never reported as the value of the
  // point asked for, which would corrupt the mesh logic and the reported minimum. What Disco does with a failure is look
  // around it: up to `rescue_trials` nearby points (random offsets of radius `rescue_radius`, shrinking by
  // `rescue_radius_decay`) are evaluated, and any valid, feasible, single-objective result is remembered as a candidate answer.
  // At the end the best remembered candidate replaces the search result if it is better.
  std::uint64_t rescue_counter = 0ULL;
  bool have_rescue = false;
  double rescue_value = std::numeric_limits<double>::infinity();
  Series rescue_point = setup.flat_initial;
  Series rescue_objectives = Series::zeros({1}, ScalarType::Float64);
  std::int64_t rescue_calls = 0;
  std::int64_t observed_calls = 0;
  std::int64_t observed_failures = 0;

  auto valid_eval = [&](const BlackboxEvaluation& ev) {
    if (!ev.evaluation_ok || !ev.objectives.defined() || ev.objectives.numel() < 1) {
      return false;
    }
    for (double v : ev.objectives.to(ScalarType::Float64).host<double>().array().data) {
      if (!std::isfinite(v)) {
        return false;
      }
    }
    if (ev.constraints.defined()) {
      for (double v : ev.constraints.to(ScalarType::Float64).host<double>().array().data) {
        if (!std::isfinite(v) || v > config.base.constraint_tolerance) {
          return false;
        }
      }
    }
    return true;
  };

  auto wrapped = [&](const Series& x) -> BlackboxEvaluation {
    const BlackboxEvaluation base_eval = blackbox(x);
    ++observed_calls;
    if (valid_eval(base_eval)) {
      return base_eval;
    }
    ++observed_failures;

    // Looking around failures costs extra calls, and plain MADS copes with an occasional failure on its own, so the rescue
    // only starts once the observed failure rate has reached `rescue_failure_rate` (after a short warm-up of 20 calls).
    const bool rescue_active = observed_calls >= 20 &&
        static_cast<double>(observed_failures) >= config.rescue_failure_rate * static_cast<double>(observed_calls);
    const Series x_flat = x.reshape({x.numel()}).to(setup.work_cfg.platform, setup.work_cfg.dtype).contiguous();
    const std::int64_t trials = rescue_active ? std::max<std::int64_t>(0, config.rescue_trials) : 0;
    for (std::int64_t t = 0; t < trials; ++t) {
      const double radius = config.rescue_radius * std::pow(config.rescue_radius_decay, static_cast<double>(t));
      Series noise = pluss::series::randn({n}, setup.work_cfg, config.rng_seed + rescue_counter++)
                         .mul(radius);
      Series cand = x_flat.add(noise);
      if (bounds.enabled) {
        cand = project_flat_to_bounds(cand, bounds.lower, bounds.upper);
      }
      const Series shaped = cand.reshape(
          std::span<const std::int64_t>(setup.original_shape.data(), setup.original_shape.size()));
      const BlackboxEvaluation rescue = blackbox(shaped);
      ++rescue_calls;
      if (valid_eval(rescue) && rescue.objectives.numel() == 1) {
        const double value = rescue.objectives.to(ScalarType::Float64).item<double>();
        if (value < rescue_value) {
          have_rescue = true;
          rescue_value = value;
          rescue_point = cand;
          rescue_objectives = rescue.objectives.to(ScalarType::Float64).reshape({1});
        }
      }
    }
    BlackboxEvaluation failed = base_eval;
    failed.evaluation_ok = false;
    if (!failed.objectives.defined()) {
      failed.objectives = Series::zeros({1}, ScalarType::Float64);
    }
    return failed;
  };

  BlackboxAdvancedOutcome out = mads_blackbox_minimizer(wrapped, initial, config.base);
  if (have_rescue && rescue_value < out.scalarized_minimum) {
    out.minimizer = rescue_point
        .reshape(std::span<const std::int64_t>(setup.original_shape.data(), setup.original_shape.size()))
        .to(setup.flat_initial.platform(), setup.flat_initial.scalar_type());
    out.objective_vector = rescue_objectives;
    out.scalarized_minimum = rescue_value;
  }
  out.diagnostics.total_evaluations += rescue_calls;
  out.diagnostics.last_phase += "|disco";
  return out;
}

BlackboxAdvancedOutcome vnsmads_blackbox_minimizer(
    const std::function<BlackboxEvaluation(const Series&)>& blackbox,
    const Series& initial,
    const VnsMadsConfig& config) {
  constexpr const char* op = "vnsmads_blackbox_minimizer";
  require_valid_initial(initial, op);

  const CommonSetup setup = make_common_setup(initial, config.base.promote_cpu_to_float64);
  const BoundsPack bounds = prepare_bounds(config.base.lower_bounds, config.base.upper_bounds, setup, op);
  const std::int64_t n = setup.dimension;
  const std::int64_t stages = std::max<std::int64_t>(1, config.neighborhoods);

  auto stage_cfg = [&](std::int64_t stage) {
    MadsBlackboxAdvancedConfig cfg = config.base;
    if (cfg.max_iterations > 0) {
      cfg.max_iterations = std::max<std::int64_t>(1, cfg.max_iterations / stages);
    }
    if (cfg.max_evaluations > 0) {
      cfg.max_evaluations = std::max<std::int64_t>(1, cfg.max_evaluations / stages);
    }
    if (cfg.max_total_evaluations > 0) {
      cfg.max_total_evaluations = std::max<std::int64_t>(1, cfg.max_total_evaluations / stages);
    }
    cfg.rng_seed += static_cast<std::uint64_t>(stage);
    return cfg;
  };

  BlackboxAdvancedOutcome best = mads_blackbox_minimizer(blackbox, initial, stage_cfg(0));
  BlackboxAdvancedDiagnostics agg{};
  merge_advanced_diagnostics(&agg, best.diagnostics);

  double kick = std::max(1e-8, config.initial_kick_scale);
  std::uint64_t seed = config.rng_seed;

  for (std::int64_t stage = 1; stage < stages; ++stage) {
    kick *= std::max(1.0, config.kick_growth);
    Series base_flat = best.minimizer.reshape({best.minimizer.numel()})
                           .to(setup.work_cfg.platform, setup.work_cfg.dtype)
                           .contiguous();
    Series perturb = pluss::series::randn({n}, setup.work_cfg, seed++).mul(kick);
    Series start_flat = base_flat.add(perturb);
    if (bounds.enabled) {
      start_flat = project_flat_to_bounds(start_flat, bounds.lower, bounds.upper);
    }
    const Series start = start_flat.reshape(
        std::span<const std::int64_t>(setup.original_shape.data(), setup.original_shape.size()));
    const BlackboxAdvancedOutcome out = mads_blackbox_minimizer(blackbox, start, stage_cfg(stage));
    merge_advanced_diagnostics(&agg, out.diagnostics);
    if (better_outcome(out, best)) {
      best = out;
    }
  }

  best.diagnostics = agg;
  best.diagnostics.last_phase = "vns_complete";
  return best;
}

} // namespace pluss::subsp::math::optimization
