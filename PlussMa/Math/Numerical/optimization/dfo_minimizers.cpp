#include "dfo_minimizers.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#if __has_include(<fmt/format.h>)
#include <fmt/format.h>
#endif
#include <iostream>
#include <limits>
#include <random>
#include <span>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace pluss::subsp::math::optimization {
namespace {

struct BoundsPack {
  bool enabled{false};
  Series lower{pluss::series::undefined()};
  Series upper{pluss::series::undefined()};
};

constexpr double kBoxEps = std::numeric_limits<double>::epsilon();

struct CommonSetup {
  std::vector<std::int64_t> original_shape{};
  std::int64_t dimension{0};
  Config work_cfg{};
  Config idx_cfg{};
  Config bool_cfg{};
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

struct HostBoundsPack {
  bool enabled{false};
  std::vector<double> lower{};
  std::vector<double> upper{};
};

double scalar_as_f64(const Series& x, const char* op) {
  if (!x.defined() || x.numel() != 1) {
    ::pluss::throw_error(std::string(op) + ": expected scalar Series");
  }
  return x.to(ScalarType::Float64).item<double>();
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

std::string iter_line(
    std::int64_t iter,
    std::int64_t evals,
    double best,
    std::string_view step) {
#if __has_include(<fmt/format.h>)
  return fmt::format("iter={} evals={} f={:.6g} step={}", iter, evals, best, step);
#else
  std::ostringstream oss;
  oss.setf(std::ios::fmtflags(0), std::ios::floatfield);
  oss << "iter=" << iter << " evals=" << evals << " f=" << best << " step=" << step;
  return oss.str();
#endif
}

void maybe_print_iteration(
    PopulationSolverDisplay display,
    std::int64_t iter,
    std::int64_t evals,
    double best,
    std::string_view step) {
  if (display != PopulationSolverDisplay::Iter) {
    return;
  }
  std::cout << iter_line(iter, evals, best, step) << "\n";
}

CommonSetup make_common_setup(
    const Series& initial,
    bool promote_cpu_to_float64) {
  CommonSetup out;
  out.original_shape.assign(initial.sizes().begin(), initial.sizes().end());
  out.dimension = initial.numel();

  ScalarType work_dtype = initial.scalar_type();
  if (promote_cpu_to_float64 && initial.platform() == pluss::pCPU &&
      work_dtype != ScalarType::Float64) {
    work_dtype = ScalarType::Float64;
  }

  out.work_cfg = Config{}.with_dtype(work_dtype).with_platform(initial.platform());
  out.idx_cfg = Config{}.with_dtype(ScalarType::Int64).with_platform(initial.platform());
  out.bool_cfg = Config{}.with_dtype(ScalarType::Bool).with_platform(initial.platform());

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

  const Series flat_lower = lower.reshape({setup.dimension}).to(setup.work_cfg.platform, setup.work_cfg.dtype).contiguous();
  const Series flat_upper = upper.reshape({setup.dimension}).to(setup.work_cfg.platform, setup.work_cfg.dtype).contiguous();

  if (flat_lower.numel() != setup.dimension || flat_upper.numel() != setup.dimension) {
    ::pluss::throw_error(std::string(op) + ": bounds must have same number of elements as initial point");
  }

  const Series ok = flat_lower.le(flat_upper).all();
  if (!ok.item<bool>()) {
    ::pluss::throw_error(std::string(op) + ": expected lower_bounds <= upper_bounds elementwise");
  }

  bounds.enabled = true;
  bounds.lower = flat_lower;
  bounds.upper = flat_upper;
  return bounds;
}

Series transform_box_matrix(
    const Series& x,
    const BoundsPack& bounds,
    std::int64_t n_pop,
    std::int64_t n_dim) {
  if (!bounds.enabled) {
    return x;
  }
  const Series lower = bounds.lower.reshape({1, n_dim}).expand(std::vector<std::int64_t>{n_pop, n_dim});
  const Series upper = bounds.upper.reshape({1, n_dim}).expand(std::vector<std::int64_t>{n_pop, n_dim});
  return x.sub(lower).add(kBoxEps).log()
      .sub(upper.sub(x).add(kBoxEps).log());
}

Series inv_transform_box_vector(const Series& z, const BoundsPack& bounds) {
  if (!bounds.enabled) {
    return z;
  }
  const Series lower = bounds.lower;
  const Series upper = bounds.upper;
  const Series span = upper.sub(lower).sub(2.0 * kBoxEps);
  return lower.add(kBoxEps).add(span.mul(z.sigmoid()));
}

HostBoundsPack to_host_bounds(
    const BoundsPack& bounds,
    std::int64_t n_dim) {
  HostBoundsPack out;
  out.enabled = bounds.enabled;
  if (!out.enabled) {
    return out;
  }
  out.lower = bounds.lower.to(ScalarType::Float64).to_vec<double>();
  out.upper = bounds.upper.to(ScalarType::Float64).to_vec<double>();
  if (static_cast<std::int64_t>(out.lower.size()) != n_dim ||
      static_cast<std::int64_t>(out.upper.size()) != n_dim) {
    ::pluss::throw_error("dfo_minimizers: invalid bounds host conversion");
  }
  return out;
}

inline double inv_transform_scalar(double z, double lb, double ub) {
  const double z_clamped = std::max(-700.0, std::min(700.0, z));
  const double sigma = 1.0 / (1.0 + std::exp(-z_clamped));
  return lb + kBoxEps + (ub - lb - 2.0 * kBoxEps) * sigma;
}

inline double transform_scalar(double x, double lb, double ub) {
  const double xc = std::max(lb + kBoxEps, std::min(ub - kBoxEps, x));
  return std::log(xc - lb + kBoxEps) - std::log(ub - xc + kBoxEps);
}

std::vector<double> inv_transform_host_vec(
    const std::vector<double>& z,
    const HostBoundsPack& bounds) {
  if (!bounds.enabled) {
    return z;
  }
  std::vector<double> x(z.size());
  for (std::size_t j = 0; j < z.size(); ++j) {
    x[j] = inv_transform_scalar(z[j], bounds.lower[j], bounds.upper[j]);
  }
  return x;
}

std::vector<double> transform_host_vec(
    const std::vector<double>& x,
    const HostBoundsPack& bounds) {
  if (!bounds.enabled) {
    return x;
  }
  std::vector<double> z(x.size());
  for (std::size_t j = 0; j < x.size(); ++j) {
    z[j] = transform_scalar(x[j], bounds.lower[j], bounds.upper[j]);
  }
  return z;
}

Series host_vector_to_series(
    const std::vector<double>& x,
    const Config& cfg) {
  return pluss::make_series(x, {static_cast<std::int64_t>(x.size())}, cfg)
      .to(cfg.platform, cfg.dtype);
}

double eval_host_candidate(
    const std::function<Series(const Series&)>& objective,
    const std::vector<double>& candidate,
    const HostBoundsPack& bounds,
    const CommonSetup& setup,
    const char* op) {
  const std::vector<double> x_eval = inv_transform_host_vec(candidate, bounds);
  const Series flat = host_vector_to_series(x_eval, setup.work_cfg);
  const Series shaped = flat.reshape(
      std::span<const std::int64_t>(setup.original_shape.data(), setup.original_shape.size()));
  const Series out = objective(shaped);
  if (!out.defined() || out.numel() != 1 || out.dtype().is_complex()) {
    ::pluss::throw_error(std::string(op) + ": objective must return real scalar Series");
  }
  const double val = scalar_as_f64(out, op);
  return std::isfinite(val) ? val : std::numeric_limits<double>::infinity();
}

inline double rand_u01(std::mt19937_64* rng) {
  return std::generate_canonical<double, 53>(*rng);
}

inline std::int64_t rand_index_inclusive(
    std::int64_t lo,
    std::int64_t hi,
    std::mt19937_64* rng) {
  std::uniform_int_distribution<std::int64_t> dist(lo, hi);
  return dist(*rng);
}

std::int64_t argmin_host(const std::vector<double>& x) {
  return static_cast<std::int64_t>(
      std::distance(x.begin(), std::min_element(x.begin(), x.end())));
}

std::vector<std::vector<double>> sample_population_host(
    const CommonSetup& setup,
    const HostBoundsPack& bounds,
    std::int64_t n_pop,
    double initial_radius,
    std::mt19937_64* rng) {
  const std::int64_t n_dim = setup.dimension;
  std::vector<double> center =
      setup.flat_initial.to(ScalarType::Float64).to_vec<double>();
  if (static_cast<std::int64_t>(center.size()) != n_dim) {
    ::pluss::throw_error("dfo_minimizers: invalid host initial conversion");
  }

  std::vector<double> lb(n_dim), ub(n_dim);
  for (std::int64_t j = 0; j < n_dim; ++j) {
    if (bounds.enabled) {
      center[static_cast<std::size_t>(j)] = std::max(
          bounds.lower[static_cast<std::size_t>(j)],
          std::min(center[static_cast<std::size_t>(j)],
                   bounds.upper[static_cast<std::size_t>(j)]));
    }
    // An infinite radius (the default) means: the whole box where both bounds are finite, otherwise start +- 0.5.
    const bool whole_box = !std::isfinite(initial_radius) && bounds.enabled &&
        std::isfinite(bounds.lower[static_cast<std::size_t>(j)]) && std::isfinite(bounds.upper[static_cast<std::size_t>(j)]);
    const double radius = std::isfinite(initial_radius) ? initial_radius : (whole_box ? 1e300 : 0.5);
    lb[static_cast<std::size_t>(j)] = center[static_cast<std::size_t>(j)] - radius;
    ub[static_cast<std::size_t>(j)] = center[static_cast<std::size_t>(j)] + radius;
    if (bounds.enabled) {
      lb[static_cast<std::size_t>(j)] = std::max(
          lb[static_cast<std::size_t>(j)], bounds.lower[static_cast<std::size_t>(j)]);
      ub[static_cast<std::size_t>(j)] = std::min(
          ub[static_cast<std::size_t>(j)], bounds.upper[static_cast<std::size_t>(j)]);
    }
  }

  std::vector<std::vector<double>> pop(
      static_cast<std::size_t>(n_pop), std::vector<double>(static_cast<std::size_t>(n_dim), 0.0));
  for (std::int64_t i = 0; i < n_pop; ++i) {
    auto& row = pop[static_cast<std::size_t>(i)];
    for (std::int64_t j = 0; j < n_dim; ++j) {
      const double u = rand_u01(rng);
      row[static_cast<std::size_t>(j)] =
          lb[static_cast<std::size_t>(j)] +
          u * (ub[static_cast<std::size_t>(j)] - lb[static_cast<std::size_t>(j)]);
    }
    if (i == 0) {
      row = center;  // the starting point itself is always a member, so the population never starts without it
    }
    if (bounds.enabled) {
      row = transform_host_vec(row, bounds);
    }
  }

  return pop;
}

Series eval_candidate(
    const std::function<Series(const Series&)>& objective,
    const Series& candidate_flat,
    const BoundsPack& bounds,
    const std::vector<std::int64_t>& original_shape,
    const Config& work_cfg,
    const char* op) {
  const Series candidate_eval = inv_transform_box_vector(candidate_flat, bounds);
  const Series x = candidate_eval.reshape(std::span<const std::int64_t>(original_shape.data(), original_shape.size()));
  const Series out = objective(x);

  if (!out.defined() || out.numel() != 1) {
    ::pluss::throw_error(std::string(op) + ": objective must return scalar Series");
  }
  if (out.dtype().is_complex()) {
    ::pluss::throw_error(std::string(op) + ": objective must return real scalar Series");
  }

  const double v = scalar_as_f64(out, op);
  if (!std::isfinite(v)) {
    return pluss::series::full({1}, std::numeric_limits<double>::infinity(), work_cfg);
  }

  return out.to(work_cfg.platform, work_cfg.dtype).reshape({1});
}

Series eval_population(
    const std::function<Series(const Series&)>& objective,
    const Series& population,
    const BoundsPack& bounds,
    const std::vector<std::int64_t>& original_shape,
    const Config& work_cfg,
    const Config& idx_cfg,
    std::int64_t* eval_counter,
    const char* op) {
  const std::int64_t n_pop = population.sizes()[0];
  Series fit = pluss::series::zeros({n_pop}, work_cfg);

  for (std::int64_t i = 0; i < n_pop; ++i) {
    const Series row = population.select(0, i);
    const Series val = eval_candidate(objective, row, bounds, original_shape, work_cfg, op);
    const Series idx = pluss::series::arange(i, i + 1, idx_cfg);
    fit = fit.put(idx, val.reshape({1}), false);
  }

  *eval_counter += n_pop;
  return fit;
}

std::int64_t argmin_index(const Series& x) {
  return x.argmin(0, false).to(ScalarType::Int64).item<std::int64_t>();
}

Series row_by_index(const Series& m, std::int64_t row) {
  return m.select(0, row);
}

Series concat_1d(const Series& a, const Series& b) {
  return a.unite(std::vector<Series>{b}, 0);
}

Series roll_rows(
    const Series& m,
    std::int64_t shift,
    const Config& idx_cfg) {
  const std::int64_t n = m.sizes()[0];
  if (n == 0) {
    return m;
  }
  const std::int64_t k = ((shift % n) + n) % n;
  if (k == 0) {
    return m;
  }

  const Series head = pluss::series::arange(n - k, n, idx_cfg);
  const Series tail = pluss::series::arange(0, n - k, idx_cfg);
  const Series idx = concat_1d(head, tail);
  return m.index_select(0, idx);
}

Series random_permutation(
    std::int64_t n,
    const Config& work_cfg,
    RngState* rng) {
  return pluss::series::rand({n}, work_cfg, rng->next()).argsort(0, false).to(work_cfg.platform, ScalarType::Int64);
}

Series inverse_permutation(const Series& perm, const Config& idx_cfg) {
  const std::int64_t n = perm.numel();
  const Series idx = pluss::series::arange(0, n, idx_cfg);
  const Series base = pluss::series::zeros({n}, idx_cfg);
  return base.scatter(0, perm, idx);
}

Series crossover_mask(
    std::int64_t n_pop,
    std::int64_t n_dim,
    double crossover_rate,
    const Config& work_cfg,
    const Config& idx_cfg,
    const Config& bool_cfg,
    RngState* rng) {
  Series mask = pluss::series::rand({n_pop, n_dim}, work_cfg, rng->next()).lt(crossover_rate);

  const Series jrand = pluss::series::randint(0, n_dim, {n_pop}, idx_cfg, rng->next()).reshape({n_pop, 1});
  const Series one_col_true = pluss::series::ones({n_pop, 1}, bool_cfg);
  mask = mask.scatter(1, jrand, one_col_true);
  return mask;
}

Series crossover_mask_per_row_rate(
    std::int64_t n_pop,
    std::int64_t n_dim,
    const Series& crossover_rate_vec,
    const Config& work_cfg,
    const Config& idx_cfg,
    const Config& bool_cfg,
    RngState* rng) {
  const Series cr = crossover_rate_vec.reshape({n_pop, 1}).expand(std::vector<std::int64_t>{n_pop, n_dim});
  Series mask = pluss::series::rand({n_pop, n_dim}, work_cfg, rng->next()).lt(cr);

  const Series jrand = pluss::series::randint(0, n_dim, {n_pop}, idx_cfg, rng->next()).reshape({n_pop, 1});
  const Series one_col_true = pluss::series::ones({n_pop, 1}, bool_cfg);
  mask = mask.scatter(1, jrand, one_col_true);
  return mask;
}

// Spread (largest minus smallest finite value) of the objective values of the current population.
double population_spread(const std::vector<double>& values) {
  double lo = std::numeric_limits<double>::infinity();
  double hi = -std::numeric_limits<double>::infinity();
  for (const double v : values) {
    if (!std::isfinite(v)) continue;
    lo = std::min(lo, v);
    hi = std::max(hi, v);
  }
  return hi >= lo ? hi - lo : 0.0;
}

double population_spread(const Series& values) {
  const std::vector<double> host = values.to(pluss::pCPU, pluss::ScalarType::Float64).contiguous().to_vec<double>();
  return population_spread(host);
}

// A population method has converged when its best value has stopped improving AND the population itself has collapsed onto
// it. A plateau of the best value alone is not convergence: a swarm or a differential-evolution population can sit on a
// plateau for a whole check window while still exploring, and calling that "converged" returns a poor point.
bool converged_by_relative_improvement(
    std::int64_t iter,
    std::int64_t check_every,
    double current_best,
    double* check_best,
    double tol,
    double spread) {
  if (check_every < 1 || iter % check_every != 0) {
    return false;
  }

  const double rel_change = std::abs(current_best - *check_best) /
      (std::numeric_limits<double>::epsilon() + std::abs(current_best));

  if (current_best < *check_best) {
    *check_best = current_best;
  }

  const bool collapsed = spread <= std::sqrt(std::max(tol, 0.0)) * (1.0 + std::abs(current_best));
  return rel_change <= tol && collapsed;
}

Series sample_population(
    const Series& flat_initial,
    std::int64_t n_pop,
    double initial_radius,
    const BoundsPack& bounds,
    const Config& work_cfg,
    RngState* rng) {
  const std::int64_t n_dim = flat_initial.numel();
  Series center = flat_initial;
  if (bounds.enabled) {
    center = center.maximum(bounds.lower).minimum(bounds.upper);
  }

  // An infinite radius (the default) means: the whole box where both bounds are finite, otherwise start +- 0.5.
  Series lb = center.sub(std::isfinite(initial_radius) ? initial_radius : 0.5);
  Series ub = center.add(std::isfinite(initial_radius) ? initial_radius : 0.5);

  if (bounds.enabled) {
    if (!std::isfinite(initial_radius)) {
      const Series box_is_finite = bounds.lower.abs().lt(1e100).logical_and(bounds.upper.abs().lt(1e100));
      lb = box_is_finite.where(bounds.lower, lb);
      ub = box_is_finite.where(bounds.upper, ub);
    }
    lb = lb.maximum(bounds.lower);
    ub = ub.minimum(bounds.upper);
  }

  const Series width = ub.sub(lb);
  const Series rand_u = pluss::series::rand({n_pop, n_dim}, work_cfg, rng->next());

  Series sampled = lb.reshape({1, n_dim}).expand(std::vector<std::int64_t>{n_pop, n_dim})
      .add(rand_u.mul(width.reshape({1, n_dim}).expand(std::vector<std::int64_t>{n_pop, n_dim})));
  if (n_pop > 1) {
    // the starting point itself is always a member, so the population never starts without it
    sampled = center.reshape({1, n_dim}).unite({sampled.slice(0, 1, n_pop, 1)}, 0);
  }
  if (bounds.enabled) {
    sampled = transform_box_matrix(sampled, bounds, n_pop, n_dim);
  }
  return sampled;
}

PopulationSolverOutcome build_outcome(
    const Series& flat_best,
    double best_val,
    const CommonSetup& setup,
    const BoundsPack& bounds,
    std::int64_t iterations,
    std::int64_t evaluations,
    std::string last_step,
    PopulationSolverStatus status) {
  const Series best_eval = inv_transform_box_vector(flat_best, bounds);
  PopulationSolverOutcome out;
  out.minimizer = best_eval
      .reshape(std::span<const std::int64_t>(setup.original_shape.data(), setup.original_shape.size()))
      .to(setup.flat_initial.platform(), setup.flat_initial.scalar_type());
  out.minimum = best_val;
  out.status = status;
  out.diagnostics.iterations = iterations;
  out.diagnostics.evaluations = evaluations;
  out.diagnostics.last_step = std::move(last_step);
  return out;
}

} // namespace

PopulationSolverOutcome differential_evolution_minimizer(
    const std::function<Series(const Series&)>& objective,
    const Series& initial,
    const DifferentialEvolutionConfig& config) {
  constexpr const char* op = "differential_evolution_minimizer";
  require_valid_initial(initial, op);
  if (config.population_size < 5) {
    ::pluss::throw_error(std::string(op) + ": population_size must be >= 5");
  }

  const CommonSetup setup = make_common_setup(initial, config.promote_cpu_to_float64);
  const BoundsPack bounds = prepare_bounds(config.lower_bounds, config.upper_bounds, setup, op);

  const std::int64_t n_pop = config.population_size;
  const std::int64_t n_dim = setup.dimension;
  const std::int64_t max_generations = (config.max_generations > 0) ? config.max_generations : 800;

  std::int64_t max_evaluations = config.max_evaluations;
  const bool eval_budget_derived = max_evaluations <= 0;  // a generation limit then ends the run, not the evaluation budget
  if (max_evaluations <= 0) {
    max_evaluations = n_pop * (max_generations + 1);
  }

  if (config.execution_mode == PopulationSolverExecutionMode::OptimMasterCompat) {
    const HostBoundsPack host_bounds = to_host_bounds(bounds, n_dim);
    std::mt19937_64 rng(config.rng_seed);

    std::vector<std::vector<double>> pop =
        sample_population_host(setup, host_bounds, n_pop, config.initial_radius, &rng);
    std::vector<double> fit(static_cast<std::size_t>(n_pop), std::numeric_limits<double>::infinity());
    std::int64_t evaluations = 0;

    for (std::int64_t i = 0; i < n_pop; ++i) {
      fit[static_cast<std::size_t>(i)] =
          eval_host_candidate(objective, pop[static_cast<std::size_t>(i)], host_bounds, setup, op);
      ++evaluations;
      if (evaluations >= max_evaluations) {
        break;
      }
    }

    std::int64_t best_idx = argmin_host(fit);
    std::vector<double> best = pop[static_cast<std::size_t>(best_idx)];
    double best_val = fit[static_cast<std::size_t>(best_idx)];
    double check_best = best_val;

    std::int64_t iterations = 0;
    std::string last_step = "initial_population";
    maybe_print_iteration(config.display, iterations, evaluations, best_val, last_step);

    bool converged = false;
    const bool use_best_mutation = config.best_mix >= 0.5;

    while (iterations < max_generations && evaluations < max_evaluations) {
      ++iterations;
      const auto old_pop = pop;
      const auto old_fit = fit;

      for (std::int64_t i = 0; i < n_pop && evaluations < max_evaluations; ++i) {
        std::int64_t c1 = 0;
        std::int64_t c2 = 0;
        std::int64_t c3 = 0;
        do {
          c1 = rand_index_inclusive(0, n_pop - 1, &rng);
        } while (c1 == i);
        do {
          c2 = rand_index_inclusive(0, n_pop - 1, &rng);
        } while (c2 == i || c2 == c1);
        do {
          c3 = rand_index_inclusive(0, n_pop - 1, &rng);
        } while (c3 == i || c3 == c1 || c3 == c2);

        const std::int64_t rand_ind = rand_index_inclusive(0, n_dim - 1, &rng);
        std::vector<double> prop(static_cast<std::size_t>(n_dim), 0.0);

        for (std::int64_t k = 0; k < n_dim; ++k) {
          if (rand_u01(&rng) < config.crossover_rate || k == rand_ind) {
            if (use_best_mutation) {
              prop[static_cast<std::size_t>(k)] =
                  best[static_cast<std::size_t>(k)] +
                  config.mutation_factor *
                      (old_pop[static_cast<std::size_t>(c1)][static_cast<std::size_t>(k)] -
                       old_pop[static_cast<std::size_t>(c2)][static_cast<std::size_t>(k)]);
            } else {
              prop[static_cast<std::size_t>(k)] =
                  old_pop[static_cast<std::size_t>(c3)][static_cast<std::size_t>(k)] +
                  config.mutation_factor *
                      (old_pop[static_cast<std::size_t>(c1)][static_cast<std::size_t>(k)] -
                       old_pop[static_cast<std::size_t>(c2)][static_cast<std::size_t>(k)]);
            }
          } else {
            prop[static_cast<std::size_t>(k)] =
                old_pop[static_cast<std::size_t>(i)][static_cast<std::size_t>(k)];
          }
        }

        const double prop_fit = eval_host_candidate(objective, prop, host_bounds, setup, op);
        ++evaluations;

        if (prop_fit <= old_fit[static_cast<std::size_t>(i)]) {
          pop[static_cast<std::size_t>(i)] = std::move(prop);
          fit[static_cast<std::size_t>(i)] = prop_fit;
        } else {
          pop[static_cast<std::size_t>(i)] = old_pop[static_cast<std::size_t>(i)];
          fit[static_cast<std::size_t>(i)] = old_fit[static_cast<std::size_t>(i)];
        }
      }

      best_idx = argmin_host(fit);
      const double current_best = fit[static_cast<std::size_t>(best_idx)];
      if (current_best < best_val) {
        best_val = current_best;
        best = pop[static_cast<std::size_t>(best_idx)];
      }

      last_step = "generation";
      maybe_print_iteration(config.display, iterations, evaluations, best_val, last_step);

      if (converged_by_relative_improvement(
              iterations, config.check_every, best_val, &check_best,
              config.relative_improvement_tolerance,
              population_spread(fit))) {
        converged = true;
        break;
      }
    }

    const PopulationSolverStatus status = converged
        ? PopulationSolverStatus::Converged
        : ((evaluations >= max_evaluations && !(eval_budget_derived && iterations >= max_generations))
               ? PopulationSolverStatus::ReachedEvaluationBudget
               : PopulationSolverStatus::ReachedIterationBudget);

    return build_outcome(
        host_vector_to_series(best, setup.work_cfg),
        best_val,
        setup,
        bounds,
        iterations,
        evaluations,
        last_step,
        status);
  }

  RngState rng{config.rng_seed, 0ULL};

  Series pop = sample_population(setup.flat_initial, n_pop, config.initial_radius, bounds, setup.work_cfg, &rng);
  std::int64_t evaluations = 0;
  Series fit = eval_population(objective, pop, bounds, setup.original_shape, setup.work_cfg, setup.idx_cfg, &evaluations, op);

  std::int64_t best_idx = argmin_index(fit);
  Series best = row_by_index(pop, best_idx);
  double best_val = scalar_as_f64(row_by_index(fit.reshape({n_pop, 1}), best_idx), op);
  double check_best = best_val;

  std::int64_t iterations = 0;
  std::string last_step = "initial_population";
  maybe_print_iteration(config.display, iterations, evaluations, best_val, last_step);

  bool converged = false;

  while (iterations < max_generations && evaluations < max_evaluations) {
    ++iterations;

    const Series perm = random_permutation(n_pop, setup.work_cfg, &rng);
    const Series inv_perm = inverse_permutation(perm, setup.idx_cfg);

    const Series base = pop.index_select(0, perm);
    const Series base_fit = fit.index_select(0, perm);

    const Series r1 = roll_rows(base, 1, setup.idx_cfg);
    const Series r2 = roll_rows(base, 2, setup.idx_cfg);
    const Series r3 = roll_rows(base, 3, setup.idx_cfg);

    const Series f = pluss::series::full({n_pop, n_dim}, config.mutation_factor, setup.work_cfg);
    const Series mutant_rand = r1.add(f.mul(r2.sub(r3)));

    const Series best_expand = best.reshape({1, n_dim}).expand(std::vector<std::int64_t>{n_pop, n_dim});
    const Series mutant_best = best_expand.add(f.mul(r1.sub(r2)));
    const Series mutant = mutant_rand.mul(1.0 - config.best_mix).add(mutant_best.mul(config.best_mix));

    const Series cross = crossover_mask(
        n_pop, n_dim, config.crossover_rate,
        setup.work_cfg, setup.idx_cfg, setup.bool_cfg, &rng);

    Series trial = cross.where(mutant, base);
    Series trial_fit = eval_population(objective, trial, bounds, setup.original_shape, setup.work_cfg, setup.idx_cfg, &evaluations, op);

    const Series improved = trial_fit.le(base_fit);
    const Series improved_2d = improved.reshape({n_pop, 1}).expand(std::vector<std::int64_t>{n_pop, n_dim});

    const Series base_next = improved_2d.where(trial, base);
    const Series fit_next = improved.where(trial_fit, base_fit);

    pop = base_next.index_select(0, inv_perm);
    fit = fit_next.index_select(0, inv_perm);

    best_idx = argmin_index(fit);
    const double current_best = scalar_as_f64(row_by_index(fit.reshape({n_pop, 1}), best_idx), op);
    if (current_best < best_val) {
      best_val = current_best;
      best = row_by_index(pop, best_idx);
    }

    last_step = "generation";
    maybe_print_iteration(config.display, iterations, evaluations, best_val, last_step);

    if (converged_by_relative_improvement(
            iterations, config.check_every, best_val, &check_best,
            config.relative_improvement_tolerance,
              population_spread(fit))) {
      converged = true;
      break;
    }
  }

  const PopulationSolverStatus status = converged
      ? PopulationSolverStatus::Converged
      : ((evaluations >= max_evaluations && !(eval_budget_derived && iterations >= max_generations))
             ? PopulationSolverStatus::ReachedEvaluationBudget
             : PopulationSolverStatus::ReachedIterationBudget);

  return build_outcome(best, best_val, setup, bounds, iterations, evaluations, last_step, status);
}

PopulationSolverOutcome differential_evolution_prmm_minimizer(
    const std::function<Series(const Series&)>& objective,
    const Series& initial,
    const DifferentialEvolutionPrmmConfig& config) {
  constexpr const char* op = "differential_evolution_prmm_minimizer";
  require_valid_initial(initial, op);
  if (config.population_size < 10) {
    ::pluss::throw_error(std::string(op) + ": population_size must be >= 10");
  }

  const CommonSetup setup = make_common_setup(initial, config.promote_cpu_to_float64);
  const BoundsPack bounds = prepare_bounds(config.lower_bounds, config.upper_bounds, setup, op);

  std::int64_t n_pop = config.population_size;
  const std::int64_t n_dim = setup.dimension;
  std::int64_t max_evaluations = (config.max_evaluations > 0) ? config.max_evaluations : 100000;
  const std::int64_t pmax = std::max<std::int64_t>(1, config.pmax);

  std::int64_t max_generations = config.max_generations;
  if (max_generations <= 0) {
    max_generations = static_cast<std::int64_t>(
        std::ceil(static_cast<double>(max_evaluations) / static_cast<double>(pmax * n_pop)));
    max_generations = std::max<std::int64_t>(1, max_generations);
  }

  if (config.execution_mode == PopulationSolverExecutionMode::OptimMasterCompat) {
    const HostBoundsPack host_bounds = to_host_bounds(bounds, n_dim);
    std::mt19937_64 rng(config.rng_seed);

    std::vector<std::vector<double>> pop =
        sample_population_host(setup, host_bounds, n_pop, config.initial_radius, &rng);
    std::vector<double> fit(static_cast<std::size_t>(n_pop), std::numeric_limits<double>::infinity());
    std::vector<double> f_vec(
        static_cast<std::size_t>(n_pop), config.initial_mutation_factor);
    std::vector<double> cr_vec(
        static_cast<std::size_t>(n_pop), config.initial_crossover_rate);

    std::int64_t evaluations = 0;
    for (std::int64_t i = 0; i < n_pop; ++i) {
      fit[static_cast<std::size_t>(i)] =
          eval_host_candidate(objective, pop[static_cast<std::size_t>(i)], host_bounds, setup, op);
      ++evaluations;
      if (evaluations >= max_evaluations) {
        break;
      }
    }

    std::int64_t best_idx = argmin_host(fit);
    std::vector<double> best = pop[static_cast<std::size_t>(best_idx)];
    double best_val = fit[static_cast<std::size_t>(best_idx)];
    double check_best = best_val;

    std::vector<double> best_main = best;
    std::vector<double> best_best = best;
    std::vector<double> xchg_vec = best;
    double best_main_val = best_val;
    double best_best_val = best_val;

    std::int64_t iterations = 0;
    std::string last_step = "initial_population";
    maybe_print_iteration(config.display, iterations, evaluations, best_val, last_step);

    std::int64_t reductions_done = 0;
    bool converged = false;

    while (iterations < max_generations + 1 && evaluations < max_evaluations) {
      ++iterations;

      if (config.reduction_passes > 0 && reductions_done < config.reduction_passes &&
          iterations == max_generations && n_pop >= 4) {
        const std::int64_t half = n_pop / 2;
        for (std::int64_t j = 0; j < half; ++j) {
          const std::int64_t j2 = j + half;
          if (fit[static_cast<std::size_t>(j)] > fit[static_cast<std::size_t>(j2)]) {
            pop[static_cast<std::size_t>(j)] = pop[static_cast<std::size_t>(j2)];
            fit[static_cast<std::size_t>(j)] = fit[static_cast<std::size_t>(j2)];
            f_vec[static_cast<std::size_t>(j)] = f_vec[static_cast<std::size_t>(j2)];
            cr_vec[static_cast<std::size_t>(j)] = cr_vec[static_cast<std::size_t>(j2)];
          }
        }
        pop.resize(static_cast<std::size_t>(half));
        fit.resize(static_cast<std::size_t>(half));
        f_vec.resize(static_cast<std::size_t>(half));
        cr_vec.resize(static_cast<std::size_t>(half));
        n_pop = half;
        ++reductions_done;
        max_generations *= 2;
        iterations = 1;

        best_idx = argmin_host(fit);
        best = pop[static_cast<std::size_t>(best_idx)];
        best_val = fit[static_cast<std::size_t>(best_idx)];
        check_best = best_val;
        best_main = best;
        best_best = best;
        xchg_vec = best;
        best_main_val = best_val;
        best_best_val = best_val;

        last_step = "reduce";
        maybe_print_iteration(config.display, iterations, evaluations, best_val, last_step);
        continue;
      }

      const auto old_pop = pop;
      const auto old_fit = fit;

      const std::int64_t elite = std::clamp<std::int64_t>(
          config.elite_count, 1, std::max<std::int64_t>(1, n_pop - 1));
      const std::int64_t split = std::max<std::int64_t>(1, n_pop - elite);

      for (std::int64_t i = 0; i < split && evaluations < max_evaluations; ++i) {
        if (rand_u01(&rng) < config.tau_mutation) {
          f_vec[static_cast<std::size_t>(i)] =
              config.mutation_lower +
              (config.mutation_upper - config.mutation_lower) * rand_u01(&rng);
        }
        if (rand_u01(&rng) < config.tau_crossover) {
          cr_vec[static_cast<std::size_t>(i)] = rand_u01(&rng);
        }

        std::int64_t c1 = 0;
        std::int64_t c2 = 0;
        std::int64_t c3 = 0;
        do {
          c1 = rand_index_inclusive(0, n_pop - 1, &rng);
        } while (c1 == i);
        do {
          c2 = rand_index_inclusive(0, n_pop - 1, &rng);
        } while (c2 == i || c2 == c1);
        do {
          c3 = rand_index_inclusive(0, n_pop - 1, &rng);
        } while (c3 == i || c3 == c1 || c3 == c2);

        const std::int64_t rand_ind = rand_index_inclusive(0, n_dim - 1, &rng);
        std::vector<double> prop(static_cast<std::size_t>(n_dim), 0.0);
        for (std::int64_t k = 0; k < n_dim; ++k) {
          if (rand_u01(&rng) < cr_vec[static_cast<std::size_t>(i)] || k == rand_ind) {
            const double rs = rand_u01(&rng);
            if (rs < config.random_main_strategy_probability || n_pop >= 100) {
              prop[static_cast<std::size_t>(k)] =
                  old_pop[static_cast<std::size_t>(c3)][static_cast<std::size_t>(k)] +
                  f_vec[static_cast<std::size_t>(i)] *
                      (old_pop[static_cast<std::size_t>(c1)][static_cast<std::size_t>(k)] -
                       old_pop[static_cast<std::size_t>(c2)][static_cast<std::size_t>(k)]);
            } else {
              prop[static_cast<std::size_t>(k)] =
                  best_main[static_cast<std::size_t>(k)] +
                  f_vec[static_cast<std::size_t>(i)] *
                      (old_pop[static_cast<std::size_t>(c1)][static_cast<std::size_t>(k)] -
                       old_pop[static_cast<std::size_t>(c2)][static_cast<std::size_t>(k)]);
            }
          } else {
            prop[static_cast<std::size_t>(k)] =
                old_pop[static_cast<std::size_t>(i)][static_cast<std::size_t>(k)];
          }
        }

        const double prop_fit = eval_host_candidate(objective, prop, host_bounds, setup, op);
        ++evaluations;
        if (prop_fit <= old_fit[static_cast<std::size_t>(i)]) {
          pop[static_cast<std::size_t>(i)] = std::move(prop);
          fit[static_cast<std::size_t>(i)] = prop_fit;
        } else {
          pop[static_cast<std::size_t>(i)] = old_pop[static_cast<std::size_t>(i)];
          fit[static_cast<std::size_t>(i)] = old_fit[static_cast<std::size_t>(i)];
        }
      }

      if (split > 0) {
        std::int64_t i_main = 0;
        double f_main = fit[0];
        for (std::int64_t i = 1; i < split; ++i) {
          if (fit[static_cast<std::size_t>(i)] < f_main) {
            f_main = fit[static_cast<std::size_t>(i)];
            i_main = i;
          }
        }
        best_main = pop[static_cast<std::size_t>(i_main)];
        best_main_val = f_main;
        if (best_main_val < best_best_val) {
          xchg_vec = best_main;
        }
      }

      for (std::int64_t i = split; i < n_pop && evaluations < max_evaluations; ++i) {
        if (rand_u01(&rng) < config.tau_mutation) {
          f_vec[static_cast<std::size_t>(i)] =
              config.mutation_lower +
              (config.mutation_upper - config.mutation_lower) * rand_u01(&rng);
        }
        if (rand_u01(&rng) < config.tau_crossover) {
          cr_vec[static_cast<std::size_t>(i)] = rand_u01(&rng);
        }

        std::int64_t c1 = 0;
        std::int64_t c2 = 0;
        do {
          c1 = rand_index_inclusive(0, n_pop - 1, &rng);
        } while (c1 == i);
        do {
          c2 = rand_index_inclusive(0, n_pop - 1, &rng);
        } while (c2 == i || c2 == c1);

        const std::int64_t rand_ind = rand_index_inclusive(0, n_dim - 1, &rng);
        std::vector<double> prop(static_cast<std::size_t>(n_dim), 0.0);
        for (std::int64_t k = 0; k < n_dim; ++k) {
          if (rand_u01(&rng) < cr_vec[static_cast<std::size_t>(i)] || k == rand_ind) {
            prop[static_cast<std::size_t>(k)] =
                best_best[static_cast<std::size_t>(k)] +
                f_vec[static_cast<std::size_t>(i)] *
                    (old_pop[static_cast<std::size_t>(c1)][static_cast<std::size_t>(k)] -
                     old_pop[static_cast<std::size_t>(c2)][static_cast<std::size_t>(k)]);
          } else {
            prop[static_cast<std::size_t>(k)] =
                old_pop[static_cast<std::size_t>(i)][static_cast<std::size_t>(k)];
          }
        }

        const double prop_fit = eval_host_candidate(objective, prop, host_bounds, setup, op);
        ++evaluations;
        if (prop_fit <= old_fit[static_cast<std::size_t>(i)]) {
          pop[static_cast<std::size_t>(i)] = std::move(prop);
          fit[static_cast<std::size_t>(i)] = prop_fit;
        } else {
          pop[static_cast<std::size_t>(i)] = old_pop[static_cast<std::size_t>(i)];
          fit[static_cast<std::size_t>(i)] = old_fit[static_cast<std::size_t>(i)];
        }
      }

      if (split < n_pop) {
        std::int64_t i_best = split;
        double f_best = fit[static_cast<std::size_t>(split)];
        for (std::int64_t i = split + 1; i < n_pop; ++i) {
          if (fit[static_cast<std::size_t>(i)] < f_best) {
            f_best = fit[static_cast<std::size_t>(i)];
            i_best = i;
          }
        }
        best_best = pop[static_cast<std::size_t>(i_best)];
        best_best_val = f_best;

        if (best_best_val < best_main_val) {
          double ratio_sum = 0.0;
          for (std::int64_t j = 0; j < n_dim; ++j) {
            double min_val = old_pop[0][static_cast<std::size_t>(j)];
            for (std::int64_t i = 1; i < n_pop; ++i) {
              min_val = std::min(min_val, old_pop[static_cast<std::size_t>(i)][static_cast<std::size_t>(j)]);
            }
            double denom =
                xchg_vec[static_cast<std::size_t>(j)] - min_val;
            if (std::abs(denom) < 1e-12) {
              denom = (denom < 0.0) ? -1e-12 : 1e-12;
            }
            ratio_sum +=
                (best_best[static_cast<std::size_t>(j)] - min_val) / denom;
          }
          ratio_sum /= static_cast<double>(n_dim);
          if (std::abs(ratio_sum - 1.0) > config.diversity_epsilon) {
            best_main = best_best;
            best_main_val = best_best_val;
          } else {
            best_best = best_main;
            best_best_val = best_main_val;
          }
        } else {
          best_best = best_main;
          best_best_val = best_main_val;
        }
      }

      best_idx = argmin_host(fit);
      const double current_best = fit[static_cast<std::size_t>(best_idx)];
      if (current_best < best_val) {
        best_val = current_best;
        best = pop[static_cast<std::size_t>(best_idx)];
      }

      last_step = "generation";
      maybe_print_iteration(config.display, iterations, evaluations, best_val, last_step);

      if (converged_by_relative_improvement(
              iterations, config.check_every, best_val, &check_best,
              config.relative_improvement_tolerance,
              population_spread(fit))) {
        converged = true;
        break;
      }
    }

    const PopulationSolverStatus status = converged
        ? PopulationSolverStatus::Converged
        : ((evaluations >= max_evaluations)
               ? PopulationSolverStatus::ReachedEvaluationBudget
               : PopulationSolverStatus::ReachedIterationBudget);

    return build_outcome(
        host_vector_to_series(best, setup.work_cfg),
        best_val,
        setup,
        bounds,
        iterations,
        evaluations,
        last_step,
        status);
  }

  RngState rng{config.rng_seed, 0ULL};

  Series pop = sample_population(setup.flat_initial, n_pop, config.initial_radius, bounds, setup.work_cfg, &rng);
  std::int64_t evaluations = 0;
  Series fit = eval_population(objective, pop, bounds, setup.original_shape, setup.work_cfg, setup.idx_cfg, &evaluations, op);

  Series f_vec = pluss::series::full({n_pop}, config.initial_mutation_factor, setup.work_cfg);
  Series cr_vec = pluss::series::full({n_pop}, config.initial_crossover_rate, setup.work_cfg);

  std::int64_t best_idx = argmin_index(fit);
  Series best = row_by_index(pop, best_idx);
  double best_val = scalar_as_f64(row_by_index(fit.reshape({n_pop, 1}), best_idx), op);
  double check_best = best_val;

  Series best_main = best;
  Series best_best = best;
  Series xchg_vec = best;
  double best_main_val = best_val;
  double best_best_val = best_val;

  std::int64_t iterations = 0;
  std::string last_step = "initial_population";
  maybe_print_iteration(config.display, iterations, evaluations, best_val, last_step);

  std::int64_t reductions_done = 0;
  bool converged = false;

  while (iterations < max_generations + 1 && evaluations < max_evaluations) {
    ++iterations;

    if (config.reduction_passes > 0 && reductions_done < config.reduction_passes &&
        iterations == max_generations && n_pop >= 4) {
      const std::int64_t half = n_pop / 2;
      const Series i_first = pluss::series::arange(0, half, setup.idx_cfg);
      const Series i_second = pluss::series::arange(half, half + half, setup.idx_cfg);

      const Series pop_a = pop.index_select(0, i_first);
      const Series pop_b = pop.index_select(0, i_second);
      const Series fit_a = fit.index_select(0, i_first);
      const Series fit_b = fit.index_select(0, i_second);
      const Series f_a = f_vec.index_select(0, i_first);
      const Series f_b = f_vec.index_select(0, i_second);
      const Series cr_a = cr_vec.index_select(0, i_first);
      const Series cr_b = cr_vec.index_select(0, i_second);

      const Series keep_a = fit_a.lt(fit_b);
      const Series keep_a_2d = keep_a.reshape({half, 1}).expand(std::vector<std::int64_t>{half, n_dim});

      pop = keep_a_2d.where(pop_a, pop_b).contiguous();
      fit = keep_a.where(fit_a, fit_b).contiguous();
      f_vec = keep_a.where(f_a, f_b).contiguous();
      cr_vec = keep_a.where(cr_a, cr_b).contiguous();
      n_pop = half;
      ++reductions_done;

      max_generations *= 2;
      iterations = 1;

      best_idx = argmin_index(fit);
      best = row_by_index(pop, best_idx);
      best_val = scalar_as_f64(row_by_index(fit.reshape({n_pop, 1}), best_idx), op);
      check_best = best_val;
      best_main = best;
      best_best = best;
      xchg_vec = best;
      best_main_val = best_val;
      best_best_val = best_val;

      last_step = "reduce";
      maybe_print_iteration(config.display, iterations, evaluations, best_val, last_step);

      continue;
    }

    const Series pop_old = pop;

    const Series adapt_u_f = pluss::series::rand({n_pop}, setup.work_cfg, rng.next());
    const Series adapt_new_f = pluss::series::rand({n_pop}, setup.work_cfg, rng.next())
        .mul(config.mutation_upper - config.mutation_lower)
        .add(config.mutation_lower);
    f_vec = adapt_u_f.lt(config.tau_mutation).where(adapt_new_f, f_vec);

    const Series adapt_u_cr = pluss::series::rand({n_pop}, setup.work_cfg, rng.next());
    const Series adapt_new_cr = pluss::series::rand({n_pop}, setup.work_cfg, rng.next());
    cr_vec = adapt_u_cr.lt(config.tau_crossover).where(adapt_new_cr, cr_vec);

    const Series perm = random_permutation(n_pop, setup.work_cfg, &rng);
    const Series inv_perm = inverse_permutation(perm, setup.idx_cfg);

    const Series base = pop.index_select(0, perm);
    const Series base_fit = fit.index_select(0, perm);
    const Series f_base = f_vec.index_select(0, perm);
    const Series cr_base = cr_vec.index_select(0, perm);

    const Series r1 = roll_rows(base, 1, setup.idx_cfg);
    const Series r2 = roll_rows(base, 2, setup.idx_cfg);
    const Series r3 = roll_rows(base, 3, setup.idx_cfg);

    const Series f_mat = f_base.reshape({n_pop, 1}).expand(std::vector<std::int64_t>{n_pop, n_dim});
    const Series best_main_expand = best_main.reshape({1, n_dim}).expand(std::vector<std::int64_t>{n_pop, n_dim});
    const Series best_best_expand = best_best.reshape({1, n_dim}).expand(std::vector<std::int64_t>{n_pop, n_dim});

    const Series mutant_main_rand = r3.add(f_mat.mul(r1.sub(r2)));
    const Series mutant_main_best = best_main_expand.add(f_mat.mul(r1.sub(r2)));
    Series mutant_main = mutant_main_rand;
    if (n_pop < 100) {
      const Series main_selector = pluss::series::rand({n_pop, n_dim}, setup.work_cfg, rng.next())
          .lt(config.random_main_strategy_probability);
      mutant_main = main_selector.where(mutant_main_rand, mutant_main_best);
    }
    const Series mutant_elite = best_best_expand.add(f_mat.mul(r1.sub(r2)));

    const std::int64_t elite = std::clamp<std::int64_t>(config.elite_count, 1, std::max<std::int64_t>(1, n_pop - 1));
    const std::int64_t split = std::max<std::int64_t>(1, n_pop - elite);
    const Series group_main = pluss::series::arange(0, n_pop, setup.idx_cfg)
        .lt(split)
        .reshape({n_pop, 1})
        .expand(std::vector<std::int64_t>{n_pop, n_dim});
    const Series mutant = group_main.where(mutant_main, mutant_elite);

    const Series cross = crossover_mask_per_row_rate(
        n_pop, n_dim, cr_base,
        setup.work_cfg, setup.idx_cfg, setup.bool_cfg, &rng);

    Series trial = cross.where(mutant, base);
    const Series trial_fit = eval_population(objective, trial, bounds, setup.original_shape, setup.work_cfg, setup.idx_cfg, &evaluations, op);

    const Series improved = trial_fit.le(base_fit);
    const Series improved_2d = improved.reshape({n_pop, 1}).expand(std::vector<std::int64_t>{n_pop, n_dim});

    const Series base_next = improved_2d.where(trial, base);
    const Series fit_next = improved.where(trial_fit, base_fit);

    if (split > 0) {
      const Series main_idx = pluss::series::arange(0, split, setup.idx_cfg);
      const Series fit_main = fit_next.index_select(0, main_idx);
      const std::int64_t i_main = argmin_index(fit_main);
      best_main = row_by_index(base_next, i_main);
      best_main_val = scalar_as_f64(row_by_index(fit_main.reshape({split, 1}), i_main), op);

      if (best_main_val < best_best_val) {
        xchg_vec = best_main;
      }
    }

    const std::int64_t n_pop_best = n_pop - split;
    if (n_pop_best > 0) {
      const Series elite_idx = pluss::series::arange(split, n_pop, setup.idx_cfg);
      const Series fit_elite = fit_next.index_select(0, elite_idx);
      const std::int64_t i_best_local = argmin_index(fit_elite);
      const std::int64_t i_best = split + i_best_local;
      const Series best_best_candidate = row_by_index(base_next, i_best);
      best_best_val = scalar_as_f64(row_by_index(fit_elite.reshape({n_pop_best, 1}), i_best_local), op);
      best_best = best_best_candidate;

      if (best_best_val < best_main_val) {
        const auto best_best_host = best_best_candidate.to(ScalarType::Float64).to_vec<double>();
        const auto xchg_host = xchg_vec.to(ScalarType::Float64).to_vec<double>();
        const auto min_host = pop_old.amin(0, false).to(ScalarType::Float64).to_vec<double>();

        double ratio_sum = 0.0;
        for (std::size_t j = 0; j < best_best_host.size(); ++j) {
          double denom = xchg_host[j] - min_host[j];
          if (std::abs(denom) < 1e-12) {
            denom = (denom < 0.0) ? -1e-12 : 1e-12;
          }
          ratio_sum += (best_best_host[j] - min_host[j]) / denom;
        }
        ratio_sum /= static_cast<double>(best_best_host.size());

        if (std::abs(ratio_sum - 1.0) > config.diversity_epsilon) {
          best_main = best_best_candidate;
          best_main_val = best_best_val;
        } else {
          best_best = best_main;
          best_best_val = best_main_val;
        }
      } else {
        best_best = best_main;
        best_best_val = best_main_val;
      }
    }

    pop = base_next.index_select(0, inv_perm);
    fit = fit_next.index_select(0, inv_perm);
    f_vec = f_base.index_select(0, inv_perm);
    cr_vec = cr_base.index_select(0, inv_perm);

    best_idx = argmin_index(fit);
    const double current_best = scalar_as_f64(row_by_index(fit.reshape({n_pop, 1}), best_idx), op);
    if (current_best < best_val) {
      best_val = current_best;
      best = row_by_index(pop, best_idx);
    }

    last_step = "generation";
    maybe_print_iteration(config.display, iterations, evaluations, best_val, last_step);

    if (converged_by_relative_improvement(
            iterations, config.check_every, best_val, &check_best,
            config.relative_improvement_tolerance,
              population_spread(fit))) {
      converged = true;
      break;
    }
  }

  const PopulationSolverStatus status = converged
      ? PopulationSolverStatus::Converged
      : ((evaluations >= max_evaluations)
             ? PopulationSolverStatus::ReachedEvaluationBudget
             : PopulationSolverStatus::ReachedIterationBudget);

  return build_outcome(best, best_val, setup, bounds, iterations, evaluations, last_step, status);
}

PopulationSolverOutcome particle_swarm_minimizer(
    const std::function<Series(const Series&)>& objective,
    const Series& initial,
    const ParticleSwarmConfig& config) {
  constexpr const char* op = "particle_swarm_minimizer";
  require_valid_initial(initial, op);
  if (config.population_size < 4) {
    ::pluss::throw_error(std::string(op) + ": population_size must be >= 4");
  }

  const CommonSetup setup = make_common_setup(initial, config.promote_cpu_to_float64);
  const BoundsPack bounds = prepare_bounds(config.lower_bounds, config.upper_bounds, setup, op);

  const std::int64_t n_pop = config.population_size;
  const std::int64_t n_dim = setup.dimension;
  const std::int64_t max_generations = (config.max_generations > 0) ? config.max_generations : 1000;

  std::int64_t max_evaluations = config.max_evaluations;
  const bool eval_budget_derived = max_evaluations <= 0;  // a generation limit then ends the run, not the evaluation budget
  if (max_evaluations <= 0) {
    max_evaluations = n_pop * (max_generations + 1) + max_generations;
  }

  if (config.execution_mode == PopulationSolverExecutionMode::OptimMasterCompat) {
    const HostBoundsPack host_bounds = to_host_bounds(bounds, n_dim);
    std::mt19937_64 rng(config.rng_seed);

    const std::int64_t n_center = config.center_particle ? 1 : 0;
    std::vector<std::vector<double>> pos =
        sample_population_host(setup, host_bounds, n_pop, config.initial_radius, &rng);
    pos.resize(static_cast<std::size_t>(n_pop + n_center),
               std::vector<double>(static_cast<std::size_t>(n_dim), 0.0));
    std::vector<double> fit(
        static_cast<std::size_t>(n_pop + n_center),
        std::numeric_limits<double>::infinity());
    std::vector<std::vector<double>> pbest = pos;
    std::vector<double> pbest_fit = fit;
    std::vector<std::vector<double>> vel(
        static_cast<std::size_t>(n_pop),
        std::vector<double>(static_cast<std::size_t>(n_dim), 0.0));

    std::int64_t evaluations = 0;
    for (std::int64_t i = 0; i < n_pop; ++i) {
      fit[static_cast<std::size_t>(i)] =
          eval_host_candidate(objective, pos[static_cast<std::size_t>(i)], host_bounds, setup, op);
      ++evaluations;
      if (evaluations >= max_evaluations) {
        break;
      }
    }

    if (config.center_particle && evaluations < max_evaluations) {
      std::vector<double> center(static_cast<std::size_t>(n_dim), 0.0);
      for (std::int64_t i = 0; i < n_pop; ++i) {
        for (std::int64_t j = 0; j < n_dim; ++j) {
          center[static_cast<std::size_t>(j)] +=
              pos[static_cast<std::size_t>(i)][static_cast<std::size_t>(j)];
        }
      }
      for (std::int64_t j = 0; j < n_dim; ++j) {
        center[static_cast<std::size_t>(j)] /= static_cast<double>(n_pop);
      }
      pos[static_cast<std::size_t>(n_pop)] = center;
      fit[static_cast<std::size_t>(n_pop)] =
          eval_host_candidate(objective, center, host_bounds, setup, op);
      ++evaluations;
    }

    pbest = pos;
    pbest_fit = fit;

    std::int64_t g_idx = argmin_host(pbest_fit);
    std::vector<double> gbest = pbest[static_cast<std::size_t>(g_idx)];
    double gbest_val = pbest_fit[static_cast<std::size_t>(g_idx)];
    double check_best = gbest_val;

    std::int64_t iterations = 0;
    std::string last_step = "initial_population";
    maybe_print_iteration(config.display, iterations, evaluations, gbest_val, last_step);

    bool converged = false;
    double inertia = config.inertia;
    while (iterations < max_generations && evaluations < max_evaluations) {
      ++iterations;

      if (config.linear_inertia_schedule) {
        inertia = config.inertia_min +
            (config.inertia_max - config.inertia_min) *
                static_cast<double>(iterations + 1) /
                static_cast<double>(std::max<std::int64_t>(1, max_generations));
      } else {
        inertia *= config.inertia_damp;
      }

      double c1 = config.cognitive;
      double c2 = config.social;
      if (config.time_varying_acceleration) {
        c1 = config.cognitive -
            (config.cognitive - config.final_cognitive) *
                static_cast<double>(iterations + 1) /
                static_cast<double>(std::max<std::int64_t>(1, max_generations));
        c2 = config.social -
            (config.social - config.final_social) *
                static_cast<double>(iterations + 1) /
                static_cast<double>(std::max<std::int64_t>(1, max_generations));
      }

      std::vector<double> vmax(static_cast<std::size_t>(n_dim), 0.0);
      bool do_clamp = config.velocity_clamp_ratio > 0.0;
      if (do_clamp) {
        for (std::int64_t j = 0; j < n_dim; ++j) {
          if (bounds.enabled) {
            vmax[static_cast<std::size_t>(j)] =
                std::abs(host_bounds.upper[static_cast<std::size_t>(j)] -
                         host_bounds.lower[static_cast<std::size_t>(j)]) *
                config.velocity_clamp_ratio;
          } else {
            vmax[static_cast<std::size_t>(j)] =
                config.initial_radius * config.velocity_clamp_ratio;
          }
        }
      }

      for (std::int64_t i = 0; i < n_pop && evaluations < max_evaluations; ++i) {
        for (std::int64_t j = 0; j < n_dim; ++j) {
          const double r1 = rand_u01(&rng);
          const double r2 = rand_u01(&rng);
          double v = inertia * vel[static_cast<std::size_t>(i)][static_cast<std::size_t>(j)] +
              c1 * r1 *
                  (pbest[static_cast<std::size_t>(i)][static_cast<std::size_t>(j)] -
                   pos[static_cast<std::size_t>(i)][static_cast<std::size_t>(j)]) +
              c2 * r2 *
                  (gbest[static_cast<std::size_t>(j)] -
                   pos[static_cast<std::size_t>(i)][static_cast<std::size_t>(j)]);
          if (do_clamp) {
            const double lim = vmax[static_cast<std::size_t>(j)];
            v = std::max(-lim, std::min(lim, v));
          }
          vel[static_cast<std::size_t>(i)][static_cast<std::size_t>(j)] = v;
          pos[static_cast<std::size_t>(i)][static_cast<std::size_t>(j)] += v;
        }

        fit[static_cast<std::size_t>(i)] =
            eval_host_candidate(objective, pos[static_cast<std::size_t>(i)], host_bounds, setup, op);
        ++evaluations;
        if (fit[static_cast<std::size_t>(i)] < pbest_fit[static_cast<std::size_t>(i)]) {
          pbest_fit[static_cast<std::size_t>(i)] = fit[static_cast<std::size_t>(i)];
          pbest[static_cast<std::size_t>(i)] = pos[static_cast<std::size_t>(i)];
        }
      }

      if (config.center_particle && evaluations < max_evaluations) {
        std::vector<double> center(static_cast<std::size_t>(n_dim), 0.0);
        for (std::int64_t i = 0; i < n_pop; ++i) {
          for (std::int64_t j = 0; j < n_dim; ++j) {
            center[static_cast<std::size_t>(j)] +=
                pos[static_cast<std::size_t>(i)][static_cast<std::size_t>(j)];
          }
        }
        for (std::int64_t j = 0; j < n_dim; ++j) {
          center[static_cast<std::size_t>(j)] /= static_cast<double>(n_pop);
        }
        pos[static_cast<std::size_t>(n_pop)] = center;
        fit[static_cast<std::size_t>(n_pop)] =
            eval_host_candidate(objective, center, host_bounds, setup, op);
        ++evaluations;
        if (fit[static_cast<std::size_t>(n_pop)] < pbest_fit[static_cast<std::size_t>(n_pop)]) {
          pbest_fit[static_cast<std::size_t>(n_pop)] = fit[static_cast<std::size_t>(n_pop)];
          pbest[static_cast<std::size_t>(n_pop)] = center;
        }
      }

      g_idx = argmin_host(pbest_fit);
      const double current_best = pbest_fit[static_cast<std::size_t>(g_idx)];
      if (current_best < gbest_val) {
        gbest_val = current_best;
        gbest = pbest[static_cast<std::size_t>(g_idx)];
      }

      last_step = "generation";
      maybe_print_iteration(config.display, iterations, evaluations, gbest_val, last_step);

      if (converged_by_relative_improvement(
              iterations, config.check_every, gbest_val, &check_best,
              config.relative_improvement_tolerance,
              population_spread(fit))) {
        converged = true;
        break;
      }
    }

    const PopulationSolverStatus status = converged
        ? PopulationSolverStatus::Converged
        : ((evaluations >= max_evaluations && !(eval_budget_derived && iterations >= max_generations))
               ? PopulationSolverStatus::ReachedEvaluationBudget
               : PopulationSolverStatus::ReachedIterationBudget);

    return build_outcome(
        host_vector_to_series(gbest, setup.work_cfg),
        gbest_val,
        setup,
        bounds,
        iterations,
        evaluations,
        last_step,
        status);
  }

  RngState rng{config.rng_seed, 0ULL};

  Series pop = sample_population(setup.flat_initial, n_pop, config.initial_radius, bounds, setup.work_cfg, &rng);
  Series vel = pluss::series::zeros({n_pop, n_dim}, setup.work_cfg);

  std::int64_t evaluations = 0;
  Series fit = eval_population(objective, pop, bounds, setup.original_shape, setup.work_cfg, setup.idx_cfg, &evaluations, op);

  Series pbest = pop;
  Series pbest_fit = fit;

  std::int64_t g_idx = argmin_index(pbest_fit);
  Series gbest = row_by_index(pbest, g_idx);
  double gbest_val = scalar_as_f64(row_by_index(pbest_fit.reshape({n_pop, 1}), g_idx), op);
  Series center_best = pluss::series::zeros({n_dim}, setup.work_cfg);
  double center_best_val = std::numeric_limits<double>::infinity();

  if (config.center_particle && evaluations < max_evaluations) {
    const Series center = pop.mean(0, false);
    const Series center_val = eval_candidate(objective, center, bounds, setup.original_shape, setup.work_cfg, op);
    ++evaluations;
    center_best = center;
    center_best_val = scalar_as_f64(center_val, op);
    if (center_best_val < gbest_val) {
      gbest = center_best;
      gbest_val = center_best_val;
    }
  }

  double check_best = gbest_val;

  std::int64_t iterations = 0;
  std::string last_step = "initial_population";
  maybe_print_iteration(config.display, iterations, evaluations, gbest_val, last_step);

  bool converged = false;
  double inertia = config.inertia;

  while (iterations < max_generations && evaluations < max_evaluations) {
    ++iterations;

    if (config.linear_inertia_schedule) {
      const double t = static_cast<double>(iterations + 1)
          / static_cast<double>(std::max<std::int64_t>(1, max_generations));
      inertia = config.inertia_min + (config.inertia_max - config.inertia_min) * t;
    } else {
      inertia *= config.inertia_damp;
    }

    double c1 = config.cognitive;
    double c2 = config.social;
    if (config.time_varying_acceleration) {
      const double t = static_cast<double>(iterations) / static_cast<double>(std::max<std::int64_t>(1, max_generations));
      c1 = config.cognitive - (config.cognitive - config.final_cognitive) * t;
      c2 = config.social - (config.social - config.final_social) * t;
    }

    const Series r1 = pluss::series::rand({n_pop, n_dim}, setup.work_cfg, rng.next());
    const Series r2 = pluss::series::rand({n_pop, n_dim}, setup.work_cfg, rng.next());
    const Series gbest_expand = gbest.reshape({1, n_dim}).expand(std::vector<std::int64_t>{n_pop, n_dim});

    vel = vel.mul(inertia)
        .add(r1.mul(c1).mul(pbest.sub(pop)))
        .add(r2.mul(c2).mul(gbest_expand.sub(pop)));

    if (config.velocity_clamp_ratio > 0.0) {
      Series vmax = pluss::series::full({1, n_dim}, config.initial_radius * config.velocity_clamp_ratio, setup.work_cfg);
      if (bounds.enabled) {
        vmax = bounds.upper.sub(bounds.lower).abs().reshape({1, n_dim}).mul(config.velocity_clamp_ratio);
      }
      const Series vmax_expand = vmax.expand(std::vector<std::int64_t>{n_pop, n_dim});
      vel = vel.maximum(vmax_expand.neg()).minimum(vmax_expand);
    }

    pop = pop.add(vel);
    fit = eval_population(objective, pop, bounds, setup.original_shape, setup.work_cfg, setup.idx_cfg, &evaluations, op);

    const Series improved = fit.lt(pbest_fit);
    const Series improved_2d = improved.reshape({n_pop, 1}).expand(std::vector<std::int64_t>{n_pop, n_dim});
    pbest = improved_2d.where(pop, pbest);
    pbest_fit = improved.where(fit, pbest_fit);

    if (config.center_particle && evaluations < max_evaluations) {
      const Series center = pop.mean(0, false);
      const Series center_val = eval_candidate(objective, center, bounds, setup.original_shape, setup.work_cfg, op);
      ++evaluations;
      const double center_val_f = scalar_as_f64(center_val, op);
      if (center_val_f < center_best_val) {
        center_best = center;
        center_best_val = center_val_f;
      }
    }

    g_idx = argmin_index(pbest_fit);
    const double current_best = scalar_as_f64(row_by_index(pbest_fit.reshape({n_pop, 1}), g_idx), op);
    if (current_best < gbest_val) {
      gbest = row_by_index(pbest, g_idx);
      gbest_val = current_best;
    }
    if (config.center_particle && center_best_val < gbest_val) {
      gbest = center_best;
      gbest_val = center_best_val;
    }

    last_step = "generation";
    maybe_print_iteration(config.display, iterations, evaluations, gbest_val, last_step);

    if (converged_by_relative_improvement(
            iterations, config.check_every, gbest_val, &check_best,
            config.relative_improvement_tolerance,
              population_spread(fit))) {
      converged = true;
      break;
    }
  }

  const PopulationSolverStatus status = converged
      ? PopulationSolverStatus::Converged
      : ((evaluations >= max_evaluations && !(eval_budget_derived && iterations >= max_generations))
             ? PopulationSolverStatus::ReachedEvaluationBudget
             : PopulationSolverStatus::ReachedIterationBudget);

  return build_outcome(gbest, gbest_val, setup, bounds, iterations, evaluations, last_step, status);
}

PopulationSolverOutcome particle_swarm_dv_minimizer(
    const std::function<Series(const Series&)>& objective,
    const Series& initial,
    const ParticleSwarmDvConfig& config) {
  constexpr const char* op = "particle_swarm_dv_minimizer";
  require_valid_initial(initial, op);
  if (config.population_size < 4) {
    ::pluss::throw_error(std::string(op) + ": population_size must be >= 4");
  }

  const CommonSetup setup = make_common_setup(initial, config.promote_cpu_to_float64);
  const BoundsPack bounds = prepare_bounds(config.lower_bounds, config.upper_bounds, setup, op);

  const std::int64_t n_pop = config.population_size;
  const std::int64_t n_dim = setup.dimension;
  const std::int64_t max_generations = (config.max_generations > 0) ? config.max_generations : 1000;

  std::int64_t max_evaluations = config.max_evaluations;
  const bool eval_budget_derived = max_evaluations <= 0;  // a generation limit then ends the run, not the evaluation budget
  if (max_evaluations <= 0) {
    max_evaluations = n_pop * (max_generations + 1);
  }

  if (config.execution_mode == PopulationSolverExecutionMode::OptimMasterCompat) {
    const HostBoundsPack host_bounds = to_host_bounds(bounds, n_dim);
    std::mt19937_64 rng(config.rng_seed);

    std::vector<std::vector<double>> pop =
        sample_population_host(setup, host_bounds, n_pop, config.initial_radius, &rng);
    std::vector<std::vector<double>> vel(
        static_cast<std::size_t>(n_pop),
        std::vector<double>(static_cast<std::size_t>(n_dim), 0.0));
    std::vector<double> fit(static_cast<std::size_t>(n_pop), std::numeric_limits<double>::infinity());
    std::vector<std::int64_t> stagnation(static_cast<std::size_t>(n_pop), 0);

    std::int64_t evaluations = 0;
    for (std::int64_t i = 0; i < n_pop; ++i) {
      fit[static_cast<std::size_t>(i)] =
          eval_host_candidate(objective, pop[static_cast<std::size_t>(i)], host_bounds, setup, op);
      ++evaluations;
      if (evaluations >= max_evaluations) {
        break;
      }
    }

    std::int64_t best_idx = argmin_host(fit);
    std::vector<double> best = pop[static_cast<std::size_t>(best_idx)];
    double best_val = fit[static_cast<std::size_t>(best_idx)];
    double check_best = best_val;

    std::int64_t iterations = 0;
    std::string last_step = "initial_population";
    maybe_print_iteration(config.display, iterations, evaluations, best_val, last_step);

    bool converged = false;
    double inertia = config.inertia;

    while (iterations < max_generations && evaluations < max_evaluations) {
      ++iterations;

      std::vector<double> pmax(static_cast<std::size_t>(n_dim), -std::numeric_limits<double>::infinity());
      std::vector<double> pmin(static_cast<std::size_t>(n_dim), std::numeric_limits<double>::infinity());
      for (std::int64_t i = 0; i < n_pop; ++i) {
        for (std::int64_t j = 0; j < n_dim; ++j) {
          pmax[static_cast<std::size_t>(j)] = std::max(
              pmax[static_cast<std::size_t>(j)],
              pop[static_cast<std::size_t>(i)][static_cast<std::size_t>(j)]);
          pmin[static_cast<std::size_t>(j)] = std::min(
              pmin[static_cast<std::size_t>(j)],
              pop[static_cast<std::size_t>(i)][static_cast<std::size_t>(j)]);
        }
      }

      bool reseeded = false;
      for (std::int64_t i = 0; i < n_pop && evaluations < max_evaluations; ++i) {
        std::int64_t c1 = 0;
        std::int64_t c2 = 0;
        do {
          c1 = rand_index_inclusive(0, n_pop - 1, &rng);
        } while (c1 == i);
        do {
          c2 = rand_index_inclusive(0, n_pop - 1, &rng);
        } while (c2 == i || c2 == c1);

        std::vector<double> trial = pop[static_cast<std::size_t>(i)];
        for (std::int64_t j = 0; j < n_dim; ++j) {
          if (rand_u01(&rng) <= config.crossover_rate) {
            const double rand_u = rand_u01(&rng);
            vel[static_cast<std::size_t>(i)][static_cast<std::size_t>(j)] =
                inertia * vel[static_cast<std::size_t>(i)][static_cast<std::size_t>(j)] +
                config.beta *
                    (pop[static_cast<std::size_t>(c1)][static_cast<std::size_t>(j)] -
                     pop[static_cast<std::size_t>(c2)][static_cast<std::size_t>(j)]) +
                config.social * rand_u *
                    (best[static_cast<std::size_t>(j)] -
                     pop[static_cast<std::size_t>(i)][static_cast<std::size_t>(j)]);
          }
          trial[static_cast<std::size_t>(j)] +=
              vel[static_cast<std::size_t>(i)][static_cast<std::size_t>(j)];
        }

        const double trial_fit =
            eval_host_candidate(objective, trial, host_bounds, setup, op);
        ++evaluations;
        if (trial_fit < fit[static_cast<std::size_t>(i)]) {
          pop[static_cast<std::size_t>(i)] = std::move(trial);
          fit[static_cast<std::size_t>(i)] = trial_fit;
          stagnation[static_cast<std::size_t>(i)] = 0;
        } else {
          ++stagnation[static_cast<std::size_t>(i)];
        }

        if (stagnation[static_cast<std::size_t>(i)] >= config.stagnation_limit &&
            evaluations < max_evaluations) {
          for (std::int64_t j = 0; j < n_dim; ++j) {
            pop[static_cast<std::size_t>(i)][static_cast<std::size_t>(j)] =
                pmin[static_cast<std::size_t>(j)] +
                rand_u01(&rng) *
                    (pmax[static_cast<std::size_t>(j)] - pmin[static_cast<std::size_t>(j)]);
          }
          stagnation[static_cast<std::size_t>(i)] = 0;
          fit[static_cast<std::size_t>(i)] =
              eval_host_candidate(objective, pop[static_cast<std::size_t>(i)], host_bounds, setup, op);
          ++evaluations;
          reseeded = true;
        }
      }

      inertia *= config.inertia_damp;

      best_idx = argmin_host(fit);
      const double current_best = fit[static_cast<std::size_t>(best_idx)];
      if (current_best < best_val) {
        best_val = current_best;
        best = pop[static_cast<std::size_t>(best_idx)];
      }

      last_step = reseeded ? "reseed" : "generation";
      maybe_print_iteration(config.display, iterations, evaluations, best_val, last_step);

      if (converged_by_relative_improvement(
              iterations, config.check_every, best_val, &check_best,
              config.relative_improvement_tolerance,
              population_spread(fit))) {
        converged = true;
        break;
      }
    }

    const PopulationSolverStatus status = converged
        ? PopulationSolverStatus::Converged
        : ((evaluations >= max_evaluations && !(eval_budget_derived && iterations >= max_generations))
               ? PopulationSolverStatus::ReachedEvaluationBudget
               : PopulationSolverStatus::ReachedIterationBudget);

    return build_outcome(
        host_vector_to_series(best, setup.work_cfg),
        best_val,
        setup,
        bounds,
        iterations,
        evaluations,
        last_step,
        status);
  }

  RngState rng{config.rng_seed, 0ULL};

  Series pop = sample_population(setup.flat_initial, n_pop, config.initial_radius, bounds, setup.work_cfg, &rng);
  Series vel = pluss::series::zeros({n_pop, n_dim}, setup.work_cfg);

  std::int64_t evaluations = 0;
  Series fit = eval_population(objective, pop, bounds, setup.original_shape, setup.work_cfg, setup.idx_cfg, &evaluations, op);

  std::int64_t best_idx = argmin_index(fit);
  Series best = row_by_index(pop, best_idx);
  double best_val = scalar_as_f64(row_by_index(fit.reshape({n_pop, 1}), best_idx), op);
  double check_best = best_val;

  Series stagnation = pluss::series::zeros({n_pop}, setup.idx_cfg);
  // A stagnant particle is re-seeded over the region the swarm started in, not over the box the swarm has collapsed to:
  // re-seeding inside the collapsed box adds no diversity and the swarm stays where it is.
  const Series region_min = pop.amin(0, false).reshape({1, n_dim}).expand(std::vector<std::int64_t>{n_pop, n_dim});
  const Series region_max = pop.amax(0, false).reshape({1, n_dim}).expand(std::vector<std::int64_t>{n_pop, n_dim});

  std::int64_t iterations = 0;
  std::string last_step = "initial_population";
  maybe_print_iteration(config.display, iterations, evaluations, best_val, last_step);

  bool converged = false;
  double inertia = config.inertia;

  while (iterations < max_generations && evaluations < max_evaluations) {
    ++iterations;


    const Series p1 = pop.index_select(0, random_permutation(n_pop, setup.work_cfg, &rng));
    const Series p2 = pop.index_select(0, random_permutation(n_pop, setup.work_cfg, &rng));
    const Series delta = p1.sub(p2);

    const Series rand_u = pluss::series::rand({n_pop, n_dim}, setup.work_cfg, rng.next());
    const Series best_expand = best.reshape({1, n_dim}).expand(std::vector<std::int64_t>{n_pop, n_dim});

    const Series v_prop = vel.mul(inertia)
        .add(delta.mul(config.beta))
        .add(rand_u.mul(config.social).mul(best_expand.sub(pop)));

    const Series apply_mask = pluss::series::rand({n_pop, n_dim}, setup.work_cfg, rng.next()).le(config.crossover_rate);
    vel = apply_mask.where(v_prop, vel);

    Series trial = pop.add(vel);
    const Series trial_fit = eval_population(objective, trial, bounds, setup.original_shape, setup.work_cfg, setup.idx_cfg, &evaluations, op);
    const Series improved = trial_fit.lt(fit);
    const Series improved_2d = improved.reshape({n_pop, 1}).expand(std::vector<std::int64_t>{n_pop, n_dim});

    pop = improved_2d.where(trial, pop);
    fit = improved.where(trial_fit, fit);

    const Series stagnation_inc = stagnation.add(1);
    stagnation = improved.where(pluss::series::zeros({n_pop}, setup.idx_cfg), stagnation_inc);

    const Series reset = stagnation.ge(config.stagnation_limit);
    const auto reset_host = reset.to(ScalarType::Bool).to_vec<bool>();
    if (std::any_of(reset_host.begin(), reset_host.end(), [](bool v) { return v; })) {
      const Series reseed = region_min.add(pluss::series::rand({n_pop, n_dim}, setup.work_cfg, rng.next()).mul(region_max.sub(region_min)));
      const Series reset_2d = reset.reshape({n_pop, 1}).expand(std::vector<std::int64_t>{n_pop, n_dim});
      pop = reset_2d.where(reseed, pop);
      stagnation = reset.where(pluss::series::zeros({n_pop}, setup.idx_cfg), stagnation);

      for (std::int64_t i = 0; i < n_pop; ++i) {
        if (!reset_host[static_cast<std::size_t>(i)]) {
          continue;
        }
        if (evaluations >= max_evaluations) {
          break;
        }
        const Series val = eval_candidate(objective, row_by_index(pop, i), bounds, setup.original_shape, setup.work_cfg, op);
        const Series idx = pluss::series::arange(i, i + 1, setup.idx_cfg);
        fit = fit.put(idx, val.reshape({1}), false);
        ++evaluations;
      }
      last_step = "reseed";
    } else {
      last_step = "generation";
    }

    inertia *= config.inertia_damp;

    best_idx = argmin_index(fit);
    const double current_best = scalar_as_f64(row_by_index(fit.reshape({n_pop, 1}), best_idx), op);
    if (current_best < best_val) {
      best_val = current_best;
      best = row_by_index(pop, best_idx);
    }

    maybe_print_iteration(config.display, iterations, evaluations, best_val, last_step);

    if (converged_by_relative_improvement(
            iterations, config.check_every, best_val, &check_best,
            config.relative_improvement_tolerance,
              population_spread(fit))) {
      converged = true;
      break;
    }
  }

  const PopulationSolverStatus status = converged
      ? PopulationSolverStatus::Converged
      : ((evaluations >= max_evaluations && !(eval_budget_derived && iterations >= max_generations))
             ? PopulationSolverStatus::ReachedEvaluationBudget
             : PopulationSolverStatus::ReachedIterationBudget);

  return build_outcome(best, best_val, setup, bounds, iterations, evaluations, last_step, status);
}

} // namespace pluss::subsp::math::optimization
