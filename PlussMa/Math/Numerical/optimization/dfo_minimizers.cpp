#include "dfo_minimizers.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#if __has_include(<fmt/format.h>)
#include <fmt/format.h>
#endif
#include <iostream>
#include <limits>
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
  Series lower{Series::undefined()};
  Series upper{Series::undefined()};
};

struct CommonSetup {
  std::vector<std::int64_t> original_shape{};
  std::int64_t dimension{0};
  Config work_cfg{};
  Config idx_cfg{};
  Config bool_cfg{};
  Series flat_initial{Series::undefined()};
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

double scalar_as_f64(const Series& x, const char* op) {
  if (!x.defined() || x.numel() != 1) {
    throw std::runtime_error(std::string(op) + ": expected scalar Series");
  }
  return x.to(pluss::pCPU, ScalarType::Float64).item<double>();
}

void require_valid_initial(const Series& initial, const char* op) {
  if (!initial.defined()) {
    throw std::runtime_error(std::string(op) + ": initial point is undefined");
  }
  if (initial.numel() < 1) {
    throw std::runtime_error(std::string(op) + ": initial point must be non-empty");
  }
  if (!initial.dtype().is_floating() || initial.dtype().is_complex()) {
    throw std::runtime_error(std::string(op) + ": initial point must be real floating-point");
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
    throw std::runtime_error(std::string(op) + ": both lower_bounds and upper_bounds must be defined");
  }

  const Series flat_lower = lower.reshape({setup.dimension}).to(setup.work_cfg.platform, setup.work_cfg.dtype).contiguous();
  const Series flat_upper = upper.reshape({setup.dimension}).to(setup.work_cfg.platform, setup.work_cfg.dtype).contiguous();

  if (flat_lower.numel() != setup.dimension || flat_upper.numel() != setup.dimension) {
    throw std::runtime_error(std::string(op) + ": bounds must have same number of elements as initial point");
  }

  const Series ok = flat_lower.le(flat_upper).all();
  if (!ok.item<bool>()) {
    throw std::runtime_error(std::string(op) + ": expected lower_bounds <= upper_bounds elementwise");
  }

  bounds.enabled = true;
  bounds.lower = flat_lower;
  bounds.upper = flat_upper;
  return bounds;
}

Series apply_bounds_vector(const Series& x, const BoundsPack& bounds) {
  if (!bounds.enabled) {
    return x;
  }
  return x.maximum(bounds.lower).minimum(bounds.upper);
}

Series apply_bounds_matrix(const Series& m, const BoundsPack& bounds, std::int64_t n_pop, std::int64_t n_dim) {
  if (!bounds.enabled) {
    return m;
  }
  const Series lower = bounds.lower.reshape({1, n_dim}).expand(std::vector<std::int64_t>{n_pop, n_dim});
  const Series upper = bounds.upper.reshape({1, n_dim}).expand(std::vector<std::int64_t>{n_pop, n_dim});
  return m.maximum(lower).minimum(upper);
}

Series eval_candidate(
    const std::function<Series(const Series&)>& objective,
    const Series& candidate_flat,
    const std::vector<std::int64_t>& original_shape,
    const Config& work_cfg,
    const char* op) {
  const Series x = candidate_flat.reshape(std::span<const std::int64_t>(original_shape.data(), original_shape.size()));
  const Series out = objective(x);

  if (!out.defined() || out.numel() != 1) {
    throw std::runtime_error(std::string(op) + ": objective must return scalar Series");
  }
  if (out.dtype().is_complex()) {
    throw std::runtime_error(std::string(op) + ": objective must return real scalar Series");
  }

  const double v = scalar_as_f64(out, op);
  if (!std::isfinite(v)) {
    throw std::runtime_error(std::string(op) + ": objective returned non-finite value");
  }

  return out.to(work_cfg.platform, work_cfg.dtype);
}

Series eval_population(
    const std::function<Series(const Series&)>& objective,
    const Series& population,
    const std::vector<std::int64_t>& original_shape,
    const Config& work_cfg,
    const Config& idx_cfg,
    std::int64_t* eval_counter,
    const char* op) {
  const std::int64_t n_pop = population.sizes()[0];
  Series fit = pluss::zeros({n_pop}, work_cfg);

  for (std::int64_t i = 0; i < n_pop; ++i) {
    const Series row = population.select(0, i);
    const Series val = eval_candidate(objective, row, original_shape, work_cfg, op);
    const Series idx = pluss::arange(i, i + 1, idx_cfg);
    fit = fit.put(idx, val.reshape({1}), false);
  }

  *eval_counter += n_pop;
  return fit;
}

std::int64_t argmin_index(const Series& x) {
  return x.argmin(0, false).to(pluss::pCPU, ScalarType::Int64).item<std::int64_t>();
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

  const Series head = pluss::arange(n - k, n, idx_cfg);
  const Series tail = pluss::arange(0, n - k, idx_cfg);
  const Series idx = concat_1d(head, tail);
  return m.index_select(0, idx);
}

Series random_permutation(
    std::int64_t n,
    const Config& work_cfg,
    RngState* rng) {
  return pluss::rand({n}, work_cfg, rng->next()).argsort(0, false).to(work_cfg.platform, ScalarType::Int64);
}

Series inverse_permutation(const Series& perm, const Config& idx_cfg) {
  const std::int64_t n = perm.numel();
  const Series idx = pluss::arange(0, n, idx_cfg);
  const Series base = pluss::zeros({n}, idx_cfg);
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
  Series mask = pluss::rand({n_pop, n_dim}, work_cfg, rng->next()).lt(crossover_rate);

  const Series jrand = pluss::randint(0, n_dim, {n_pop}, idx_cfg, rng->next()).reshape({n_pop, 1});
  const Series one_col_true = pluss::ones({n_pop, 1}, bool_cfg);
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
  Series mask = pluss::rand({n_pop, n_dim}, work_cfg, rng->next()).lt(cr);

  const Series jrand = pluss::randint(0, n_dim, {n_pop}, idx_cfg, rng->next()).reshape({n_pop, 1});
  const Series one_col_true = pluss::ones({n_pop, 1}, bool_cfg);
  mask = mask.scatter(1, jrand, one_col_true);
  return mask;
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

Series sample_population(
    const Series& flat_initial,
    std::int64_t n_pop,
    double initial_radius,
    const BoundsPack& bounds,
    const Config& work_cfg,
    RngState* rng) {
  const std::int64_t n_dim = flat_initial.numel();
  Series lb = flat_initial.sub(initial_radius);
  Series ub = flat_initial.add(initial_radius);

  if (bounds.enabled) {
    lb = lb.maximum(bounds.lower);
    ub = ub.minimum(bounds.upper);
  }

  const Series width = ub.sub(lb);
  const Series rand_u = pluss::rand({n_pop, n_dim}, work_cfg, rng->next());

  return lb.reshape({1, n_dim}).expand(std::vector<std::int64_t>{n_pop, n_dim})
      .add(rand_u.mul(width.reshape({1, n_dim}).expand(std::vector<std::int64_t>{n_pop, n_dim})));
}

PopulationSolverOutcome build_outcome(
    const Series& flat_best,
    double best_val,
    const CommonSetup& setup,
    std::int64_t iterations,
    std::int64_t evaluations,
    std::string last_step,
    PopulationSolverStatus status) {
  PopulationSolverOutcome out;
  out.minimizer = flat_best
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
    throw std::runtime_error(std::string(op) + ": population_size must be >= 5");
  }

  const CommonSetup setup = make_common_setup(initial, config.promote_cpu_to_float64);
  const BoundsPack bounds = prepare_bounds(config.lower_bounds, config.upper_bounds, setup, op);

  const std::int64_t n_pop = config.population_size;
  const std::int64_t n_dim = setup.dimension;
  const std::int64_t max_generations = (config.max_generations > 0) ? config.max_generations : 800;

  std::int64_t max_evaluations = config.max_evaluations;
  if (max_evaluations <= 0) {
    max_evaluations = n_pop * (max_generations + 1);
  }

  RngState rng{config.rng_seed, 0ULL};

  Series pop = sample_population(setup.flat_initial, n_pop, config.initial_radius, bounds, setup.work_cfg, &rng);
  std::int64_t evaluations = 0;
  Series fit = eval_population(objective, pop, setup.original_shape, setup.work_cfg, setup.idx_cfg, &evaluations, op);

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

    const Series f = pluss::full({n_pop, n_dim}, config.mutation_factor, setup.work_cfg);
    const Series mutant_rand = r1.add(f.mul(r2.sub(r3)));

    const Series best_expand = best.reshape({1, n_dim}).expand(std::vector<std::int64_t>{n_pop, n_dim});
    const Series mutant_best = best_expand.add(f.mul(r1.sub(r2)));
    const Series mutant = mutant_rand.mul(1.0 - config.best_mix).add(mutant_best.mul(config.best_mix));

    const Series cross = crossover_mask(
        n_pop, n_dim, config.crossover_rate,
        setup.work_cfg, setup.idx_cfg, setup.bool_cfg, &rng);

    Series trial = mutant.where(cross, base);
    trial = apply_bounds_matrix(trial, bounds, n_pop, n_dim);

    Series trial_fit = eval_population(objective, trial, setup.original_shape, setup.work_cfg, setup.idx_cfg, &evaluations, op);

    const Series improved = trial_fit.le(base_fit);
    const Series improved_2d = improved.reshape({n_pop, 1}).expand(std::vector<std::int64_t>{n_pop, n_dim});

    const Series base_next = trial.where(improved_2d, base);
    const Series fit_next = trial_fit.where(improved, base_fit);

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
            config.relative_improvement_tolerance)) {
      converged = true;
      break;
    }
  }

  const PopulationSolverStatus status = converged
      ? PopulationSolverStatus::Converged
      : ((evaluations >= max_evaluations)
             ? PopulationSolverStatus::ReachedEvaluationBudget
             : PopulationSolverStatus::ReachedIterationBudget);

  return build_outcome(best, best_val, setup, iterations, evaluations, last_step, status);
}

PopulationSolverOutcome differential_evolution_prmm_minimizer(
    const std::function<Series(const Series&)>& objective,
    const Series& initial,
    const DifferentialEvolutionPrmmConfig& config) {
  constexpr const char* op = "differential_evolution_prmm_minimizer";
  require_valid_initial(initial, op);
  if (config.population_size < 10) {
    throw std::runtime_error(std::string(op) + ": population_size must be >= 10");
  }

  const CommonSetup setup = make_common_setup(initial, config.promote_cpu_to_float64);
  const BoundsPack bounds = prepare_bounds(config.lower_bounds, config.upper_bounds, setup, op);

  std::int64_t n_pop = config.population_size;
  const std::int64_t n_dim = setup.dimension;
  const std::int64_t max_generations = (config.max_generations > 0) ? config.max_generations : 1200;

  std::int64_t max_evaluations = config.max_evaluations;
  if (max_evaluations <= 0) {
    max_evaluations = n_pop * (max_generations + 1);
  }

  RngState rng{config.rng_seed, 0ULL};

  Series pop = sample_population(setup.flat_initial, n_pop, config.initial_radius, bounds, setup.work_cfg, &rng);
  std::int64_t evaluations = 0;
  Series fit = eval_population(objective, pop, setup.original_shape, setup.work_cfg, setup.idx_cfg, &evaluations, op);

  Series f_vec = pluss::full({n_pop}, config.initial_mutation_factor, setup.work_cfg);
  Series cr_vec = pluss::full({n_pop}, config.initial_crossover_rate, setup.work_cfg);

  std::int64_t best_idx = argmin_index(fit);
  Series best = row_by_index(pop, best_idx);
  double best_val = scalar_as_f64(row_by_index(fit.reshape({n_pop, 1}), best_idx), op);
  double check_best = best_val;

  std::int64_t iterations = 0;
  std::string last_step = "initial_population";
  maybe_print_iteration(config.display, iterations, evaluations, best_val, last_step);

  const std::int64_t reduction_gap = std::max<std::int64_t>(1, max_generations / (config.reduction_passes + 1));
  std::int64_t reductions_done = 0;
  bool converged = false;

  while (iterations < max_generations && evaluations < max_evaluations) {
    ++iterations;

    if (config.reduction_passes > 0 &&
        reductions_done < config.reduction_passes &&
        iterations % reduction_gap == 0 &&
        n_pop >= 12) {
      const Series perm = random_permutation(n_pop, setup.work_cfg, &rng);
      const Series shuf_pop = pop.index_select(0, perm);
      const Series shuf_fit = fit.index_select(0, perm);
      const Series shuf_f = f_vec.index_select(0, perm);
      const Series shuf_cr = cr_vec.index_select(0, perm);

      const std::int64_t half = n_pop / 2;
      const Series i_first = pluss::arange(0, half, setup.idx_cfg);
      const Series i_second = pluss::arange(n_pop - half, n_pop, setup.idx_cfg);

      const Series pop_a = shuf_pop.index_select(0, i_first);
      const Series pop_b = shuf_pop.index_select(0, i_second);
      const Series fit_a = shuf_fit.index_select(0, i_first);
      const Series fit_b = shuf_fit.index_select(0, i_second);
      const Series f_a = shuf_f.index_select(0, i_first);
      const Series f_b = shuf_f.index_select(0, i_second);
      const Series cr_a = shuf_cr.index_select(0, i_first);
      const Series cr_b = shuf_cr.index_select(0, i_second);

      const Series keep_a = fit_a.le(fit_b);
      const Series keep_a_2d = keep_a.reshape({half, 1}).expand(std::vector<std::int64_t>{half, n_dim});

      pop = pop_a.where(keep_a_2d, pop_b);
      fit = fit_a.where(keep_a, fit_b);
      f_vec = f_a.where(keep_a, f_b);
      cr_vec = cr_a.where(keep_a, cr_b);
      n_pop = half;
      ++reductions_done;

      best_idx = argmin_index(fit);
      best = row_by_index(pop, best_idx);
      best_val = scalar_as_f64(row_by_index(fit.reshape({n_pop, 1}), best_idx), op);
      check_best = best_val;

      last_step = "reduce";
      maybe_print_iteration(config.display, iterations, evaluations, best_val, last_step);

      continue;
    }

    const Series adapt_u_f = pluss::rand({n_pop}, setup.work_cfg, rng.next());
    const Series adapt_new_f = pluss::rand({n_pop}, setup.work_cfg, rng.next())
        .mul(config.mutation_upper - config.mutation_lower)
        .add(config.mutation_lower);
    f_vec = adapt_new_f.where(adapt_u_f.lt(config.tau_mutation), f_vec);

    const Series adapt_u_cr = pluss::rand({n_pop}, setup.work_cfg, rng.next());
    const Series adapt_new_cr = pluss::rand({n_pop}, setup.work_cfg, rng.next());
    cr_vec = adapt_new_cr.where(adapt_u_cr.lt(config.tau_crossover), cr_vec);

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
    const Series best_expand = best.reshape({1, n_dim}).expand(std::vector<std::int64_t>{n_pop, n_dim});

    const Series mutant_main_rand = r3.add(f_mat.mul(r1.sub(r2)));
    const Series mutant_main_best = best_expand.add(f_mat.mul(r1.sub(r2)));
    const Series main_selector = pluss::rand({n_pop, 1}, setup.work_cfg, rng.next())
        .lt(config.random_main_strategy_probability)
        .expand(std::vector<std::int64_t>{n_pop, n_dim});
    const Series mutant_main = mutant_main_rand.where(main_selector, mutant_main_best);
    const Series mutant_elite = best_expand.add(f_mat.mul(r1.sub(r2)));

    const std::int64_t elite = std::clamp<std::int64_t>(config.elite_count, 1, std::max<std::int64_t>(1, n_pop - 1));
    const std::int64_t split = std::max<std::int64_t>(1, n_pop - elite);
    const Series group_main = pluss::arange(0, n_pop, setup.idx_cfg).lt(split).reshape({n_pop, 1}).expand(std::vector<std::int64_t>{n_pop, n_dim});
    const Series mutant = mutant_main.where(group_main, mutant_elite);

    const Series cross = crossover_mask_per_row_rate(
        n_pop, n_dim, cr_base,
        setup.work_cfg, setup.idx_cfg, setup.bool_cfg, &rng);

    Series trial = mutant.where(cross, base);
    trial = apply_bounds_matrix(trial, bounds, n_pop, n_dim);

    const Series trial_fit = eval_population(objective, trial, setup.original_shape, setup.work_cfg, setup.idx_cfg, &evaluations, op);

    const Series improved = trial_fit.le(base_fit);
    const Series improved_2d = improved.reshape({n_pop, 1}).expand(std::vector<std::int64_t>{n_pop, n_dim});

    const Series base_next = trial.where(improved_2d, base);
    const Series fit_next = trial_fit.where(improved, base_fit);

    pop = base_next.index_select(0, inv_perm);
    fit = fit_next.index_select(0, inv_perm);
    f_vec = f_base.index_select(0, inv_perm);
    cr_vec = cr_base.index_select(0, inv_perm);

    best_idx = argmin_index(fit);
    const double current_best = scalar_as_f64(row_by_index(fit.reshape({n_pop, 1}), best_idx), op);
    if (current_best < best_val - config.diversity_epsilon * std::numeric_limits<double>::epsilon()) {
      best_val = current_best;
      best = row_by_index(pop, best_idx);
    }

    last_step = "generation";
    maybe_print_iteration(config.display, iterations, evaluations, best_val, last_step);

    if (converged_by_relative_improvement(
            iterations, config.check_every, best_val, &check_best,
            config.relative_improvement_tolerance)) {
      converged = true;
      break;
    }
  }

  const PopulationSolverStatus status = converged
      ? PopulationSolverStatus::Converged
      : ((evaluations >= max_evaluations)
             ? PopulationSolverStatus::ReachedEvaluationBudget
             : PopulationSolverStatus::ReachedIterationBudget);

  return build_outcome(best, best_val, setup, iterations, evaluations, last_step, status);
}

PopulationSolverOutcome particle_swarm_minimizer(
    const std::function<Series(const Series&)>& objective,
    const Series& initial,
    const ParticleSwarmConfig& config) {
  constexpr const char* op = "particle_swarm_minimizer";
  require_valid_initial(initial, op);
  if (config.population_size < 4) {
    throw std::runtime_error(std::string(op) + ": population_size must be >= 4");
  }

  const CommonSetup setup = make_common_setup(initial, config.promote_cpu_to_float64);
  const BoundsPack bounds = prepare_bounds(config.lower_bounds, config.upper_bounds, setup, op);

  const std::int64_t n_pop = config.population_size;
  const std::int64_t n_dim = setup.dimension;
  const std::int64_t max_generations = (config.max_generations > 0) ? config.max_generations : 1000;

  std::int64_t max_evaluations = config.max_evaluations;
  if (max_evaluations <= 0) {
    max_evaluations = n_pop * (max_generations + 1) + max_generations;
  }

  RngState rng{config.rng_seed, 0ULL};

  Series pop = sample_population(setup.flat_initial, n_pop, config.initial_radius, bounds, setup.work_cfg, &rng);
  Series vel = pluss::zeros({n_pop, n_dim}, setup.work_cfg);

  std::int64_t evaluations = 0;
  Series fit = eval_population(objective, pop, setup.original_shape, setup.work_cfg, setup.idx_cfg, &evaluations, op);

  Series pbest = pop;
  Series pbest_fit = fit;

  std::int64_t g_idx = argmin_index(pbest_fit);
  Series gbest = row_by_index(pbest, g_idx);
  double gbest_val = scalar_as_f64(row_by_index(pbest_fit.reshape({n_pop, 1}), g_idx), op);
  double check_best = gbest_val;

  std::int64_t iterations = 0;
  std::string last_step = "initial_population";
  maybe_print_iteration(config.display, iterations, evaluations, gbest_val, last_step);

  bool converged = false;
  double inertia = config.inertia;

  while (iterations < max_generations && evaluations < max_evaluations) {
    ++iterations;

    if (config.linear_inertia_schedule) {
      const double t = static_cast<double>(iterations) / static_cast<double>(std::max<std::int64_t>(1, max_generations));
      inertia = config.inertia_max - (config.inertia_max - config.inertia_min) * t;
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

    const Series r1 = pluss::rand({n_pop, n_dim}, setup.work_cfg, rng.next());
    const Series r2 = pluss::rand({n_pop, n_dim}, setup.work_cfg, rng.next());
    const Series gbest_expand = gbest.reshape({1, n_dim}).expand(std::vector<std::int64_t>{n_pop, n_dim});

    vel = vel.mul(inertia)
        .add(r1.mul(c1).mul(pbest.sub(pop)))
        .add(r2.mul(c2).mul(gbest_expand.sub(pop)));

    if (config.velocity_clamp_ratio > 0.0) {
      Series vmax = pluss::full({1, n_dim}, config.initial_radius * config.velocity_clamp_ratio, setup.work_cfg);
      if (bounds.enabled) {
        vmax = bounds.upper.sub(bounds.lower).abs().reshape({1, n_dim}).mul(config.velocity_clamp_ratio);
      }
      const Series vmax_expand = vmax.expand(std::vector<std::int64_t>{n_pop, n_dim});
      vel = vel.maximum(vmax_expand.neg()).minimum(vmax_expand);
    }

    pop = pop.add(vel);
    pop = apply_bounds_matrix(pop, bounds, n_pop, n_dim);

    fit = eval_population(objective, pop, setup.original_shape, setup.work_cfg, setup.idx_cfg, &evaluations, op);

    const Series improved = fit.lt(pbest_fit);
    const Series improved_2d = improved.reshape({n_pop, 1}).expand(std::vector<std::int64_t>{n_pop, n_dim});
    pbest = pop.where(improved_2d, pbest);
    pbest_fit = fit.where(improved, pbest_fit);

    if (config.center_particle && evaluations < max_evaluations) {
      const Series center = pop.mean(0, false);
      const Series center_val = eval_candidate(objective, center, setup.original_shape, setup.work_cfg, op);
      ++evaluations;
      const double center_best = scalar_as_f64(center_val, op);
      if (center_best < gbest_val) {
        gbest = center;
        gbest_val = center_best;
      }
    }

    g_idx = argmin_index(pbest_fit);
    const double current_best = scalar_as_f64(row_by_index(pbest_fit.reshape({n_pop, 1}), g_idx), op);
    if (current_best < gbest_val) {
      gbest = row_by_index(pbest, g_idx);
      gbest_val = current_best;
    }

    last_step = "generation";
    maybe_print_iteration(config.display, iterations, evaluations, gbest_val, last_step);

    if (converged_by_relative_improvement(
            iterations, config.check_every, gbest_val, &check_best,
            config.relative_improvement_tolerance)) {
      converged = true;
      break;
    }
  }

  const PopulationSolverStatus status = converged
      ? PopulationSolverStatus::Converged
      : ((evaluations >= max_evaluations)
             ? PopulationSolverStatus::ReachedEvaluationBudget
             : PopulationSolverStatus::ReachedIterationBudget);

  return build_outcome(gbest, gbest_val, setup, iterations, evaluations, last_step, status);
}

PopulationSolverOutcome particle_swarm_dv_minimizer(
    const std::function<Series(const Series&)>& objective,
    const Series& initial,
    const ParticleSwarmDvConfig& config) {
  constexpr const char* op = "particle_swarm_dv_minimizer";
  require_valid_initial(initial, op);
  if (config.population_size < 4) {
    throw std::runtime_error(std::string(op) + ": population_size must be >= 4");
  }

  const CommonSetup setup = make_common_setup(initial, config.promote_cpu_to_float64);
  const BoundsPack bounds = prepare_bounds(config.lower_bounds, config.upper_bounds, setup, op);

  const std::int64_t n_pop = config.population_size;
  const std::int64_t n_dim = setup.dimension;
  const std::int64_t max_generations = (config.max_generations > 0) ? config.max_generations : 1000;

  std::int64_t max_evaluations = config.max_evaluations;
  if (max_evaluations <= 0) {
    max_evaluations = n_pop * (max_generations + 1);
  }

  RngState rng{config.rng_seed, 0ULL};

  Series pop = sample_population(setup.flat_initial, n_pop, config.initial_radius, bounds, setup.work_cfg, &rng);
  Series vel = pluss::zeros({n_pop, n_dim}, setup.work_cfg);

  std::int64_t evaluations = 0;
  Series fit = eval_population(objective, pop, setup.original_shape, setup.work_cfg, setup.idx_cfg, &evaluations, op);

  std::int64_t best_idx = argmin_index(fit);
  Series best = row_by_index(pop, best_idx);
  double best_val = scalar_as_f64(row_by_index(fit.reshape({n_pop, 1}), best_idx), op);
  double check_best = best_val;

  Series stagnation = pluss::zeros({n_pop}, setup.idx_cfg);

  std::int64_t iterations = 0;
  std::string last_step = "initial_population";
  maybe_print_iteration(config.display, iterations, evaluations, best_val, last_step);

  bool converged = false;
  double inertia = config.inertia;

  while (iterations < max_generations && evaluations < max_evaluations) {
    ++iterations;

    const Series pmax = pop.amax(0, false).reshape({1, n_dim}).expand(std::vector<std::int64_t>{n_pop, n_dim});
    const Series pmin = pop.amin(0, false).reshape({1, n_dim}).expand(std::vector<std::int64_t>{n_pop, n_dim});

    const Series p1 = pop.index_select(0, random_permutation(n_pop, setup.work_cfg, &rng));
    const Series p2 = pop.index_select(0, random_permutation(n_pop, setup.work_cfg, &rng));
    const Series delta = p1.sub(p2);

    const Series rand_u = pluss::rand({n_pop, n_dim}, setup.work_cfg, rng.next());
    const Series best_expand = best.reshape({1, n_dim}).expand(std::vector<std::int64_t>{n_pop, n_dim});

    const Series v_prop = vel.mul(inertia)
        .add(delta.mul(config.beta))
        .add(rand_u.mul(config.social).mul(best_expand.sub(pop)));

    const Series apply_mask = pluss::rand({n_pop, n_dim}, setup.work_cfg, rng.next()).le(config.crossover_rate);
    vel = v_prop.where(apply_mask, vel);

    Series trial = pop.add(vel);
    trial = apply_bounds_matrix(trial, bounds, n_pop, n_dim);

    const Series trial_fit = eval_population(objective, trial, setup.original_shape, setup.work_cfg, setup.idx_cfg, &evaluations, op);
    const Series improved = trial_fit.lt(fit);
    const Series improved_2d = improved.reshape({n_pop, 1}).expand(std::vector<std::int64_t>{n_pop, n_dim});

    pop = trial.where(improved_2d, pop);
    fit = trial_fit.where(improved, fit);

    const Series stagnation_inc = stagnation.add(1);
    stagnation = pluss::zeros({n_pop}, setup.idx_cfg).where(improved, stagnation_inc);

    const Series reset = stagnation.ge(config.stagnation_limit);
    const auto reset_host = reset.to(pluss::pCPU, ScalarType::Bool).to_vec<bool>();
    if (std::any_of(reset_host.begin(), reset_host.end(), [](bool v) { return v; })) {
      const Series reseed = pmin.add(pluss::rand({n_pop, n_dim}, setup.work_cfg, rng.next()).mul(pmax.sub(pmin)));
      const Series reset_2d = reset.reshape({n_pop, 1}).expand(std::vector<std::int64_t>{n_pop, n_dim});
      pop = reseed.where(reset_2d, pop);
      stagnation = pluss::zeros({n_pop}, setup.idx_cfg).where(reset, stagnation);

      for (std::int64_t i = 0; i < n_pop; ++i) {
        if (!reset_host[static_cast<std::size_t>(i)]) {
          continue;
        }
        if (evaluations >= max_evaluations) {
          break;
        }
        const Series val = eval_candidate(objective, row_by_index(pop, i), setup.original_shape, setup.work_cfg, op);
        const Series idx = pluss::arange(i, i + 1, setup.idx_cfg);
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
            config.relative_improvement_tolerance)) {
      converged = true;
      break;
    }
  }

  const PopulationSolverStatus status = converged
      ? PopulationSolverStatus::Converged
      : ((evaluations >= max_evaluations)
             ? PopulationSolverStatus::ReachedEvaluationBudget
             : PopulationSolverStatus::ReachedIterationBudget);

  return build_outcome(best, best_val, setup, iterations, evaluations, last_step, status);
}

} // namespace pluss::subsp::math::optimization
