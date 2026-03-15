#include "pluss/subsp/math/finance/rates.h"

#include "CodeM/core/Errors.h"
#include "pluss/codem/series_factory.h"
#include "pluss/subsp/stats/rng.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <string>
#include <vector>

namespace pluss::subsp::math::finance::rates {
namespace {

using pluss::Config;
using pluss::ScalarType;
using pluss::Series;
using pluss::subsp::stats::rng::HaltonOptions;
using pluss::subsp::stats::rng::Method;
using pluss::subsp::stats::rng::SobolOptions;
using pluss::subsp::stats::rng::TorusOptions;
using volatility::RandMethod;

constexpr double kPsiCut = 1.5;
constexpr double kEps = 1e-12;

Config f64_cfg_(Config cfg) {
  return Config{}.with_dtype(ScalarType::Float64).with_platform(cfg.platform).with_pinned(cfg.pinned);
}

Config i64_cfg_(Config cfg) {
  return Config{}.with_dtype(ScalarType::Int64).with_platform(cfg.platform).with_pinned(cfg.pinned);
}

Series row_index_(std::int64_t i, Config cfg) {
  return pluss::make_series(std::vector<std::int64_t>{i}, {1}, i64_cfg_(cfg));
}

std::uint64_t effective_seed_(const SimOptions& options, std::uint64_t compat_seed, std::uint64_t bump = 0U) {
  const std::uint64_t base = (compat_seed != 0U) ? compat_seed : options.seed;
  if (base == 0U) {
    return bump;
  }
  return base ^ (0x9E3779B97F4A7C15ULL + bump + (base << 6U) + (base >> 2U));
}

std::uint64_t effective_offset_(const SimOptions& options, std::uint64_t bump = 0U) {
  if (!options.randomized_quasi) {
    return options.offset + bump;
  }
  const std::uint64_t seed = (options.seed == 0U) ? (0xA5A5A5A5A5A5A5A5ULL + bump) : effective_seed_(options, options.seed, bump);
  return options.offset + (seed % 1000003ULL);
}

Method to_rng_method_(RandMethod method) {
  switch (method) {
    case RandMethod::Pseudo:
      return Method::Pseudo;
    case RandMethod::Sobol:
      return Method::Sobol;
    case RandMethod::Halton:
      return Method::Halton;
    case RandMethod::Torus:
      return Method::Torus;
  }
  return Method::Pseudo;
}

Series draw_normals_(std::int64_t rows,
                     std::int64_t cols,
                     const SimOptions& options,
                     Config cfg,
                     std::uint64_t compat_seed,
                     std::uint64_t bump) {
  const auto out_cfg = f64_cfg_(cfg);

  if (options.method == RandMethod::Pseudo) {
    return pluss::randn({rows, cols}, out_cfg, effective_seed_(options, compat_seed, bump));
  }

  const std::uint64_t off = effective_offset_(options, bump);
  const std::uint64_t seed = effective_seed_(options, compat_seed, bump);

  switch (options.method) {
    case RandMethod::Sobol: {
      SobolOptions o;
      o.init = true;
      o.scrambling = std::clamp(options.sobol_scrambling, 0, 3);
      o.mixed = options.randomized_quasi;
      o.normal = true;
      o.start = off + 1U;
      o.seed = seed;
      o.seed_set = true;
      return pluss::subsp::stats::rng::sobol({rows, cols}, o, out_cfg);
    }
    case RandMethod::Halton: {
      HaltonOptions o;
      o.init = true;
      o.normal = true;
      o.mixed = options.randomized_quasi;
      o.start = off + 1U;
      o.seed = seed;
      o.seed_set = true;
      return pluss::subsp::stats::rng::halton({rows, cols}, o, out_cfg);
    }
    case RandMethod::Torus: {
      TorusOptions o;
      o.init = true;
      o.normal = true;
      o.mixed = options.randomized_quasi;
      o.start = off + 1U;
      o.seed = seed;
      o.seed_set = true;
      return pluss::subsp::stats::rng::torus({rows, cols}, o, out_cfg);
    }
    case RandMethod::Pseudo:
      break;
  }

  return pluss::subsp::stats::rng::standard_normal(
      {rows, cols}, to_rng_method_(options.method), out_cfg, off);
}

Series draw_uniforms_(std::int64_t rows,
                      std::int64_t cols,
                      const SimOptions& options,
                      Config cfg,
                      std::uint64_t compat_seed,
                      std::uint64_t bump) {
  const auto out_cfg = f64_cfg_(cfg);

  if (options.method == RandMethod::Pseudo) {
    return pluss::rand({rows, cols}, out_cfg, effective_seed_(options, compat_seed, bump));
  }

  const std::uint64_t off = effective_offset_(options, bump);
  const std::uint64_t seed = effective_seed_(options, compat_seed, bump);

  switch (options.method) {
    case RandMethod::Sobol: {
      SobolOptions o;
      o.init = true;
      o.scrambling = std::clamp(options.sobol_scrambling, 0, 3);
      o.mixed = options.randomized_quasi;
      o.normal = false;
      o.start = off + 1U;
      o.seed = seed;
      o.seed_set = true;
      return pluss::subsp::stats::rng::sobol({rows, cols}, o, out_cfg);
    }
    case RandMethod::Halton: {
      HaltonOptions o;
      o.init = true;
      o.normal = false;
      o.mixed = options.randomized_quasi;
      o.start = off + 1U;
      o.seed = seed;
      o.seed_set = true;
      return pluss::subsp::stats::rng::halton({rows, cols}, o, out_cfg);
    }
    case RandMethod::Torus: {
      TorusOptions o;
      o.init = true;
      o.normal = false;
      o.mixed = options.randomized_quasi;
      o.start = off + 1U;
      o.seed = seed;
      o.seed_set = true;
      return pluss::subsp::stats::rng::torus({rows, cols}, o, out_cfg);
    }
    case RandMethod::Pseudo:
      break;
  }

  return pluss::subsp::stats::rng::uniform({rows, cols}, to_rng_method_(options.method), out_cfg, off);
}

void validate_common_(std::int64_t steps, std::int64_t paths, double dt, const char* op) {
  PLUSS_CHECK(steps > 0, std::string("subsp::math::finance::rates::") + op + ": steps must be > 0");
  PLUSS_CHECK(paths > 0, std::string("subsp::math::finance::rates::") + op + ": paths must be > 0");
  PLUSS_CHECK(std::isfinite(dt) && dt > 0.0,
              std::string("subsp::math::finance::rates::") + op + ": dt must be finite and > 0");
}

void validate_options_(const SimOptions& options, const char* op) {
  PLUSS_CHECK(options.sobol_scrambling >= 0 && options.sobol_scrambling <= 3,
              std::string("subsp::math::finance::rates::") + op + ": sobol_scrambling must be in [0,3]");
  if (options.method == RandMethod::Pseudo) {
    PLUSS_CHECK(!options.randomized_quasi,
                std::string("subsp::math::finance::rates::") + op +
                    ": randomized_quasi requires Sobol/Halton/Torus method");
  }
}

void validate_vasicek_(const VasicekParams& p, const char* op) {
  PLUSS_CHECK(std::isfinite(p.kappa) && p.kappa > 0.0,
              std::string("subsp::math::finance::rates::") + op + ": kappa must be finite and > 0");
  PLUSS_CHECK(std::isfinite(p.theta),
              std::string("subsp::math::finance::rates::") + op + ": theta must be finite");
  PLUSS_CHECK(std::isfinite(p.sigma) && p.sigma > 0.0,
              std::string("subsp::math::finance::rates::") + op + ": sigma must be finite and > 0");
  PLUSS_CHECK(std::isfinite(p.r0), std::string("subsp::math::finance::rates::") + op + ": r0 must be finite");
}

void validate_cir_(const CirParams& p, const char* op) {
  PLUSS_CHECK(std::isfinite(p.kappa) && p.kappa > 0.0,
              std::string("subsp::math::finance::rates::") + op + ": kappa must be finite and > 0");
  PLUSS_CHECK(std::isfinite(p.theta) && p.theta > 0.0,
              std::string("subsp::math::finance::rates::") + op + ": theta must be finite and > 0");
  PLUSS_CHECK(std::isfinite(p.sigma) && p.sigma > 0.0,
              std::string("subsp::math::finance::rates::") + op + ": sigma must be finite and > 0");
  PLUSS_CHECK(std::isfinite(p.r0) && p.r0 >= p.rate_floor,
              std::string("subsp::math::finance::rates::") + op + ": r0 must be finite and >= rate_floor");
  PLUSS_CHECK(std::isfinite(p.rate_floor) && p.rate_floor > 0.0,
              std::string("subsp::math::finance::rates::") + op + ": rate_floor must be finite and > 0");
}

void validate_hull_white_(const HullWhiteParams& p, const char* op) {
  PLUSS_CHECK(std::isfinite(p.kappa) && p.kappa > 0.0,
              std::string("subsp::math::finance::rates::") + op + ": kappa must be finite and > 0");
  PLUSS_CHECK(std::isfinite(p.sigma) && p.sigma > 0.0,
              std::string("subsp::math::finance::rates::") + op + ": sigma must be finite and > 0");
  PLUSS_CHECK(std::isfinite(p.r0), std::string("subsp::math::finance::rates::") + op + ": r0 must be finite");
  PLUSS_CHECK(std::isfinite(p.theta0), std::string("subsp::math::finance::rates::") + op + ": theta0 must be finite");
  PLUSS_CHECK(std::isfinite(p.theta_slope),
              std::string("subsp::math::finance::rates::") + op + ": theta_slope must be finite");
}

void validate_hjm_(const HjmParams& p, const char* op) {
  PLUSS_CHECK(std::isfinite(p.r0), std::string("subsp::math::finance::rates::") + op + ": r0 must be finite");
  PLUSS_CHECK(!p.tenors.empty(),
              std::string("subsp::math::finance::rates::") + op + ": tenors must be non-empty");
  PLUSS_CHECK(p.tenors.size() == p.vol.size(),
              std::string("subsp::math::finance::rates::") + op + ": tenors and vol size must match");
  for (std::size_t i = 0; i < p.tenors.size(); ++i) {
    PLUSS_CHECK(std::isfinite(p.tenors[i]) && p.tenors[i] > 0.0,
                std::string("subsp::math::finance::rates::") + op + ": tenors must be finite and > 0");
    PLUSS_CHECK(std::isfinite(p.vol[i]) && p.vol[i] >= 0.0,
                std::string("subsp::math::finance::rates::") + op + ": vol must be finite and >= 0");
    if (i > 0) {
      PLUSS_CHECK(p.tenors[i] > p.tenors[i - 1],
                  std::string("subsp::math::finance::rates::") + op + ": tenors must be strictly increasing");
    }
  }
}

Series cir_transition_step_(const Series& rt, const Series& z, const Series& u, double dt, const CirParams& p) {
  const Series r_safe = rt.clamp_min(p.rate_floor);
  const Config cfg = Config{}.with_dtype(r_safe.scalar_type()).with_platform(r_safe.platform());
  const Series exp_kdt = pluss::full(r_safe.sizes(), std::exp(-p.kappa * dt), cfg);
  const Series one_minus_exp = exp_kdt.mul(-1.0).add(1.0);

  const Series m = pluss::full(r_safe.sizes(), p.theta, cfg).add(r_safe.sub(p.theta).mul(exp_kdt));
  const Series s2 = r_safe.mul(p.sigma * p.sigma)
                        .mul(exp_kdt)
                        .mul(one_minus_exp)
                        .div(std::max(p.kappa, kEps))
                        .add(pluss::full(r_safe.sizes(), p.theta, cfg)
                                 .mul(p.sigma * p.sigma)
                                 .mul(one_minus_exp.square())
                                 .div(std::max(2.0 * p.kappa, kEps)));

  const Series psi = s2.div(m.square().clamp_min(kEps));
  const Series use_q1 = psi.le(kPsiCut);

  const Series two_over_psi = psi.clamp_min(kEps).reciprocal().mul(2.0);
  const Series b2 = two_over_psi.sub(1.0).add(two_over_psi.sqrt().mul(two_over_psi.sub(1.0).clamp_min(0.0).sqrt()));
  const Series a = m.div(b2.add(1.0).clamp_min(kEps));
  const Series r1 = a.mul(b2.clamp_min(0.0).sqrt().add(z).square());

  const Series p_zero = psi.sub(1.0).div(psi.add(1.0).clamp_min(kEps)).clamp_min(0.0).clamp_max(1.0 - kEps);
  const Series beta = p_zero.mul(-1.0).add(1.0).div(m.clamp_min(kEps));
  const Series u_safe = u.clamp_min(kEps).clamp_max(1.0 - kEps);
  const Series r2_nonzero = u_safe.sub(1.0).div(p_zero.sub(1.0).clamp_max(-kEps)).log().mul(-1.0).div(beta.clamp_min(kEps));
  const Series r2 = u_safe.le(p_zero).where(pluss::zeros(r_safe.sizes(), cfg), r2_nonzero);

  return use_q1.where(r1, r2).clamp_min(p.rate_floor);
}

std::vector<double> hjm_alpha_(const std::vector<double>& tenors, const std::vector<double>& vol) {
  const std::size_t n = tenors.size();
  std::vector<double> alpha(n, 0.0);
  for (std::size_t j = 0; j < n; ++j) {
    double tail = 0.0;
    const double base_t = (j == 0 ? 0.0 : tenors[j - 1]);
    for (std::size_t k = j; k < n; ++k) {
      const double prev_t = (k == 0 ? 0.0 : tenors[k - 1]);
      const double dtau = tenors[k] - std::max(prev_t, base_t);
      if (dtau > 0.0) {
        tail += vol[k] * dtau;
      }
    }
    alpha[j] = vol[j] * tail;
  }
  return alpha;
}

RatePaths finalize_rate_paths_(const Series& rate, const Series& innovation, const SimOptions& options, Config out_cfg) {
  Series rate_out = rate.to(out_cfg.platform, out_cfg.dtype);
  Series innov_out = innovation.to(out_cfg.platform, out_cfg.dtype);
  if (options.paths_first) {
    rate_out = rate_out.transpose(0, 1);
    innov_out = innov_out.transpose(0, 1);
  }
  return RatePaths{
      .rate = rate_out,
      .innovation = innov_out,
  };
}

HjmPaths finalize_hjm_paths_(const Series& rate,
                             const Series& forward,
                             const Series& innovation,
                             const SimOptions& options,
                             Config out_cfg) {
  Series rate_out = rate.to(out_cfg.platform, out_cfg.dtype);
  Series forward_out = forward.to(out_cfg.platform, out_cfg.dtype);
  Series innov_out = innovation.to(out_cfg.platform, out_cfg.dtype);
  if (options.paths_first) {
    rate_out = rate_out.transpose(0, 1);
    innov_out = innov_out.transpose(0, 1);
    const std::vector<std::int64_t> order{1, 0, 2};
    forward_out = forward_out.permute(order);
  }
  return HjmPaths{
      .rate = rate_out,
      .forward = forward_out,
      .innovation = innov_out,
  };
}

} // namespace

RatePaths vasicek_paths(std::int64_t steps,
                        std::int64_t paths,
                        double dt,
                        const VasicekParams& params,
                        const SimOptions& options,
                        Config cfg,
                        std::uint64_t seed) {
  validate_common_(steps, paths, dt, "vasicek_paths");
  validate_vasicek_(params, "vasicek_paths");
  validate_options_(options, "vasicek_paths");

  const auto out_cfg = f64_cfg_(cfg);
  const std::int64_t n = steps + 1;

  Series rate = pluss::full({n, paths}, params.r0, out_cfg);
  const Series z = draw_normals_(steps, paths, options, out_cfg, seed, 0x1619ULL);

  const double exp_kdt = std::exp(-params.kappa * dt);
  const double mean_scale = 1.0 - exp_kdt;
  const double var_scale = (params.sigma * params.sigma) * (1.0 - std::exp(-2.0 * params.kappa * dt)) /
                           std::max(2.0 * params.kappa, kEps);

  for (std::int64_t t = 0; t < steps; ++t) {
    const Series idx_cur = row_index_(t, out_cfg);
    const Series idx_next = row_index_(t + 1, out_cfg);
    const Series rt = rate.index_select(0, idx_cur).reshape({1, paths});
    const Series eps = z.index_select(0, idx_cur).reshape({1, paths});

    Series r_next;
    if (options.scheme == Scheme::Transition) {
      r_next = pluss::full({1, paths}, params.theta, out_cfg)
                   .add(rt.sub(params.theta).mul(exp_kdt))
                   .add(eps.mul(std::sqrt(std::max(var_scale, 0.0))));
    } else {
      r_next = rt.add(rt.mul(-params.kappa * dt))
                   .add(params.kappa * params.theta * dt)
                   .add(eps.mul(params.sigma * std::sqrt(dt)));
    }

    rate = pluss::index_put(rate, std::vector<Series>{idx_next}, r_next.reshape({paths}), false);
  }

  PLUSS_CHECK(rate.isfinite().all().item<double>() == 1.0,
              "subsp::math::finance::rates::vasicek_paths: rate path produced non-finite values");

  return finalize_rate_paths_(rate, z, options, out_cfg);
}

RatePaths vasicek_paths(std::int64_t steps,
                        std::int64_t paths,
                        double dt,
                        const VasicekParams& params,
                        Config cfg,
                        std::uint64_t seed) {
  SimOptions options;
  options.seed = seed;
  return vasicek_paths(steps, paths, dt, params, options, cfg, seed);
}

RatePaths cir_paths(std::int64_t steps,
                    std::int64_t paths,
                    double dt,
                    const CirParams& params,
                    const SimOptions& options,
                    Config cfg,
                    std::uint64_t seed) {
  validate_common_(steps, paths, dt, "cir_paths");
  validate_cir_(params, "cir_paths");
  validate_options_(options, "cir_paths");

  const auto out_cfg = f64_cfg_(cfg);
  const std::int64_t n = steps + 1;

  Series rate = pluss::full({n, paths}, std::max(params.r0, params.rate_floor), out_cfg);
  const Series z = draw_normals_(steps, paths, options, out_cfg, seed, 0x2713ULL);
  const Series u = (options.scheme == Scheme::Transition)
                       ? draw_uniforms_(steps, paths, options, out_cfg, seed, 0x3521ULL)
                       : pluss::zeros({steps, paths}, out_cfg);

  for (std::int64_t t = 0; t < steps; ++t) {
    const Series idx_cur = row_index_(t, out_cfg);
    const Series idx_next = row_index_(t + 1, out_cfg);
    const Series rt = rate.index_select(0, idx_cur).reshape({1, paths});
    const Series eps = z.index_select(0, idx_cur).reshape({1, paths});

    Series r_next;
    if (options.scheme == Scheme::Transition) {
      const Series uq = u.index_select(0, idx_cur).reshape({1, paths});
      r_next = cir_transition_step_(rt, eps, uq, dt, params);
    } else if (options.scheme == Scheme::Milstein) {
      const Series dW = eps.mul(std::sqrt(dt));
      const Series r_safe = rt.clamp_min(params.rate_floor);
      r_next = rt.add(rt.mul(-params.kappa * dt))
                   .add(params.kappa * params.theta * dt)
                   .add(r_safe.sqrt().mul(params.sigma).mul(dW))
                   .add(dW.square().sub(dt).mul(0.25 * params.sigma * params.sigma))
                   .clamp_min(params.rate_floor);
    } else {
      const Series r_safe = rt.clamp_min(params.rate_floor);
      r_next = rt.add(rt.mul(-params.kappa * dt))
                   .add(params.kappa * params.theta * dt)
                   .add(r_safe.sqrt().mul(params.sigma * std::sqrt(dt)).mul(eps))
                   .clamp_min(params.rate_floor);
    }

    rate = pluss::index_put(rate, std::vector<Series>{idx_next}, r_next.reshape({paths}), false);
  }

  PLUSS_CHECK(rate.isfinite().all().item<double>() == 1.0,
              "subsp::math::finance::rates::cir_paths: rate path produced non-finite values");

  return finalize_rate_paths_(rate, z, options, out_cfg);
}

RatePaths cir_paths(std::int64_t steps,
                    std::int64_t paths,
                    double dt,
                    const CirParams& params,
                    Config cfg,
                    std::uint64_t seed) {
  SimOptions options;
  options.seed = seed;
  return cir_paths(steps, paths, dt, params, options, cfg, seed);
}

RatePaths hull_white_paths(std::int64_t steps,
                           std::int64_t paths,
                           double dt,
                           const HullWhiteParams& params,
                           const SimOptions& options,
                           Config cfg,
                           std::uint64_t seed) {
  validate_common_(steps, paths, dt, "hull_white_paths");
  validate_hull_white_(params, "hull_white_paths");
  validate_options_(options, "hull_white_paths");

  const auto out_cfg = f64_cfg_(cfg);
  const std::int64_t n = steps + 1;
  Series rate = pluss::full({n, paths}, params.r0, out_cfg);
  const Series z = draw_normals_(steps, paths, options, out_cfg, seed, 0x4131ULL);

  const double exp_kdt = std::exp(-params.kappa * dt);
  const double var_scale = (params.sigma * params.sigma) * (1.0 - std::exp(-2.0 * params.kappa * dt)) /
                           std::max(2.0 * params.kappa, kEps);

  for (std::int64_t t = 0; t < steps; ++t) {
    const Series idx_cur = row_index_(t, out_cfg);
    const Series idx_next = row_index_(t + 1, out_cfg);
    const Series rt = rate.index_select(0, idx_cur).reshape({1, paths});
    const Series eps = z.index_select(0, idx_cur).reshape({1, paths});
    const double time_t = static_cast<double>(t) * dt;
    const double theta_t = params.theta0 + params.theta_slope * time_t;

    Series r_next;
    if (options.scheme == Scheme::Transition) {
      r_next = pluss::full({1, paths}, theta_t, out_cfg)
                   .add(rt.sub(theta_t).mul(exp_kdt))
                   .add(eps.mul(std::sqrt(std::max(var_scale, 0.0))));
    } else {
      r_next = rt.add(rt.mul(-params.kappa * dt))
                   .add(theta_t * dt)
                   .add(eps.mul(params.sigma * std::sqrt(dt)));
    }
    rate = pluss::index_put(rate, std::vector<Series>{idx_next}, r_next.reshape({paths}), false);
  }

  PLUSS_CHECK(rate.isfinite().all().item<double>() == 1.0,
              "subsp::math::finance::rates::hull_white_paths: rate path produced non-finite values");
  return finalize_rate_paths_(rate, z, options, out_cfg);
}

RatePaths hull_white_paths(std::int64_t steps,
                           std::int64_t paths,
                           double dt,
                           const HullWhiteParams& params,
                           Config cfg,
                           std::uint64_t seed) {
  SimOptions options;
  options.seed = seed;
  return hull_white_paths(steps, paths, dt, params, options, cfg, seed);
}

HjmPaths hjm_paths(std::int64_t steps,
                   std::int64_t paths,
                   double dt,
                   const HjmParams& params,
                   const SimOptions& options,
                   Config cfg,
                   std::uint64_t seed) {
  validate_common_(steps, paths, dt, "hjm_paths");
  validate_hjm_(params, "hjm_paths");
  validate_options_(options, "hjm_paths");

  const auto out_cfg = f64_cfg_(cfg);
  const std::int64_t n = steps + 1;
  const std::int64_t k = static_cast<std::int64_t>(params.tenors.size());
  const std::vector<double> alpha = hjm_alpha_(params.tenors, params.vol);

  Series rate = pluss::full({n, paths}, params.r0, out_cfg);
  Series forward = pluss::full({n, paths, k}, params.r0, out_cfg);
  const Series z = draw_normals_(steps, paths, options, out_cfg, seed, 0x5719ULL);

  Series f_state = pluss::full({paths, k}, params.r0, out_cfg);
  const Series alpha_row = pluss::make_series(alpha, {1, k}, out_cfg).mul(dt);
  const Series vol_row = pluss::make_series(params.vol, {1, k}, out_cfg).mul(std::sqrt(dt));

  for (std::int64_t t = 0; t < steps; ++t) {
    const Series idx_cur = row_index_(t, out_cfg);
    const Series idx_next = row_index_(t + 1, out_cfg);
    const Series eps = z.index_select(0, idx_cur).reshape({paths, 1});
    f_state = f_state.add(alpha_row).add(eps.mul(vol_row));

    forward = pluss::index_put(forward, std::vector<Series>{idx_next}, f_state, false);
    rate = pluss::index_put(rate, std::vector<Series>{idx_next}, f_state.select(1, 0).reshape({paths}), false);
  }

  PLUSS_CHECK(rate.isfinite().all().item<double>() == 1.0,
              "subsp::math::finance::rates::hjm_paths: rate path produced non-finite values");
  PLUSS_CHECK(forward.isfinite().all().item<double>() == 1.0,
              "subsp::math::finance::rates::hjm_paths: forward path produced non-finite values");

  return finalize_hjm_paths_(rate, forward, z, options, out_cfg);
}

HjmPaths hjm_paths(std::int64_t steps,
                   std::int64_t paths,
                   double dt,
                   const HjmParams& params,
                   Config cfg,
                   std::uint64_t seed) {
  SimOptions options;
  options.seed = seed;
  return hjm_paths(steps, paths, dt, params, options, cfg, seed);
}

double vasicek_zcb(double maturity, double short_rate, const VasicekParams& params) {
  PLUSS_CHECK(std::isfinite(maturity) && maturity >= 0.0,
              "subsp::math::finance::rates::vasicek_zcb: maturity must be finite and >= 0");
  validate_vasicek_(params, "vasicek_zcb");
  PLUSS_CHECK(std::isfinite(short_rate),
              "subsp::math::finance::rates::vasicek_zcb: short_rate must be finite");

  if (maturity <= 0.0) {
    return 1.0;
  }

  const double k = params.kappa;
  const double s2 = params.sigma * params.sigma;
  const double b = (1.0 - std::exp(-k * maturity)) / k;
  const double a = std::exp((params.theta - (s2 / (2.0 * k * k))) * (b - maturity) - (s2 * b * b) / (4.0 * k));
  return a * std::exp(-b * short_rate);
}

double cir_zcb(double maturity, double short_rate, const CirParams& params) {
  PLUSS_CHECK(std::isfinite(maturity) && maturity >= 0.0,
              "subsp::math::finance::rates::cir_zcb: maturity must be finite and >= 0");
  validate_cir_(params, "cir_zcb");
  PLUSS_CHECK(std::isfinite(short_rate) && short_rate >= params.rate_floor,
              "subsp::math::finance::rates::cir_zcb: short_rate must be finite and >= rate_floor");

  if (maturity <= 0.0) {
    return 1.0;
  }

  const double gamma = std::sqrt(params.kappa * params.kappa + 2.0 * params.sigma * params.sigma);
  const double exp_gt = std::exp(gamma * maturity);
  const double den = (gamma + params.kappa) * (exp_gt - 1.0) + 2.0 * gamma;
  const double b = 2.0 * (exp_gt - 1.0) / den;
  const double pow_term = (2.0 * gamma * std::exp(0.5 * (params.kappa + gamma) * maturity)) / den;
  const double a = std::pow(pow_term, (2.0 * params.kappa * params.theta) / (params.sigma * params.sigma));
  return a * std::exp(-b * short_rate);
}

double hull_white_zcb(double maturity, double short_rate, const HullWhiteParams& params) {
  PLUSS_CHECK(std::isfinite(maturity) && maturity >= 0.0,
              "subsp::math::finance::rates::hull_white_zcb: maturity must be finite and >= 0");
  validate_hull_white_(params, "hull_white_zcb");
  PLUSS_CHECK(std::isfinite(short_rate),
              "subsp::math::finance::rates::hull_white_zcb: short_rate must be finite");

  if (maturity <= 0.0) {
    return 1.0;
  }

  const VasicekParams approx{
      .kappa = params.kappa,
      .theta = params.theta0,
      .sigma = params.sigma,
      .r0 = params.r0,
  };
  return vasicek_zcb(maturity, short_rate, approx);
}

double hjm_zcb_mc(double maturity,
                  const HjmParams& params,
                  std::int64_t paths,
                  std::int64_t steps,
                  SimOptions options,
                  Config cfg) {
  PLUSS_CHECK(std::isfinite(maturity) && maturity > 0.0,
              "subsp::math::finance::rates::hjm_zcb_mc: maturity must be finite and > 0");
  PLUSS_CHECK(paths > 0 && steps > 0, "subsp::math::finance::rates::hjm_zcb_mc: paths/steps must be > 0");
  validate_hjm_(params, "hjm_zcb_mc");
  const double dt = maturity / static_cast<double>(steps);
  const auto sim = hjm_paths(steps, paths, dt, params, options, cfg, options.seed);
  const Series integral = sim.rate.index_select(0, Series::arange(0, steps, 1, ScalarType::Int64)).sum(0, false).mul(dt);
  return integral.mul(-1.0).exp().mean().item<double>();
}

} // namespace pluss::subsp::math::finance::rates
