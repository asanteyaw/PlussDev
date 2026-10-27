#include "pluss/subsp/math/finance/volatility.h"

#include "CodeM/core/Errors.h"
#include "pluss/codem/series_factory.h"
#include "pluss/subsp/stats/rng.h"

#include <cmath>
#include <cstdint>
#include <random>
#include <string>
#include <vector>

namespace pluss::subsp::math::finance::volatility {
namespace {

using pluss::Config;
using pluss::ScalarType;
using pluss::Series;
using volatility::GbmParams;
using volatility::BatesParams;
using volatility::HestonNandiParams;
using volatility::SabrParams;
using volatility::SlvParams;
using volatility::RoughHestonParams;
using volatility::RoughBergomiParams;
using pluss::subsp::stats::rng::HaltonOptions;
using pluss::subsp::stats::rng::Method;
using pluss::subsp::stats::rng::SobolOptions;
using pluss::subsp::stats::rng::TorusOptions;

constexpr double kPsiCut = 1.5;
constexpr double kEps = 1e-12;

Config f64_cfg_(Config cfg) {
  return Config{}.with_dtype(ScalarType::Float64).with_platform(cfg.platform).with_pinned(cfg.pinned);
}

Config cfg_like_(const Series& x) {
  return Config{}.with_dtype(x.scalar_type()).with_platform(x.platform());
}

Series append_antithetic_(const Series& draws, std::int64_t cols) {
  const std::int64_t base_cols = draws.size(1);
  if (base_cols >= cols) {
    return draws;
  }
  const std::int64_t need = cols - base_cols;
  const Series mirror = draws.mul(-1.0).slice(1, 0, need);
  return draws.unite({mirror}, 1);
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
    Series z = pluss::series::randn({rows, base_cols}, out_cfg, effective_seed_(options, compat_seed, bump));
    if (options.antithetic) {
      z = append_antithetic_(z, cols);
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
        z = append_antithetic_(z, cols);
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
        z = append_antithetic_(z, cols);
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
        z = append_antithetic_(z, cols);
      }
      return (std::abs(options.normal_shift) > 0.0) ? z.add(options.normal_shift) : z;
    }
    case RandMethod::Pseudo:
      break;
  }

  Series z = pluss::subsp::stats::rng::standard_normal(
      {rows, base_cols}, to_rng_method_(options.method), out_cfg, off);
  if (options.antithetic) {
    z = append_antithetic_(z, cols);
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
    return pluss::series::rand({rows, cols}, out_cfg, effective_seed_(options, compat_seed, bump));
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

void validate_gbm_(const GbmParams& p, const char* op) {
  PLUSS_CHECK(std::isfinite(p.mu), std::string("subsp::math::finance::volatility::") + op + ": mu must be finite");
  PLUSS_CHECK(std::isfinite(p.sigma) && p.sigma >= 0.0,
              std::string("subsp::math::finance::volatility::") + op + ": sigma must be finite and >= 0");
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

void validate_rough_heston_(const RoughHestonParams& p, const char* op) {
  PLUSS_CHECK(std::isfinite(p.kappa) && p.kappa >= 0.0,
              std::string("subsp::math::finance::volatility::") + op + ": kappa must be finite and >= 0");
  PLUSS_CHECK(std::isfinite(p.theta) && p.theta >= 0.0,
              std::string("subsp::math::finance::volatility::") + op + ": theta must be finite and >= 0");
  PLUSS_CHECK(std::isfinite(p.sigma) && p.sigma >= 0.0,
              std::string("subsp::math::finance::volatility::") + op + ": sigma must be finite and >= 0");
  PLUSS_CHECK(std::isfinite(p.alpha) && p.alpha > -0.5 && p.alpha < 0.0,
              std::string("subsp::math::finance::volatility::") + op + ": alpha must be in (-0.5,0)");
  PLUSS_CHECK(std::isfinite(p.rho) && p.rho > -1.0 && p.rho < 1.0,
              std::string("subsp::math::finance::volatility::") + op + ": rho must be in (-1,1)");
  PLUSS_CHECK(std::isfinite(p.v0) && p.v0 >= 0.0,
              std::string("subsp::math::finance::volatility::") + op + ": v0 must be finite and >= 0");
  PLUSS_CHECK(std::isfinite(p.mu), std::string("subsp::math::finance::volatility::") + op + ": mu must be finite");
}

void validate_rough_bergomi_(const RoughBergomiParams& p, const char* op) {
  PLUSS_CHECK(std::isfinite(p.alpha) && p.alpha > -0.5 && p.alpha < 0.0,
              std::string("subsp::math::finance::volatility::") + op + ": alpha must be in (-0.5,0)");
  PLUSS_CHECK(std::isfinite(p.xi) && p.xi >= 0.0,
              std::string("subsp::math::finance::volatility::") + op + ": xi must be finite and >= 0");
  PLUSS_CHECK(std::isfinite(p.eta) && p.eta > 0.0,
              std::string("subsp::math::finance::volatility::") + op + ": eta must be finite and > 0");
  PLUSS_CHECK(std::isfinite(p.rho) && p.rho > -1.0 && p.rho < 1.0,
              std::string("subsp::math::finance::volatility::") + op + ": rho must be in (-1,1)");
  PLUSS_CHECK(std::isfinite(p.mu), std::string("subsp::math::finance::volatility::") + op + ": mu must be finite");
}

Series local_vol_(const Series& s, const SlvParams& p) {
  if (p.local_vol_form == LocalVolForm::Constant) {
    return pluss::series::full(s.sizes(), std::max(0.0, p.local_vol_level), cfg_like_(s));
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

  PLUSS_CHECK(scheme == SimScheme::QuadExp || scheme == SimScheme::Transition,
              "subsp::math::finance::volatility::heston_variance_step_: invalid scheme for QE step");

  const Config vt_cfg = cfg_like_(vt);
  const Series exp_kdt = pluss::series::full(vt.sizes(), std::exp(-p.kappa * dt), vt_cfg);
  const Series one_minus_exp = exp_kdt.mul(-1.0).add(1.0);

  const Series m = pluss::series::full(vt.sizes(), p.theta, vt_cfg).add(vt.sub(p.theta).mul(exp_kdt));
  const Series s2 = vt.mul(p.sigma * p.sigma)
                        .mul(exp_kdt)
                        .mul(one_minus_exp)
                        .div(std::max(p.kappa, kEps))
                        .add(pluss::series::full(vt.sizes(), p.theta, vt_cfg)
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
  const Series v2 = u.le(p_zero).where(pluss::series::zeros(vt.sizes(), vt_cfg), v2_nonzero);

  return use_q1.where(v1, v2).clamp_min(p.variance_floor);
}

Series heston_transition_step_(const Series& vt,
                               const Series& z_var,
                               double dt,
                               const HestonParams& p,
                               std::uint64_t seed,
                               std::int64_t t_step) {
  const Series v_safe = vt.clamp_min(p.variance_floor);
  const Config cfg = Config{}.with_dtype(v_safe.scalar_type()).with_platform(v_safe.platform());
  const Config cpu_cfg = Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);
  const auto shape = v_safe.sizes();
  const std::int64_t paths = (shape.size() == 2) ? shape[1] : v_safe.numel();
  PLUSS_CHECK(paths > 0, "subsp::math::finance::volatility::heston_transition_step_: invalid path count");

  const double exp_kdt = std::exp(-p.kappa * dt);
  const double c = (p.sigma * p.sigma) * (1.0 - exp_kdt) / std::max(4.0 * p.kappa, kEps);
  const double degree = 4.0 * p.theta * p.kappa / std::max(p.sigma * p.sigma, kEps);
  PLUSS_CHECK(std::isfinite(c) && c > 0.0,
              "subsp::math::finance::volatility::heston_transition_step_: invalid scale constant");
  PLUSS_CHECK(std::isfinite(degree) && degree > 0.0,
              "subsp::math::finance::volatility::heston_transition_step_: invalid degree");

  const std::vector<double> v_vec = v_safe.reshape({paths}).to(cpu_cfg.platform, cpu_cfg.dtype).host<double>();
  const std::vector<double> z_vec = z_var.reshape({paths}).to(cpu_cfg.platform, cpu_cfg.dtype).host<double>();
  std::vector<double> out(static_cast<std::size_t>(paths), p.variance_floor);

  std::mt19937_64 rng(seed ^ (0x94D049BB133111EBULL + static_cast<std::uint64_t>(t_step + 1) * 0x9E3779B97F4A7C15ULL));
  if (degree > 1.0) {
    std::gamma_distribution<double> chisq((degree - 1.0) * 0.5, 2.0);
    for (std::int64_t i = 0; i < paths; ++i) {
      const double lambda_v = std::max(0.0, (v_vec[static_cast<std::size_t>(i)] * exp_kdt) / c);
      const double chi = chisq(rng);
      const double draw = c * (std::pow(z_vec[static_cast<std::size_t>(i)] + std::sqrt(lambda_v), 2.0) + chi);
      out[static_cast<std::size_t>(i)] = std::max(draw, p.variance_floor);
    }
  } else {
    for (std::int64_t i = 0; i < paths; ++i) {
      const double lambda_v = std::max(0.0, (v_vec[static_cast<std::size_t>(i)] * exp_kdt) / c);
      std::poisson_distribution<std::int64_t> pois(0.5 * lambda_v);
      const std::int64_t n = pois(rng);
      std::gamma_distribution<double> chisq(0.5 * degree + static_cast<double>(n), 2.0);
      const double draw = c * chisq(rng);
      out[static_cast<std::size_t>(i)] = std::max(draw, p.variance_floor);
    }
  }

  return pluss::make_series(out, {1, paths}, cpu_cfg).to(cfg.platform, cfg.dtype);
}

Series stack_rows_(const std::vector<Series>& rows, const char* op) {
  PLUSS_CHECK(!rows.empty(), std::string("subsp::math::finance::volatility::") + op + ": row history is empty");
  return pluss::series::pile(pluss::sign_list(rows), 0);
}

} // namespace

StochVolPaths gbm_paths(std::int64_t steps,
                        std::int64_t paths,
                        double spot0,
                        double dt,
                        const GbmParams& params,
                        const SimOptions& options,
                        Config cfg,
                        std::uint64_t seed) {
  validate_common_(steps, paths, spot0, dt, 1.0e-12, "gbm_paths");
  validate_gbm_(params, "gbm_paths");
  validate_options_(options, "gbm_paths");

  const auto out_cfg = f64_cfg_(cfg);
  const std::int64_t n = steps + 1;
  const double sigma2 = params.sigma * params.sigma;
  const Series z_spot = draw_normals_(steps, paths, options, out_cfg, seed, 0x7011ULL);
  const Series z_var = pluss::series::zeros({steps, paths}, out_cfg);
  const Series log_inc = z_spot.mul(params.sigma * std::sqrt(dt)).add((params.mu - 0.5 * sigma2) * dt);
  const Series spot_tail = log_inc.cumsum(0, false).exp().mul(spot0);
  const Series spot = pluss::series::full({1, paths}, spot0, out_cfg).unite({spot_tail}, 0);
  const Series variance = pluss::series::full({n, paths}, sigma2, out_cfg);

  PLUSS_CHECK(spot.isfinite().all().item<double>() == 1.0,
              "subsp::math::finance::volatility::gbm_paths: spot path produced non-finite values");
  return finalize_paths_(spot, variance, z_spot, z_var, options, out_cfg);
}

StochVolPaths gbm_paths(std::int64_t steps,
                        std::int64_t paths,
                        double spot0,
                        double dt,
                        const GbmParams& params,
                        Config cfg,
                        std::uint64_t seed) {
  SimOptions options;
  options.seed = seed;
  return gbm_paths(steps, paths, spot0, dt, params, options, cfg, seed);
}

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

  const Series z_spot = draw_normals_(steps, paths, options, out_cfg, seed, 0x1021ULL);
  const Series z_aux = draw_normals_(steps, paths, options, out_cfg, seed, 0x2303ULL);
  const bool use_qe = (options.scheme == SimScheme::QuadExp);
  const Series u_qe = use_qe ? draw_uniforms_(steps, paths, options, out_cfg, seed, 0x4517ULL)
                             : pluss::series::zeros({steps, paths}, out_cfg);
  const std::uint64_t transition_seed = effective_seed_(options, seed, 0x5843ULL);

  const double rho_scale = std::sqrt(std::max(0.0, 1.0 - params.rho * params.rho));
  const Series z_var = z_spot.mul(params.rho).add(z_aux.mul(rho_scale));

  Series st = pluss::series::full({1, paths}, spot0, out_cfg);
  Series vt = pluss::series::full({1, paths}, std::max(params.v0, params.variance_floor), out_cfg);
  std::vector<Series> spot_rows;
  std::vector<Series> variance_rows;
  spot_rows.reserve(static_cast<std::size_t>(n));
  variance_rows.reserve(static_cast<std::size_t>(n));
  spot_rows.push_back(st.reshape({paths}));
  variance_rows.push_back(vt.reshape({paths}));

  for (std::int64_t t = 0; t < steps; ++t) {
    const Series eps_s = z_spot.select(0, t).reshape({1, paths});
    const Series eps_v = z_var.select(0, t).reshape({1, paths});
    const Series v_next = (options.scheme == SimScheme::Transition)
                              ? heston_transition_step_(vt, eps_v, dt, params, transition_seed, t)
                              : heston_variance_step_(vt,
                                                     eps_v,
                                                     u_qe.select(0, t).reshape({1, paths}),
                                                     dt,
                                                     params,
                                                     options.scheme);
    const Series v_bar = vt.add(v_next).mul(0.5).clamp_min(params.variance_floor);

    const Series log_inc = v_bar.mul(-0.5 * dt)
                               .add(params.mu * dt)
                               .add(v_bar.mul(dt).sqrt().mul(eps_s));
    const Series s_next = st.mul(log_inc.exp()).clamp_min(params.variance_floor).clamp_max(1.0e12);

    vt = v_next;
    st = s_next;
    variance_rows.push_back(vt.reshape({paths}));
    spot_rows.push_back(st.reshape({paths}));
  }

  const Series spot = stack_rows_(spot_rows, "heston_paths");
  const Series variance = stack_rows_(variance_rows, "heston_paths");
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

  const Series z_spot = draw_normals_(steps, paths, options, out_cfg, seed, 0xA129ULL);
  const Series z_aux = draw_normals_(steps, paths, options, out_cfg, seed, 0xB273ULL);
  const bool use_qe = (options.scheme == SimScheme::QuadExp);
  const Series u_qe = use_qe ? draw_uniforms_(steps, paths, options, out_cfg, seed, 0xC317ULL)
                             : pluss::series::zeros({steps, paths}, out_cfg);
  const Series z_jump = draw_normals_(steps, paths, options, out_cfg, seed, 0xD401ULL);
  const Series u_jump = draw_uniforms_(steps, paths, options, out_cfg, seed, 0xE511ULL);
  const std::uint64_t transition_seed = effective_seed_(options, seed, 0xF713ULL);

  const double rho_scale = std::sqrt(std::max(0.0, 1.0 - params.rho * params.rho));
  const Series z_var = z_spot.mul(params.rho).add(z_aux.mul(rho_scale));
  const double p_jump = 1.0 - std::exp(-params.jump_intensity * dt);

  Series st = pluss::series::full({1, paths}, spot0, out_cfg);
  Series vt = pluss::series::full({1, paths}, std::max(params.v0, params.variance_floor), out_cfg);
  std::vector<Series> spot_rows;
  std::vector<Series> variance_rows;
  spot_rows.reserve(static_cast<std::size_t>(n));
  variance_rows.reserve(static_cast<std::size_t>(n));
  spot_rows.push_back(st.reshape({paths}));
  variance_rows.push_back(vt.reshape({paths}));

  for (std::int64_t t = 0; t < steps; ++t) {
    const Series eps_s = z_spot.select(0, t).reshape({1, paths});
    const Series eps_v = z_var.select(0, t).reshape({1, paths});
    const Series zj = z_jump.select(0, t).reshape({1, paths});
    const Series uj = u_jump.select(0, t).reshape({1, paths});

    const Series v_next = (options.scheme == SimScheme::Transition)
                              ? heston_transition_step_(vt, eps_v, dt, h_params, transition_seed, t)
                              : heston_variance_step_(vt,
                                                     eps_v,
                                                     u_qe.select(0, t).reshape({1, paths}),
                                                     dt,
                                                     h_params,
                                                     options.scheme);
    const Series v_bar = vt.add(v_next).mul(0.5).clamp_min(params.variance_floor);

    const Series jump_mask = uj.le(p_jump);
    const Series jump_log = zj.mul(params.jump_vol).add(params.jump_mean);
    const Series jump_term = jump_mask.where(jump_log, pluss::series::zeros({1, paths}, out_cfg));
    const Series log_inc = v_bar.mul(-0.5 * dt)
                               .add(params.mu * dt)
                               .add(v_bar.mul(dt).sqrt().mul(eps_s))
                               .add(jump_term);
    const Series s_next = st.mul(log_inc.exp());

    vt = v_next;
    st = s_next;
    variance_rows.push_back(vt.reshape({paths}));
    spot_rows.push_back(st.reshape({paths}));
  }

  const Series spot = stack_rows_(spot_rows, "bates_paths");
  const Series variance = stack_rows_(variance_rows, "bates_paths");
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

  const Series z_spot = draw_normals_(steps, paths, options, out_cfg, seed, 0x7811ULL);
  const Series z_aux = draw_normals_(steps, paths, options, out_cfg, seed, 0x9119ULL);
  const double rho_scale = std::sqrt(std::max(0.0, 1.0 - params.rho * params.rho));
  const Series z_var = z_spot.mul(params.rho).add(z_aux.mul(rho_scale));

  Series st = pluss::series::full({1, paths}, spot0, out_cfg);
  Series h_t = pluss::series::full({1, paths}, params.logvar0, out_cfg);
  Series vt = pluss::series::full({1, paths}, std::exp(params.logvar0), out_cfg).clamp_min(params.variance_floor);
  std::vector<Series> spot_rows;
  std::vector<Series> variance_rows;
  spot_rows.reserve(static_cast<std::size_t>(n));
  variance_rows.reserve(static_cast<std::size_t>(n));
  spot_rows.push_back(st.reshape({paths}));
  variance_rows.push_back(vt.reshape({paths}));

  for (std::int64_t t = 0; t < steps; ++t) {
    const Series v_t = h_t.exp().clamp_min(params.variance_floor);
    const Series eps_s = z_spot.select(0, t).reshape({1, paths});
    const Series eps_h = z_var.select(0, t).reshape({1, paths});

    const Series h_next = h_t.mul(params.phi)
                              .add(params.logvar_mean * (1.0 - params.phi))
                              .add(eps_h.mul(params.vol_of_logvar * std::sqrt(dt)));
    const Series v_next = h_next.exp().clamp_min(params.variance_floor);

    const Series log_inc = v_t.mul(-0.5 * dt).add(params.mu * dt).add(v_t.mul(dt).sqrt().mul(eps_s));
    const Series s_next = st.mul(log_inc.exp());

    h_t = h_next;
    vt = v_next;
    st = s_next;
    variance_rows.push_back(vt.reshape({paths}));
    spot_rows.push_back(st.reshape({paths}));
  }

  const Series spot = stack_rows_(spot_rows, "sv_paths");
  const Series variance = stack_rows_(variance_rows, "sv_paths");
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

  const Series z_raw = draw_normals_(steps, paths, options, out_cfg, seed, 0xF311ULL);
  const Series z_spot = z_raw.isfinite()
                            .where(z_raw, pluss::series::zeros({steps, paths}, out_cfg))
                            .clamp_min(-10.0)
                            .clamp_max(10.0);

  Series st = pluss::series::full({1, paths}, spot0, out_cfg);
  Series ht = pluss::series::full({1, paths}, h0, out_cfg);
  std::vector<Series> spot_rows;
  std::vector<Series> variance_rows;
  spot_rows.reserve(static_cast<std::size_t>(n));
  variance_rows.reserve(static_cast<std::size_t>(n));
  spot_rows.push_back(st.reshape({paths}));
  variance_rows.push_back(ht.reshape({paths}));

  for (std::int64_t t = 0; t < steps; ++t) {
    ht = ht.clamp_min(params.variance_floor);
    const Series zt = z_spot.select(0, t).reshape({1, paths});

    const Series sig_t = ht.sqrt();
    const Series log_inc = ht.mul(params.lambda).add(params.r * dt).add(sig_t.mul(zt)).clamp_min(-30.0).clamp_max(30.0);
    const Series s_raw = st.mul(log_inc.exp());
    const Series s_next = s_raw.isfinite()
                              .where(s_raw, pluss::series::full({1, paths}, spot0, out_cfg))
                              .clamp_min(params.variance_floor)
                              .clamp_max(1.0e12);

    const Series h_next = ht.mul(params.beta).add(params.omega).add(zt.sub(sig_t.mul(params.gamma)).square().mul(params.alpha));
    const Series h_safe = h_next.isfinite()
                              .where(h_next, pluss::series::full({1, paths}, params.variance_floor, out_cfg))
                              .clamp_min(params.variance_floor)
                              .clamp_max(1.0e6);

    st = s_next;
    ht = h_safe;
    spot_rows.push_back(st.reshape({paths}));
    variance_rows.push_back(ht.reshape({paths}));
  }

  const Series spot = stack_rows_(spot_rows, "heston_nandi_paths");
  const Series variance = stack_rows_(variance_rows, "heston_nandi_paths");
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
  const Series z1 = draw_normals_(steps, paths, options, out_cfg, seed, 0xAA11ULL);
  const Series z2 = draw_normals_(steps, paths, options, out_cfg, seed, 0xBB31ULL);
  const double rho_scale = std::sqrt(std::max(0.0, 1.0 - params.rho * params.rho));
  const Series z_alpha = z1.mul(params.rho).add(z2.mul(rho_scale));
  const double sqdt = std::sqrt(dt);
  const bool positive_forward = params.beta > 0.0;

  Series ft_state = pluss::series::full({1, paths}, forward0, out_cfg);
  Series at_state = pluss::series::full({1, paths}, std::max(params.alpha0, params.alpha_floor), out_cfg);
  std::vector<Series> forward_rows;
  std::vector<Series> alpha_rows;
  forward_rows.reserve(static_cast<std::size_t>(n));
  alpha_rows.reserve(static_cast<std::size_t>(n));
  forward_rows.push_back(ft_state.reshape({paths}));
  alpha_rows.push_back(at_state.reshape({paths}));

  for (std::int64_t t = 0; t < steps; ++t) {
    const Series ft_raw = ft_state;
    const Series ft = positive_forward ? ft_raw.clamp_min(params.alpha_floor) : ft_raw;
    const Series at = at_state.clamp_min(params.alpha_floor);
    const Series eps_f = z1.select(0, t).reshape({1, paths});
    const Series eps_a = z_alpha.select(0, t).reshape({1, paths});

    // Exact lognormal step for d alpha_t = nu * alpha_t * dW_t.
    const Series alpha_log_inc = eps_a.mul(params.nu * sqdt).add(-0.5 * params.nu * params.nu * dt).clamp_min(-30.0).clamp_max(30.0);
    const Series a_next = at.mul(alpha_log_inc.exp()).clamp_min(params.alpha_floor).clamp_max(1.0e6);

    const Series f_pow = (params.beta == 0.0) ? pluss::series::full({1, paths}, 1.0, out_cfg)
                                               : ft.clamp_min(params.alpha_floor).pow(params.beta);
    const Series sigma_f = at.mul(f_pow);
    Series f_next = ft.add(ft.mul(params.mu * dt)).add(sigma_f.mul(sqdt).mul(eps_f));

    if (options.scheme == SimScheme::Milstein || options.scheme == SimScheme::Milstein2) {
      if (params.beta > 0.0) {
        const Series milstein = at.square()
                                    .mul(0.5 * params.beta * dt)
                                    .mul(ft.clamp_min(params.alpha_floor).pow(2.0 * params.beta - 1.0))
                                    .mul(eps_f.square().add(-1.0));
        f_next = f_next.add(milstein);
      }
    }

    const Series f_safe = f_next.isfinite().where(f_next, pluss::series::full({1, paths}, forward0, out_cfg));
    const Series f_bounded = positive_forward
                                 ? f_safe.clamp_min(params.alpha_floor).clamp_max(1.0e12)
                                 : f_safe.clamp_min(-1.0e12).clamp_max(1.0e12);

    at_state = a_next;
    ft_state = f_bounded;
    alpha_rows.push_back(at_state.reshape({paths}));
    forward_rows.push_back(ft_state.reshape({paths}));
  }

  const Series forward = stack_rows_(forward_rows, "sabr_paths");
  const Series alpha = stack_rows_(alpha_rows, "sabr_paths");
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
  const Series z_spot = draw_normals_(steps, paths, options, out_cfg, seed, 0xCC17ULL);
  const Series z_aux = draw_normals_(steps, paths, options, out_cfg, seed, 0xDD29ULL);
  const double rho_scale = std::sqrt(std::max(0.0, 1.0 - params.rho * params.rho));
  const Series z_var = z_spot.mul(params.rho).add(z_aux.mul(rho_scale));

  Series st = pluss::series::full({1, paths}, spot0, out_cfg);
  Series vt = pluss::series::full({1, paths}, std::max(params.v0, params.variance_floor), out_cfg);
  std::vector<Series> spot_rows;
  std::vector<Series> variance_rows;
  spot_rows.reserve(static_cast<std::size_t>(n));
  variance_rows.reserve(static_cast<std::size_t>(n));
  spot_rows.push_back(st.reshape({paths}));
  variance_rows.push_back(vt.reshape({paths}));

  for (std::int64_t t = 0; t < steps; ++t) {
    st = st.clamp_min(params.variance_floor);
    vt = vt.clamp_min(params.variance_floor);
    const Series eps_s = z_spot.select(0, t).reshape({1, paths});
    const Series eps_v = z_var.select(0, t).reshape({1, paths});

    const Series v_next = vt.add(vt.mul(-params.kappa * dt))
                              .add(params.kappa * params.theta * dt)
                              .add(vt.sqrt().mul(params.sigma * std::sqrt(dt)).mul(eps_v))
                              .clamp_min(params.variance_floor);
    const Series lv = local_vol_(st, params);
    const Series inst_vol = lv.mul(vt.sqrt()).clamp_min(params.variance_floor);
    const Series log_inc = inst_vol.square().mul(-0.5 * dt).add(params.mu * dt).add(inst_vol.mul(std::sqrt(dt)).mul(eps_s));
    const Series s_next = st.mul(log_inc.exp()).clamp_min(params.variance_floor);

    vt = v_next;
    st = s_next;
    variance_rows.push_back(vt.reshape({paths}));
    spot_rows.push_back(st.reshape({paths}));
  }

  const Series spot = stack_rows_(spot_rows, "slv_paths");
  const Series variance = stack_rows_(variance_rows, "slv_paths");
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

StochVolPaths rough_heston_paths(std::int64_t steps,
                                 std::int64_t paths,
                                 double spot0,
                                 double dt,
                                 const RoughHestonParams& params,
                                 const SimOptions& options,
                                 Config cfg,
                                 std::uint64_t seed) {
  validate_common_(steps, paths, spot0, dt, params.variance_floor, "rough_heston_paths");
  validate_rough_heston_(params, "rough_heston_paths");
  validate_options_(options, "rough_heston_paths");

  const auto out_cfg = f64_cfg_(cfg);
  const std::int64_t n = steps + 1;
  const double H = params.alpha + 0.5;
  const double sqdt = std::sqrt(dt);

  const Series z_spot = draw_normals_(steps, paths, options, out_cfg, seed, 0xAB11ULL);
  const Series z_aux = draw_normals_(steps, paths, options, out_cfg, seed, 0xCD29ULL);
  const double rho_scale = std::sqrt(std::max(0.0, 1.0 - params.rho * params.rho));
  const Series z_var = z_spot.mul(params.rho).add(z_aux.mul(rho_scale));

  std::vector<double> kernel(static_cast<std::size_t>(steps), 0.0);
  for (std::int64_t k = 0; k < steps; ++k) {
    const double a = std::pow(static_cast<double>(k + 1), H);
    const double b = (k == 0) ? 0.0 : std::pow(static_cast<double>(k), H);
    kernel[static_cast<std::size_t>(k)] = a - b;
  }
  const Series kernel_series = pluss::make_series(kernel, {steps}, out_cfg);
  Series drives_hist = pluss::series::zeros({paths, steps}, out_cfg);

  Series st = pluss::series::full({1, paths}, spot0, out_cfg);
  Series vt = pluss::series::full({1, paths}, std::max(params.v0, params.variance_floor), out_cfg);
  std::vector<Series> spot_rows;
  std::vector<Series> variance_rows;
  spot_rows.reserve(static_cast<std::size_t>(n));
  variance_rows.reserve(static_cast<std::size_t>(n));
  spot_rows.push_back(st.reshape({paths}));
  variance_rows.push_back(vt.reshape({paths}));

  for (std::int64_t t = 0; t < steps; ++t) {
    st = st.clamp_min(params.variance_floor);
    vt = vt.clamp_min(params.variance_floor);
    const Series eps_s = z_spot.select(0, t).reshape({1, paths});
    const Series eps_v = z_var.select(0, t).reshape({1, paths});

    const Series drift = pluss::series::full({1, paths}, params.kappa * params.theta * dt, out_cfg).sub(vt.mul(params.kappa * dt));
    const Series diff = vt.sqrt().mul(params.sigma * sqdt).mul(eps_v);
    const Series drive_t = drift.add(diff).reshape({paths});
    drives_hist.index_put_({pluss::Slicing::SliceAll(), t}, drive_t);
    const Series drives_prefix = drives_hist.slice(1, 0, t + 1);
    const Series k_rev = kernel_series.slice(0, 0, t + 1).flip(0).reshape({1, t + 1});
    const Series frac_acc = drives_prefix.mul(k_rev).sum(1, false).reshape({1, paths});

    const Series v_next =
        pluss::series::full({1, paths}, std::max(params.v0, params.variance_floor), out_cfg).add(frac_acc).clamp_min(params.variance_floor);
    const Series v_bar = vt.add(v_next).mul(0.5).clamp_min(params.variance_floor);
    const Series log_inc = v_bar.mul(-0.5 * dt).add(params.mu * dt).add(v_bar.mul(dt).sqrt().mul(eps_s));
    const Series s_next = st.mul(log_inc.exp()).clamp_min(params.variance_floor).clamp_max(1.0e12);

    vt = v_next;
    st = s_next;
    variance_rows.push_back(vt.reshape({paths}));
    spot_rows.push_back(st.reshape({paths}));
  }

  const Series spot = stack_rows_(spot_rows, "rough_heston_paths");
  const Series variance = stack_rows_(variance_rows, "rough_heston_paths");
  PLUSS_CHECK(spot.isfinite().all().item<double>() == 1.0,
              "subsp::math::finance::volatility::rough_heston_paths: spot path produced non-finite values");
  PLUSS_CHECK(variance.isfinite().all().item<double>() == 1.0,
              "subsp::math::finance::volatility::rough_heston_paths: variance path produced non-finite values");
  return finalize_paths_(spot, variance, z_spot, z_var, options, out_cfg);
}

StochVolPaths rough_heston_paths(std::int64_t steps,
                                 std::int64_t paths,
                                 double spot0,
                                 double dt,
                                 const RoughHestonParams& params,
                                 Config cfg,
                                 std::uint64_t seed) {
  SimOptions options;
  options.seed = seed;
  return rough_heston_paths(steps, paths, spot0, dt, params, options, cfg, seed);
}

StochVolPaths rough_bergomi_paths(std::int64_t steps,
                                  std::int64_t paths,
                                  double spot0,
                                  double dt,
                                  const RoughBergomiParams& params,
                                  const SimOptions& options,
                                  Config cfg,
                                  std::uint64_t seed) {
  validate_common_(steps, paths, spot0, dt, params.variance_floor, "rough_bergomi_paths");
  validate_rough_bergomi_(params, "rough_bergomi_paths");
  validate_options_(options, "rough_bergomi_paths");

  const auto out_cfg = f64_cfg_(cfg);
  const std::int64_t n = steps + 1;
  const double H = params.alpha + 0.5;
  const double sqdt = std::sqrt(dt);
  const double rough_scale = std::sqrt(std::max(1e-12, 2.0 * H));

  const Series z_spot = draw_normals_(steps, paths, options, out_cfg, seed, 0xD519ULL);
  const Series z_aux = draw_normals_(steps, paths, options, out_cfg, seed, 0xF731ULL);
  const double rho_scale = std::sqrt(std::max(0.0, 1.0 - params.rho * params.rho));
  const Series z_var = z_spot.mul(params.rho).add(z_aux.mul(rho_scale));

  std::vector<double> kernel(static_cast<std::size_t>(steps), 0.0);
  for (std::int64_t k = 0; k < steps; ++k) {
    const double a = std::pow(static_cast<double>(k + 1), H);
    const double b = (k == 0) ? 0.0 : std::pow(static_cast<double>(k), H);
    kernel[static_cast<std::size_t>(k)] = a - b;
  }
  const Series kernel_series = pluss::make_series(kernel, {steps}, out_cfg);
  Series dWv_hist = pluss::series::zeros({paths, steps}, out_cfg);

  Series st = pluss::series::full({1, paths}, spot0, out_cfg);
  Series vt = pluss::series::full({1, paths}, std::max(params.xi, params.variance_floor), out_cfg);
  std::vector<Series> spot_rows;
  std::vector<Series> variance_rows;
  spot_rows.reserve(static_cast<std::size_t>(n));
  variance_rows.reserve(static_cast<std::size_t>(n));
  spot_rows.push_back(st.reshape({paths}));
  variance_rows.push_back(vt.reshape({paths}));

  for (std::int64_t t = 0; t < steps; ++t) {
    st = st.clamp_min(params.variance_floor);
    vt = vt.clamp_min(params.variance_floor);
    const Series eps_s = z_spot.select(0, t).reshape({1, paths});
    const Series eps_v = z_var.select(0, t).reshape({1, paths});
    dWv_hist.index_put_({pluss::Slicing::SliceAll(), t}, eps_v.mul(sqdt).reshape({paths}));

    const Series log_inc = vt.mul(-0.5 * dt).add(params.mu * dt).add(vt.mul(dt).sqrt().mul(eps_s));
    const Series s_next = st.mul(log_inc.exp()).clamp_min(params.variance_floor).clamp_max(1.0e12);

    const Series dW_prefix = dWv_hist.slice(1, 0, t + 1);
    const Series k_rev = kernel_series.slice(0, 0, t + 1).flip(0).reshape({1, t + 1});
    Series y_next = dW_prefix.mul(k_rev).sum(1, false).reshape({1, paths});
    y_next = y_next.mul(rough_scale);

    const double t_next = static_cast<double>(t + 1) * dt;
    const double det_term = -0.5 * params.eta * params.eta * std::pow(t_next, 2.0 * H);
    const Series v_next = y_next.mul(params.eta)
                              .add(det_term)
                              .exp()
                              .mul(std::max(params.xi, params.variance_floor))
                              .clamp_min(params.variance_floor)
                              .clamp_max(1.0e6);

    st = s_next;
    vt = v_next;
    spot_rows.push_back(st.reshape({paths}));
    variance_rows.push_back(vt.reshape({paths}));
  }

  const Series spot = stack_rows_(spot_rows, "rough_bergomi_paths");
  const Series variance = stack_rows_(variance_rows, "rough_bergomi_paths");
  PLUSS_CHECK(spot.isfinite().all().item<double>() == 1.0,
              "subsp::math::finance::volatility::rough_bergomi_paths: spot path produced non-finite values");
  PLUSS_CHECK(variance.isfinite().all().item<double>() == 1.0,
              "subsp::math::finance::volatility::rough_bergomi_paths: variance path produced non-finite values");
  return finalize_paths_(spot, variance, z_spot, z_var, options, out_cfg);
}

StochVolPaths rough_bergomi_paths(std::int64_t steps,
                                  std::int64_t paths,
                                  double spot0,
                                  double dt,
                                  const RoughBergomiParams& params,
                                  Config cfg,
                                  std::uint64_t seed) {
  SimOptions options;
  options.seed = seed;
  return rough_bergomi_paths(steps, paths, spot0, dt, params, options, cfg, seed);
}

} // namespace pluss::subsp::math::finance::volatility
