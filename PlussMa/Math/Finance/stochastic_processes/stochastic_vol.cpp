#include "pluss/subsp/math/finance/volatility.h"

#include "CodeM/core/Errors.h"
#include "pluss/codem/series_factory.h"
#include "pluss/subsp/stats/rng.h"

#include <cmath>
#include <cstdint>
#include <string>
#include <vector>

namespace pluss::subsp::math::finance::volatility {
namespace {

using pluss::Config;
using pluss::ScalarType;
using pluss::Series;
using volatility::BatesParams;
using volatility::HestonNandiParams;
using volatility::SabrParams;
using volatility::SlvParams;
using pluss::subsp::stats::rng::HaltonOptions;
using pluss::subsp::stats::rng::Method;
using pluss::subsp::stats::rng::SobolOptions;
using pluss::subsp::stats::rng::TorusOptions;

constexpr double kPsiCut = 1.5;
constexpr double kEps = 1e-12;

Config f64_cfg_(Config cfg) {
  return Config{}.with_dtype(ScalarType::Float64).with_platform(cfg.platform).with_pinned(cfg.pinned);
}

Config i64_cfg_(Config cfg) {
  return Config{}.with_dtype(ScalarType::Int64).with_platform(cfg.platform).with_pinned(cfg.pinned);
}

Config cfg_like_(const Series& x) {
  return Config{}.with_dtype(x.scalar_type()).with_platform(x.platform());
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
  const std::int64_t base_cols = options.antithetic ? ((cols + 1) / 2) : cols;
  const auto out_cfg = f64_cfg_(cfg);

  if (options.method == RandMethod::Pseudo) {
    Series z = pluss::randn({rows, base_cols}, out_cfg, effective_seed_(options, compat_seed, bump));
    if (options.antithetic) {
      const Series mirror = z.mul(-1.0);
      const std::int64_t need = cols - base_cols;
      if (need > 0) {
        const Series idx = Series::arange(0, need, 1, ScalarType::Int64).to(i64_cfg_(out_cfg).platform, i64_cfg_(out_cfg).dtype);
        z = z.unite({mirror.index_select(1, idx)}, 1);
      } else {
        z = z.unite({mirror}, 1);
      }
    }
    return (std::abs(options.normal_shift) > 0.0) ? z.add(options.normal_shift) : z;
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
      Series z = pluss::subsp::stats::rng::sobol({rows, base_cols}, o, out_cfg);
      if (options.antithetic) {
        const Series mirror = z.mul(-1.0);
        const std::int64_t need = cols - base_cols;
        if (need > 0) {
          const Series idx = Series::arange(0, need, 1, ScalarType::Int64).to(i64_cfg_(out_cfg).platform, i64_cfg_(out_cfg).dtype);
          z = z.unite({mirror.index_select(1, idx)}, 1);
        } else {
          z = z.unite({mirror}, 1);
        }
      }
      return (std::abs(options.normal_shift) > 0.0) ? z.add(options.normal_shift) : z;
    }
    case RandMethod::Halton: {
      HaltonOptions o;
      o.init = true;
      o.normal = true;
      o.mixed = options.randomized_quasi;
      o.start = off + 1U;
      o.seed = seed;
      o.seed_set = true;
      Series z = pluss::subsp::stats::rng::halton({rows, base_cols}, o, out_cfg);
      if (options.antithetic) {
        const Series mirror = z.mul(-1.0);
        const std::int64_t need = cols - base_cols;
        if (need > 0) {
          const Series idx = Series::arange(0, need, 1, ScalarType::Int64).to(i64_cfg_(out_cfg).platform, i64_cfg_(out_cfg).dtype);
          z = z.unite({mirror.index_select(1, idx)}, 1);
        } else {
          z = z.unite({mirror}, 1);
        }
      }
      return (std::abs(options.normal_shift) > 0.0) ? z.add(options.normal_shift) : z;
    }
    case RandMethod::Torus: {
      TorusOptions o;
      o.init = true;
      o.normal = true;
      o.mixed = options.randomized_quasi;
      o.start = off + 1U;
      o.seed = seed;
      o.seed_set = true;
      Series z = pluss::subsp::stats::rng::torus({rows, base_cols}, o, out_cfg);
      if (options.antithetic) {
        const Series mirror = z.mul(-1.0);
        const std::int64_t need = cols - base_cols;
        if (need > 0) {
          const Series idx = Series::arange(0, need, 1, ScalarType::Int64).to(i64_cfg_(out_cfg).platform, i64_cfg_(out_cfg).dtype);
          z = z.unite({mirror.index_select(1, idx)}, 1);
        } else {
          z = z.unite({mirror}, 1);
        }
      }
      return (std::abs(options.normal_shift) > 0.0) ? z.add(options.normal_shift) : z;
    }
    case RandMethod::Pseudo:
      break;
  }

  Series z = pluss::subsp::stats::rng::standard_normal(
      {rows, base_cols}, to_rng_method_(options.method), out_cfg, off);
  if (options.antithetic) {
    const Series mirror = z.mul(-1.0);
    const std::int64_t need = cols - base_cols;
    if (need > 0) {
      const Series idx = Series::arange(0, need, 1, ScalarType::Int64).to(i64_cfg_(out_cfg).platform, i64_cfg_(out_cfg).dtype);
      z = z.unite({mirror.index_select(1, idx)}, 1);
    } else {
      z = z.unite({mirror}, 1);
    }
  }
  return (std::abs(options.normal_shift) > 0.0) ? z.add(options.normal_shift) : z;
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

void validate_common_(std::int64_t steps,
                      std::int64_t paths,
                      double spot0,
                      double dt,
                      double variance_floor,
                      const char* op) {
  PLUSS_CHECK(steps > 0, std::string("subsp::math::finance::volatility::") + op + ": steps must be > 0");
  PLUSS_CHECK(paths > 0, std::string("subsp::math::finance::volatility::") + op + ": paths must be > 0");
  PLUSS_CHECK(std::isfinite(spot0) && spot0 > 0.0,
              std::string("subsp::math::finance::volatility::") + op + ": spot0 must be finite and > 0");
  PLUSS_CHECK(std::isfinite(dt) && dt > 0.0,
              std::string("subsp::math::finance::volatility::") + op + ": dt must be finite and > 0");
  PLUSS_CHECK(std::isfinite(variance_floor) && variance_floor > 0.0,
              std::string("subsp::math::finance::volatility::") + op + ": variance_floor must be finite and > 0");
}

void validate_options_(const SimOptions& options, const char* op) {
  PLUSS_CHECK(options.sobol_scrambling >= 0 && options.sobol_scrambling <= 3,
              std::string("subsp::math::finance::volatility::") + op + ": sobol_scrambling must be in [0,3]");
  PLUSS_CHECK(std::isfinite(options.normal_shift),
              std::string("subsp::math::finance::volatility::") + op + ": normal_shift must be finite");
  if (options.method == RandMethod::Pseudo) {
    PLUSS_CHECK(!options.randomized_quasi,
                std::string("subsp::math::finance::volatility::") + op +
                    ": randomized_quasi requires Sobol/Halton/Torus method");
  }
}

void validate_heston_(const HestonParams& p, const char* op) {
  PLUSS_CHECK(std::isfinite(p.kappa) && p.kappa > 0.0,
              std::string("subsp::math::finance::volatility::") + op + ": kappa must be finite and > 0");
  PLUSS_CHECK(std::isfinite(p.theta) && p.theta > 0.0,
              std::string("subsp::math::finance::volatility::") + op + ": theta must be finite and > 0");
  PLUSS_CHECK(std::isfinite(p.sigma) && p.sigma > 0.0,
              std::string("subsp::math::finance::volatility::") + op + ": sigma must be finite and > 0");
  PLUSS_CHECK(std::isfinite(p.rho) && p.rho > -1.0 && p.rho < 1.0,
              std::string("subsp::math::finance::volatility::") + op + ": rho must be in (-1,1)");
  PLUSS_CHECK(std::isfinite(p.v0) && p.v0 > 0.0,
              std::string("subsp::math::finance::volatility::") + op + ": v0 must be finite and > 0");
  PLUSS_CHECK(std::isfinite(p.mu), std::string("subsp::math::finance::volatility::") + op + ": mu must be finite");
}

void validate_bates_(const BatesParams& p, const char* op) {
  PLUSS_CHECK(std::isfinite(p.kappa) && p.kappa > 0.0,
              std::string("subsp::math::finance::volatility::") + op + ": kappa must be finite and > 0");
  PLUSS_CHECK(std::isfinite(p.theta) && p.theta > 0.0,
              std::string("subsp::math::finance::volatility::") + op + ": theta must be finite and > 0");
  PLUSS_CHECK(std::isfinite(p.sigma) && p.sigma > 0.0,
              std::string("subsp::math::finance::volatility::") + op + ": sigma must be finite and > 0");
  PLUSS_CHECK(std::isfinite(p.rho) && p.rho > -1.0 && p.rho < 1.0,
              std::string("subsp::math::finance::volatility::") + op + ": rho must be in (-1,1)");
  PLUSS_CHECK(std::isfinite(p.v0) && p.v0 > 0.0,
              std::string("subsp::math::finance::volatility::") + op + ": v0 must be finite and > 0");
  PLUSS_CHECK(std::isfinite(p.mu), std::string("subsp::math::finance::volatility::") + op + ": mu must be finite");
  PLUSS_CHECK(std::isfinite(p.jump_intensity) && p.jump_intensity >= 0.0,
              std::string("subsp::math::finance::volatility::") + op + ": jump_intensity must be finite and >= 0");
  PLUSS_CHECK(std::isfinite(p.jump_mean),
              std::string("subsp::math::finance::volatility::") + op + ": jump_mean must be finite");
  PLUSS_CHECK(std::isfinite(p.jump_vol) && p.jump_vol >= 0.0,
              std::string("subsp::math::finance::volatility::") + op + ": jump_vol must be finite and >= 0");
}

void validate_sv_(const SvParams& p, const char* op) {
  PLUSS_CHECK(std::isfinite(p.mu), std::string("subsp::math::finance::volatility::") + op + ": mu must be finite");
  PLUSS_CHECK(std::isfinite(p.logvar_mean),
              std::string("subsp::math::finance::volatility::") + op + ": logvar_mean must be finite");
  PLUSS_CHECK(std::isfinite(p.phi) && std::abs(p.phi) < 1.0,
              std::string("subsp::math::finance::volatility::") + op + ": abs(phi) must be < 1");
  PLUSS_CHECK(std::isfinite(p.vol_of_logvar) && p.vol_of_logvar > 0.0,
              std::string("subsp::math::finance::volatility::") + op + ": vol_of_logvar must be finite and > 0");
  PLUSS_CHECK(std::isfinite(p.rho) && p.rho > -1.0 && p.rho < 1.0,
              std::string("subsp::math::finance::volatility::") + op + ": rho must be in (-1,1)");
  PLUSS_CHECK(std::isfinite(p.logvar0),
              std::string("subsp::math::finance::volatility::") + op + ": logvar0 must be finite");
}

void validate_heston_nandi_(const HestonNandiParams& p, const char* op) {
  PLUSS_CHECK(std::isfinite(p.omega) && p.omega > 0.0,
              std::string("subsp::math::finance::volatility::") + op + ": omega must be finite and > 0");
  PLUSS_CHECK(std::isfinite(p.alpha) && p.alpha >= 0.0,
              std::string("subsp::math::finance::volatility::") + op + ": alpha must be finite and >= 0");
  PLUSS_CHECK(std::isfinite(p.beta) && p.beta >= 0.0,
              std::string("subsp::math::finance::volatility::") + op + ": beta must be finite and >= 0");
  PLUSS_CHECK(std::isfinite(p.gamma), std::string("subsp::math::finance::volatility::") + op + ": gamma must be finite");
  PLUSS_CHECK(std::isfinite(p.lambda), std::string("subsp::math::finance::volatility::") + op + ": lambda must be finite");
  PLUSS_CHECK(std::isfinite(p.r), std::string("subsp::math::finance::volatility::") + op + ": r must be finite");
  PLUSS_CHECK(std::isfinite(p.variance_floor) && p.variance_floor > 0.0,
              std::string("subsp::math::finance::volatility::") + op + ": variance_floor must be finite and > 0");
  PLUSS_CHECK(std::isfinite(p.stationarity_margin) && p.stationarity_margin > 0.0 && p.stationarity_margin < 1.0,
              std::string("subsp::math::finance::volatility::") + op + ": stationarity_margin must be in (0,1)");
  PLUSS_CHECK(!std::isfinite(p.initial_variance) || p.initial_variance > 0.0,
              std::string("subsp::math::finance::volatility::") + op +
                  ": initial_variance must be NaN or finite and > 0");

  const double persistence = p.beta + p.alpha * p.gamma * p.gamma;
  PLUSS_CHECK(persistence < (1.0 - p.stationarity_margin),
              std::string("subsp::math::finance::volatility::") + op +
                  ": requires beta + alpha*gamma^2 < 1 - stationarity_margin");
}

double heston_nandi_unconditional_variance_(const HestonNandiParams& p, const char* op) {
  validate_heston_nandi_(p, op);
  const double persistence = p.beta + p.alpha * p.gamma * p.gamma;
  const double denom = 1.0 - persistence;
  PLUSS_CHECK(denom > 0.0,
              std::string("subsp::math::finance::volatility::") + op +
                  ": unconditional variance undefined (non-stationary persistence)");
  const double unc = (p.omega + p.alpha) / denom;
  PLUSS_CHECK(std::isfinite(unc) && unc > 0.0,
              std::string("subsp::math::finance::volatility::") + op + ": unconditional variance must be finite and > 0");
  return std::max(unc, p.variance_floor);
}

double heston_nandi_initial_variance_(const HestonNandiParams& p, const char* op) {
  if (std::isfinite(p.initial_variance)) {
    return std::max(p.initial_variance, p.variance_floor);
  }
  return heston_nandi_unconditional_variance_(p, op);
}

void validate_sabr_(const SabrParams& p, const char* op) {
  PLUSS_CHECK(std::isfinite(p.alpha0) && p.alpha0 > 0.0,
              std::string("subsp::math::finance::volatility::") + op + ": alpha0 must be finite and > 0");
  PLUSS_CHECK(std::isfinite(p.beta) && p.beta >= 0.0 && p.beta <= 1.0,
              std::string("subsp::math::finance::volatility::") + op + ": beta must be in [0,1]");
  PLUSS_CHECK(std::isfinite(p.nu) && p.nu >= 0.0,
              std::string("subsp::math::finance::volatility::") + op + ": nu must be finite and >= 0");
  PLUSS_CHECK(std::isfinite(p.rho) && p.rho > -1.0 && p.rho < 1.0,
              std::string("subsp::math::finance::volatility::") + op + ": rho must be in (-1,1)");
  PLUSS_CHECK(std::isfinite(p.mu), std::string("subsp::math::finance::volatility::") + op + ": mu must be finite");
  PLUSS_CHECK(std::isfinite(p.alpha_floor) && p.alpha_floor > 0.0,
              std::string("subsp::math::finance::volatility::") + op + ": alpha_floor must be finite and > 0");
}

void validate_slv_(const SlvParams& p, const char* op) {
  PLUSS_CHECK(std::isfinite(p.mu), std::string("subsp::math::finance::volatility::") + op + ": mu must be finite");
  PLUSS_CHECK(std::isfinite(p.kappa) && p.kappa > 0.0,
              std::string("subsp::math::finance::volatility::") + op + ": kappa must be finite and > 0");
  PLUSS_CHECK(std::isfinite(p.theta) && p.theta > 0.0,
              std::string("subsp::math::finance::volatility::") + op + ": theta must be finite and > 0");
  PLUSS_CHECK(std::isfinite(p.sigma) && p.sigma >= 0.0,
              std::string("subsp::math::finance::volatility::") + op + ": sigma must be finite and >= 0");
  PLUSS_CHECK(std::isfinite(p.rho) && p.rho > -1.0 && p.rho < 1.0,
              std::string("subsp::math::finance::volatility::") + op + ": rho must be in (-1,1)");
  PLUSS_CHECK(std::isfinite(p.v0) && p.v0 > 0.0,
              std::string("subsp::math::finance::volatility::") + op + ": v0 must be finite and > 0");
  PLUSS_CHECK(std::isfinite(p.local_vol_level) && p.local_vol_level >= 0.0,
              std::string("subsp::math::finance::volatility::") + op + ": local_vol_level must be finite and >= 0");
  PLUSS_CHECK(std::isfinite(p.local_vol_slope),
              std::string("subsp::math::finance::volatility::") + op + ": local_vol_slope must be finite");
  PLUSS_CHECK(std::isfinite(p.local_vol_curvature),
              std::string("subsp::math::finance::volatility::") + op + ": local_vol_curvature must be finite");
}

Series local_vol_(const Series& s, const SlvParams& p) {
  if (p.local_vol_form == LocalVolForm::Constant) {
    return pluss::full(s.sizes(), std::max(0.0, p.local_vol_level), cfg_like_(s));
  }
  if (p.local_vol_form == LocalVolForm::Cev) {
    return s.clamp_min(p.variance_floor).pow(p.local_vol_curvature).mul(p.local_vol_level).clamp_min(0.0);
  }
  return s.clamp_min(p.variance_floor)
      .log()
      .mul(p.local_vol_slope)
      .add(p.local_vol_curvature)
      .exp()
      .mul(std::max(0.0, p.local_vol_level))
      .clamp_min(0.0);
}

StochVolPaths finalize_paths_(const Series& spot,
                              const Series& variance,
                              const Series& innovation_spot,
                              const Series& innovation_var,
                              const SimOptions& options,
                              Config out_cfg) {
  Series spot_out = spot.to(out_cfg.platform, out_cfg.dtype);
  Series var_out = variance.to(out_cfg.platform, out_cfg.dtype);
  Series innov_spot_out = innovation_spot.to(out_cfg.platform, out_cfg.dtype);
  Series innov_var_out = innovation_var.to(out_cfg.platform, out_cfg.dtype);
  if (options.paths_first) {
    spot_out = spot_out.transpose(0, 1);
    var_out = var_out.transpose(0, 1);
    innov_spot_out = innov_spot_out.transpose(0, 1);
    innov_var_out = innov_var_out.transpose(0, 1);
  }
  return StochVolPaths{
      .spot = spot_out,
      .variance = var_out,
      .innovation_spot = innov_spot_out,
      .innovation_var = innov_var_out,
  };
}

HestonParams to_heston_(const BatesParams& p) {
  HestonParams h;
  h.kappa = p.kappa;
  h.theta = p.theta;
  h.sigma = p.sigma;
  h.rho = p.rho;
  h.v0 = p.v0;
  h.mu = p.mu;
  h.variance_floor = p.variance_floor;
  return h;
}

Series heston_variance_step_(const Series& vt,
                             const Series& z_var,
                             const Series& u_qe,
                             double dt,
                             const HestonParams& p,
                             SimScheme scheme) {
  const Series v_safe = vt.clamp_min(p.variance_floor);
  const Series sqrt_v = v_safe.sqrt();

  if (scheme == SimScheme::Euler) {
    return vt.add(vt.mul(-p.kappa * dt))
        .add(p.kappa * p.theta * dt)
        .add(sqrt_v.mul(p.sigma * std::sqrt(dt)).mul(z_var))
        .clamp_min(p.variance_floor);
  }

  if (scheme == SimScheme::Milstein || scheme == SimScheme::Milstein2) {
    const Series dW = z_var.mul(std::sqrt(dt));
    Series v_next = vt.add(vt.mul(-p.kappa * dt))
                        .add(p.kappa * p.theta * dt)
                        .add(sqrt_v.mul(p.sigma).mul(dW))
                        .add(dW.square().sub(dt).mul(0.25 * p.sigma * p.sigma));

    if (scheme == SimScheme::Milstein2) {
      v_next = v_next.add(sqrt_v.mul(0.5 * p.kappa * p.sigma * dt).mul(dW));
    }
    return v_next.clamp_min(p.variance_floor);
  }

  const Config vt_cfg = cfg_like_(vt);
  const Series exp_kdt = pluss::full(vt.sizes(), std::exp(-p.kappa * dt), vt_cfg);
  const Series one_minus_exp = exp_kdt.mul(-1.0).add(1.0);

  const Series m = pluss::full(vt.sizes(), p.theta, vt_cfg).add(vt.sub(p.theta).mul(exp_kdt));
  const Series s2 = vt.mul(p.sigma * p.sigma)
                        .mul(exp_kdt)
                        .mul(one_minus_exp)
                        .div(std::max(p.kappa, kEps))
                        .add(pluss::full(vt.sizes(), p.theta, vt_cfg)
                                 .mul(p.sigma * p.sigma)
                                 .mul(one_minus_exp.square())
                                 .div(std::max(2.0 * p.kappa, kEps)));

  const Series psi = s2.div(m.square().clamp_min(kEps));
  const Series use_q1 = psi.le(kPsiCut);

  const Series two_over_psi = psi.clamp_min(kEps).reciprocal().mul(2.0);
  const Series b2 = two_over_psi.sub(1.0).add(two_over_psi.sqrt().mul(two_over_psi.sub(1.0).clamp_min(0.0).sqrt()));
  const Series a = m.div(b2.add(1.0).clamp_min(kEps));
  const Series v1 = a.mul(b2.clamp_min(0.0).sqrt().add(z_var).square());

  const Series p_zero = psi.sub(1.0).div(psi.add(1.0).clamp_min(kEps)).clamp_min(0.0).clamp_max(1.0 - kEps);
  const Series beta = p_zero.mul(-1.0).add(1.0).div(m.clamp_min(kEps));
  const Series u = u_qe.clamp_min(kEps).clamp_max(1.0 - kEps);
  const Series v2_nonzero = u.sub(1.0).div(p_zero.sub(1.0).clamp_max(-kEps)).log().mul(-1.0).div(beta.clamp_min(kEps));
  const Series v2 = u.le(p_zero).where(pluss::zeros(vt.sizes(), vt_cfg), v2_nonzero);

  return use_q1.where(v1, v2).clamp_min(p.variance_floor);
}

} // namespace

StochVolPaths heston_paths(std::int64_t steps,
                           std::int64_t paths,
                           double spot0,
                           double dt,
                           const HestonParams& params,
                           const SimOptions& options,
                           Config cfg,
                           std::uint64_t seed) {
  validate_common_(steps, paths, spot0, dt, params.variance_floor, "heston_paths");
  validate_heston_(params, "heston_paths");
  validate_options_(options, "heston_paths");

  const auto out_cfg = f64_cfg_(cfg);
  const std::int64_t n = steps + 1;

  Series spot = pluss::full({n, paths}, spot0, out_cfg);
  Series variance = pluss::full({n, paths}, std::max(params.v0, params.variance_floor), out_cfg);

  const Series z_spot = draw_normals_(steps, paths, options, out_cfg, seed, 0x1021ULL);
  const Series z_aux = draw_normals_(steps, paths, options, out_cfg, seed, 0x2303ULL);
  const Series u_qe = draw_uniforms_(steps, paths, options, out_cfg, seed, 0x4517ULL);

  const double rho_scale = std::sqrt(std::max(0.0, 1.0 - params.rho * params.rho));
  const Series z_var = z_spot.mul(params.rho).add(z_aux.mul(rho_scale));

  for (std::int64_t t = 0; t < steps; ++t) {
    const Series idx_cur = row_index_(t, out_cfg);
    const Series idx_next = row_index_(t + 1, out_cfg);

    const Series vt = variance.index_select(0, idx_cur).reshape({1, paths});
    const Series st = spot.index_select(0, idx_cur).reshape({1, paths});
    const Series eps_s = z_spot.index_select(0, idx_cur).reshape({1, paths});
    const Series eps_v = z_var.index_select(0, idx_cur).reshape({1, paths});
    const Series uq = u_qe.index_select(0, idx_cur).reshape({1, paths});

    const Series v_next = heston_variance_step_(vt, eps_v, uq, dt, params, options.scheme);
    const Series v_bar = vt.add(v_next).mul(0.5).clamp_min(params.variance_floor);

    const Series log_inc = v_bar.mul(-0.5 * dt)
                               .add(params.mu * dt)
                               .add(v_bar.mul(dt).sqrt().mul(eps_s));
    const Series s_next = st.mul(log_inc.exp()).clamp_min(params.variance_floor).clamp_max(1.0e12);

    variance = pluss::index_put(variance, std::vector<Series>{idx_next}, v_next.reshape({paths}), false);
    spot = pluss::index_put(spot, std::vector<Series>{idx_next}, s_next.reshape({paths}), false);
  }

  PLUSS_CHECK(spot.isfinite().all().item<double>() == 1.0,
              "subsp::math::finance::volatility::heston_paths: spot path produced non-finite values");
  PLUSS_CHECK(variance.isfinite().all().item<double>() == 1.0,
              "subsp::math::finance::volatility::heston_paths: variance path produced non-finite values");

  return finalize_paths_(spot, variance, z_spot, z_var, options, out_cfg);
}

StochVolPaths heston_paths(std::int64_t steps,
                           std::int64_t paths,
                           double spot0,
                           double dt,
                           const HestonParams& params,
                           Config cfg,
                           std::uint64_t seed) {
  SimOptions options;
  options.seed = seed;
  return heston_paths(steps, paths, spot0, dt, params, options, cfg, seed);
}

StochVolPaths bates_paths(std::int64_t steps,
                          std::int64_t paths,
                          double spot0,
                          double dt,
                          const BatesParams& params,
                          const SimOptions& options,
                          Config cfg,
                          std::uint64_t seed) {
  validate_common_(steps, paths, spot0, dt, params.variance_floor, "bates_paths");
  validate_bates_(params, "bates_paths");
  validate_options_(options, "bates_paths");

  const auto out_cfg = f64_cfg_(cfg);
  const std::int64_t n = steps + 1;
  const HestonParams h_params = to_heston_(params);

  Series spot = pluss::full({n, paths}, spot0, out_cfg);
  Series variance = pluss::full({n, paths}, std::max(params.v0, params.variance_floor), out_cfg);

  const Series z_spot = draw_normals_(steps, paths, options, out_cfg, seed, 0xA129ULL);
  const Series z_aux = draw_normals_(steps, paths, options, out_cfg, seed, 0xB273ULL);
  const Series u_qe = draw_uniforms_(steps, paths, options, out_cfg, seed, 0xC317ULL);
  const Series z_jump = draw_normals_(steps, paths, options, out_cfg, seed, 0xD401ULL);
  const Series u_jump = draw_uniforms_(steps, paths, options, out_cfg, seed, 0xE511ULL);

  const double rho_scale = std::sqrt(std::max(0.0, 1.0 - params.rho * params.rho));
  const Series z_var = z_spot.mul(params.rho).add(z_aux.mul(rho_scale));
  const double p_jump = 1.0 - std::exp(-params.jump_intensity * dt);

  for (std::int64_t t = 0; t < steps; ++t) {
    const Series idx_cur = row_index_(t, out_cfg);
    const Series idx_next = row_index_(t + 1, out_cfg);

    const Series vt = variance.index_select(0, idx_cur).reshape({1, paths});
    const Series st = spot.index_select(0, idx_cur).reshape({1, paths});
    const Series eps_s = z_spot.index_select(0, idx_cur).reshape({1, paths});
    const Series eps_v = z_var.index_select(0, idx_cur).reshape({1, paths});
    const Series uq = u_qe.index_select(0, idx_cur).reshape({1, paths});
    const Series zj = z_jump.index_select(0, idx_cur).reshape({1, paths});
    const Series uj = u_jump.index_select(0, idx_cur).reshape({1, paths});

    const Series v_next = heston_variance_step_(vt, eps_v, uq, dt, h_params, options.scheme);
    const Series v_bar = vt.add(v_next).mul(0.5).clamp_min(params.variance_floor);

    const Series jump_mask = uj.le(p_jump);
    const Series jump_log = zj.mul(params.jump_vol).add(params.jump_mean);
    const Series jump_term = jump_mask.where(jump_log, pluss::zeros({1, paths}, out_cfg));
    const Series log_inc = v_bar.mul(-0.5 * dt)
                               .add(params.mu * dt)
                               .add(v_bar.mul(dt).sqrt().mul(eps_s))
                               .add(jump_term);
    const Series s_next = st.mul(log_inc.exp());

    variance = pluss::index_put(variance, std::vector<Series>{idx_next}, v_next.reshape({paths}), false);
    spot = pluss::index_put(spot, std::vector<Series>{idx_next}, s_next.reshape({paths}), false);
  }

  PLUSS_CHECK(spot.isfinite().all().item<double>() == 1.0,
              "subsp::math::finance::volatility::bates_paths: spot path produced non-finite values");
  PLUSS_CHECK(variance.isfinite().all().item<double>() == 1.0,
              "subsp::math::finance::volatility::bates_paths: variance path produced non-finite values");

  return finalize_paths_(spot, variance, z_spot, z_var, options, out_cfg);
}

StochVolPaths bates_paths(std::int64_t steps,
                          std::int64_t paths,
                          double spot0,
                          double dt,
                          const BatesParams& params,
                          Config cfg,
                          std::uint64_t seed) {
  SimOptions options;
  options.seed = seed;
  return bates_paths(steps, paths, spot0, dt, params, options, cfg, seed);
}

StochVolPaths sv_paths(std::int64_t steps,
                       std::int64_t paths,
                       double spot0,
                       double dt,
                       const SvParams& params,
                       const SimOptions& options,
                       Config cfg,
                       std::uint64_t seed) {
  validate_common_(steps, paths, spot0, dt, params.variance_floor, "sv_paths");
  validate_sv_(params, "sv_paths");
  validate_options_(options, "sv_paths");

  const auto out_cfg = f64_cfg_(cfg);
  const std::int64_t n = steps + 1;

  Series spot = pluss::full({n, paths}, spot0, out_cfg);
  Series logvar = pluss::full({n, paths}, params.logvar0, out_cfg);
  Series variance = pluss::full({n, paths}, std::exp(params.logvar0), out_cfg).clamp_min(params.variance_floor);

  const Series z_spot = draw_normals_(steps, paths, options, out_cfg, seed, 0x7811ULL);
  const Series z_aux = draw_normals_(steps, paths, options, out_cfg, seed, 0x9119ULL);
  const double rho_scale = std::sqrt(std::max(0.0, 1.0 - params.rho * params.rho));
  const Series z_var = z_spot.mul(params.rho).add(z_aux.mul(rho_scale));

  for (std::int64_t t = 0; t < steps; ++t) {
    const Series idx_cur = row_index_(t, out_cfg);
    const Series idx_next = row_index_(t + 1, out_cfg);

    const Series st = spot.index_select(0, idx_cur).reshape({1, paths});
    const Series h_t = logvar.index_select(0, idx_cur).reshape({1, paths});
    const Series v_t = h_t.exp().clamp_min(params.variance_floor);
    const Series eps_s = z_spot.index_select(0, idx_cur).reshape({1, paths});
    const Series eps_h = z_var.index_select(0, idx_cur).reshape({1, paths});

    const Series h_next = h_t.mul(params.phi)
                              .add(params.logvar_mean * (1.0 - params.phi))
                              .add(eps_h.mul(params.vol_of_logvar * std::sqrt(dt)));
    const Series v_next = h_next.exp().clamp_min(params.variance_floor);

    const Series log_inc = v_t.mul(-0.5 * dt).add(params.mu * dt).add(v_t.mul(dt).sqrt().mul(eps_s));
    const Series s_next = st.mul(log_inc.exp());

    logvar = pluss::index_put(logvar, std::vector<Series>{idx_next}, h_next.reshape({paths}), false);
    variance = pluss::index_put(variance, std::vector<Series>{idx_next}, v_next.reshape({paths}), false);
    spot = pluss::index_put(spot, std::vector<Series>{idx_next}, s_next.reshape({paths}), false);
  }

  PLUSS_CHECK(spot.isfinite().all().item<double>() == 1.0,
              "subsp::math::finance::volatility::sv_paths: spot path produced non-finite values");
  PLUSS_CHECK(variance.isfinite().all().item<double>() == 1.0,
              "subsp::math::finance::volatility::sv_paths: variance path produced non-finite values");

  return finalize_paths_(spot, variance, z_spot, z_var, options, out_cfg);
}

StochVolPaths sv_paths(std::int64_t steps,
                       std::int64_t paths,
                       double spot0,
                       double dt,
                       const SvParams& params,
                       Config cfg,
                       std::uint64_t seed) {
  SimOptions options;
  options.seed = seed;
  return sv_paths(steps, paths, spot0, dt, params, options, cfg, seed);
}

StochVolPaths heston_nandi_paths(std::int64_t steps,
                                 std::int64_t paths,
                                 double spot0,
                                 double dt,
                                 const HestonNandiParams& params,
                                 const SimOptions& options,
                                 Config cfg,
                                 std::uint64_t seed) {
  validate_common_(steps, paths, spot0, dt, params.variance_floor, "heston_nandi_paths");
  validate_heston_nandi_(params, "heston_nandi_paths");
  validate_options_(options, "heston_nandi_paths");

  const auto out_cfg = f64_cfg_(cfg);
  const std::int64_t n = steps + 1;
  const double h0 = heston_nandi_initial_variance_(params, "heston_nandi_paths");

  Series spot = pluss::full({n, paths}, spot0, out_cfg);
  Series variance = pluss::full({n, paths}, h0, out_cfg);
  const Series z_raw = draw_normals_(steps, paths, options, out_cfg, seed, 0xF311ULL);
  const Series z_spot = z_raw.isfinite()
                            .where(z_raw, pluss::zeros({steps, paths}, out_cfg))
                            .clamp_min(-10.0)
                            .clamp_max(10.0);

  for (std::int64_t t = 0; t < steps; ++t) {
    const Series idx_cur = row_index_(t, out_cfg);
    const Series idx_next = row_index_(t + 1, out_cfg);

    const Series ht = variance.index_select(0, idx_cur).reshape({1, paths}).clamp_min(params.variance_floor);
    const Series st = spot.index_select(0, idx_cur).reshape({1, paths});
    const Series zt = z_spot.index_select(0, idx_cur).reshape({1, paths});

    const Series sig_t = ht.sqrt();
    const Series log_inc = ht.mul(params.lambda).add(params.r * dt).add(sig_t.mul(zt)).clamp_min(-30.0).clamp_max(30.0);
    const Series s_raw = st.mul(log_inc.exp());
    const Series s_next = s_raw.isfinite()
                              .where(s_raw, pluss::full({1, paths}, spot0, out_cfg))
                              .clamp_min(params.variance_floor)
                              .clamp_max(1.0e12);

    const Series h_next = ht.mul(params.beta).add(params.omega).add(zt.sub(sig_t.mul(params.gamma)).square().mul(params.alpha));
    const Series h_safe = h_next.isfinite()
                              .where(h_next, pluss::full({1, paths}, params.variance_floor, out_cfg))
                              .clamp_min(params.variance_floor)
                              .clamp_max(1.0e6);

    spot = pluss::index_put(spot, std::vector<Series>{idx_next}, s_next.reshape({paths}), false);
    variance = pluss::index_put(variance, std::vector<Series>{idx_next}, h_safe.reshape({paths}), false);
  }

  PLUSS_CHECK(spot.isfinite().all().item<double>() == 1.0,
              "subsp::math::finance::volatility::heston_nandi_paths: spot path produced non-finite values");
  PLUSS_CHECK(variance.isfinite().all().item<double>() == 1.0,
              "subsp::math::finance::volatility::heston_nandi_paths: variance path produced non-finite values");

  return finalize_paths_(spot, variance, z_spot, z_spot, options, out_cfg);
}

StochVolPaths heston_nandi_paths(std::int64_t steps,
                                 std::int64_t paths,
                                 double spot0,
                                 double dt,
                                 const HestonNandiParams& params,
                                 Config cfg,
                                 std::uint64_t seed) {
  SimOptions options;
  options.seed = seed;
  return heston_nandi_paths(steps, paths, spot0, dt, params, options, cfg, seed);
}

StochVolPaths sabr_paths(std::int64_t steps,
                         std::int64_t paths,
                         double forward0,
                         double dt,
                         const SabrParams& params,
                         const SimOptions& options,
                         Config cfg,
                         std::uint64_t seed) {
  validate_common_(steps, paths, forward0, dt, params.alpha_floor, "sabr_paths");
  validate_sabr_(params, "sabr_paths");
  validate_options_(options, "sabr_paths");

  const auto out_cfg = f64_cfg_(cfg);
  const std::int64_t n = steps + 1;

  Series forward = pluss::full({n, paths}, forward0, out_cfg);
  Series alpha = pluss::full({n, paths}, std::max(params.alpha0, params.alpha_floor), out_cfg);
  const Series z1 = draw_normals_(steps, paths, options, out_cfg, seed, 0xAA11ULL);
  const Series z2 = draw_normals_(steps, paths, options, out_cfg, seed, 0xBB31ULL);
  const double rho_scale = std::sqrt(std::max(0.0, 1.0 - params.rho * params.rho));
  const Series z_alpha = z1.mul(params.rho).add(z2.mul(rho_scale));

  for (std::int64_t t = 0; t < steps; ++t) {
    const Series idx_cur = row_index_(t, out_cfg);
    const Series idx_next = row_index_(t + 1, out_cfg);
    const Series ft = forward.index_select(0, idx_cur).reshape({1, paths}).clamp_min(params.alpha_floor);
    const Series at = alpha.index_select(0, idx_cur).reshape({1, paths}).clamp_min(params.alpha_floor);
    const Series eps_f = z1.index_select(0, idx_cur).reshape({1, paths});
    const Series eps_a = z_alpha.index_select(0, idx_cur).reshape({1, paths});

    const Series alpha_log_inc =
        at.mul(params.nu * std::sqrt(dt)).mul(eps_a).clamp_min(-30.0).clamp_max(30.0);
    const Series a_next = at.mul(alpha_log_inc.exp()).clamp_min(params.alpha_floor).clamp_max(1.0e6);
    const Series sigma_f = at.mul(ft.pow(params.beta));
    const Series log_inc = sigma_f.mul(std::sqrt(dt)).mul(eps_f).add(params.mu * dt).clamp_min(-30.0).clamp_max(30.0);
    const Series f_next = ft.mul(log_inc.exp()).clamp_min(params.alpha_floor).clamp_max(1.0e12);

    alpha = pluss::index_put(alpha, std::vector<Series>{idx_next}, a_next.reshape({paths}), false);
    forward = pluss::index_put(forward, std::vector<Series>{idx_next}, f_next.reshape({paths}), false);
  }

  PLUSS_CHECK(forward.isfinite().all().item<double>() == 1.0,
              "subsp::math::finance::volatility::sabr_paths: forward path produced non-finite values");
  PLUSS_CHECK(alpha.isfinite().all().item<double>() == 1.0,
              "subsp::math::finance::volatility::sabr_paths: alpha path produced non-finite values");

  return finalize_paths_(forward, alpha.square(), z1, z_alpha, options, out_cfg);
}

StochVolPaths sabr_paths(std::int64_t steps,
                         std::int64_t paths,
                         double forward0,
                         double dt,
                         const SabrParams& params,
                         Config cfg,
                         std::uint64_t seed) {
  SimOptions options;
  options.seed = seed;
  return sabr_paths(steps, paths, forward0, dt, params, options, cfg, seed);
}

StochVolPaths slv_paths(std::int64_t steps,
                        std::int64_t paths,
                        double spot0,
                        double dt,
                        const SlvParams& params,
                        const SimOptions& options,
                        Config cfg,
                        std::uint64_t seed) {
  validate_common_(steps, paths, spot0, dt, params.variance_floor, "slv_paths");
  validate_slv_(params, "slv_paths");
  validate_options_(options, "slv_paths");

  const auto out_cfg = f64_cfg_(cfg);
  const std::int64_t n = steps + 1;

  Series spot = pluss::full({n, paths}, spot0, out_cfg);
  Series variance = pluss::full({n, paths}, std::max(params.v0, params.variance_floor), out_cfg);
  const Series z_spot = draw_normals_(steps, paths, options, out_cfg, seed, 0xCC17ULL);
  const Series z_aux = draw_normals_(steps, paths, options, out_cfg, seed, 0xDD29ULL);
  const double rho_scale = std::sqrt(std::max(0.0, 1.0 - params.rho * params.rho));
  const Series z_var = z_spot.mul(params.rho).add(z_aux.mul(rho_scale));

  for (std::int64_t t = 0; t < steps; ++t) {
    const Series idx_cur = row_index_(t, out_cfg);
    const Series idx_next = row_index_(t + 1, out_cfg);
    const Series st = spot.index_select(0, idx_cur).reshape({1, paths}).clamp_min(params.variance_floor);
    const Series vt = variance.index_select(0, idx_cur).reshape({1, paths}).clamp_min(params.variance_floor);
    const Series eps_s = z_spot.index_select(0, idx_cur).reshape({1, paths});
    const Series eps_v = z_var.index_select(0, idx_cur).reshape({1, paths});

    const Series v_next = vt.add(vt.mul(-params.kappa * dt))
                              .add(params.kappa * params.theta * dt)
                              .add(vt.sqrt().mul(params.sigma * std::sqrt(dt)).mul(eps_v))
                              .clamp_min(params.variance_floor);
    const Series lv = local_vol_(st, params);
    const Series inst_vol = lv.mul(vt.sqrt()).clamp_min(params.variance_floor);
    const Series log_inc = inst_vol.square().mul(-0.5 * dt).add(params.mu * dt).add(inst_vol.mul(std::sqrt(dt)).mul(eps_s));
    const Series s_next = st.mul(log_inc.exp()).clamp_min(params.variance_floor);

    variance = pluss::index_put(variance, std::vector<Series>{idx_next}, v_next.reshape({paths}), false);
    spot = pluss::index_put(spot, std::vector<Series>{idx_next}, s_next.reshape({paths}), false);
  }

  PLUSS_CHECK(spot.isfinite().all().item<double>() == 1.0,
              "subsp::math::finance::volatility::slv_paths: spot path produced non-finite values");
  PLUSS_CHECK(variance.isfinite().all().item<double>() == 1.0,
              "subsp::math::finance::volatility::slv_paths: variance path produced non-finite values");

  return finalize_paths_(spot, variance, z_spot, z_var, options, out_cfg);
}

StochVolPaths slv_paths(std::int64_t steps,
                        std::int64_t paths,
                        double spot0,
                        double dt,
                        const SlvParams& params,
                        Config cfg,
                        std::uint64_t seed) {
  SimOptions options;
  options.seed = seed;
  return slv_paths(steps, paths, spot0, dt, params, options, cfg, seed);
}

} // namespace pluss::subsp::math::finance::volatility
