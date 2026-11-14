#include "pluss/subsp/math/finance/volatility.h"

#include "CodeM/core/Errors.h"
#include "pluss/codem/series_factory.h"
#include "pluss/subsp/linearalg/linearalg_fwd.h"
#include "pluss/subsp/stats/rng.h"

#include <cmath>
#include <cstdint>
#include <random>
#include <string>
#include <vector>

namespace pluss::subsp::math::finance::volatility {
using namespace detail;
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

// The independent random inputs of a generator (the spot shock, the variance shock, jump shocks, ...) are columns blocks of ONE
// low-discrepancy sequence: `count` blocks of `rows` dimensions each, and the draw returns block `index`. Pseudo-random draws ignore the
// block, since each call already has its own stream.
struct DrawBlock {
  std::int64_t index = 0;
  std::int64_t count = 1;
};

// Draws a block of a quasi-random sequence with `paths` points and `rows * count` dimensions, and returns it as {rows, paths}. The points
// are the paths and the dimensions are the time steps; a sequence whose points were time steps would have thousands of dimensions and a
// handful of points.
template <typename Sequence>
Series quasi_block_(Sequence&& sequence, std::int64_t rows, std::int64_t points, DrawBlock block) {
  const Series all = sequence({points, rows * block.count});
  return all.slice(1, block.index * rows, (block.index + 1) * rows).transpose(0, 1).contiguous();
}

// `apply_shift` marks the shock that carries the importance-sampling shift: only the spot shock is shifted, and the likelihood ratio of
// the price estimators accounts for that shock alone (the variance shock follows through its correlation with it).
Series draw_normals_(std::int64_t rows,
                     std::int64_t cols,
                     const SimOptions& options,
                     Config cfg,
                     std::uint64_t compat_seed,
                     std::uint64_t bump,
                     DrawBlock block = {},
                     bool apply_shift = false) {
  const std::int64_t base_cols = options.antithetic ? ((cols + 1) / 2) : cols;
  const auto out_cfg = f64_cfg_(cfg);
  const bool shifted = apply_shift && std::abs(options.normal_shift) > 0.0;
  auto finish = [&](Series z) {
    if (options.antithetic) {
      z = append_antithetic_(z, cols);
    }
    return shifted ? z.add(options.normal_shift) : z;
  };

  if (options.method == RandMethod::Pseudo) {
    return finish(pluss::series::randn({rows, base_cols}, out_cfg, effective_seed_(options, compat_seed, bump)));
  }

  // Every block of one generator uses the same sequence parameters, so that the blocks are disjoint dimensions of the same points.
  const std::uint64_t off = effective_offset_(options, 0x1ULL);
  const std::uint64_t seed = effective_seed_(options, compat_seed, 0x1ULL);

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
      return finish(quasi_block_([&](std::initializer_list<std::int64_t> sz) { return pluss::subsp::stats::rng::sobol(sz, o, out_cfg); }, rows, base_cols, block));
    }
    case RandMethod::Halton: {
      HaltonOptions o;
      o.init = true;
      o.normal = true;
      o.mixed = options.randomized_quasi;
      o.start = off + 1U;
      o.seed = seed;
      o.seed_set = true;
      return finish(quasi_block_([&](std::initializer_list<std::int64_t> sz) { return pluss::subsp::stats::rng::halton(sz, o, out_cfg); }, rows, base_cols, block));
    }
    case RandMethod::Torus: {
      TorusOptions o;
      o.init = true;
      o.normal = true;
      o.mixed = options.randomized_quasi;
      o.start = off + 1U;
      o.seed = seed;
      o.seed_set = true;
      return finish(quasi_block_([&](std::initializer_list<std::int64_t> sz) { return pluss::subsp::stats::rng::torus(sz, o, out_cfg); }, rows, base_cols, block));
    }
    case RandMethod::Pseudo:
      break;
  }
  return finish(pluss::subsp::stats::rng::standard_normal({rows, base_cols}, to_rng_method_(options.method), out_cfg, off));
}

Series draw_uniforms_(std::int64_t rows,
                      std::int64_t cols,
                      const SimOptions& options,
                      Config cfg,
                      std::uint64_t compat_seed,
                      std::uint64_t bump,
                      DrawBlock block = {}) {
  const auto out_cfg = f64_cfg_(cfg);

  if (options.method == RandMethod::Pseudo) {
    return pluss::series::rand({rows, cols}, out_cfg, effective_seed_(options, compat_seed, bump));
  }

  const std::uint64_t off = effective_offset_(options, 0x1ULL);
  const std::uint64_t seed = effective_seed_(options, compat_seed, 0x1ULL);

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
      return quasi_block_([&](std::initializer_list<std::int64_t> sz) { return pluss::subsp::stats::rng::sobol(sz, o, out_cfg); }, rows, cols, block);
    }
    case RandMethod::Halton: {
      HaltonOptions o;
      o.init = true;
      o.normal = false;
      o.mixed = options.randomized_quasi;
      o.start = off + 1U;
      o.seed = seed;
      o.seed_set = true;
      return quasi_block_([&](std::initializer_list<std::int64_t> sz) { return pluss::subsp::stats::rng::halton(sz, o, out_cfg); }, rows, cols, block);
    }
    case RandMethod::Torus: {
      TorusOptions o;
      o.init = true;
      o.normal = false;
      o.mixed = options.randomized_quasi;
      o.start = off + 1U;
      o.seed = seed;
      o.seed_set = true;
      return quasi_block_([&](std::initializer_list<std::int64_t> sz) { return pluss::subsp::stats::rng::torus(sz, o, out_cfg); }, rows, cols, block);
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

// The uniform that drives the exponential branch of the quadratic-exponential step is taken from the same correlated normal shock as the quadratic branch,
// u = Phi(z), so that the variance keeps its correlation with the price (and, across assets, with the other variances) whichever branch a step falls in. u is
// exactly uniform, so the marginal law of the step is unchanged; an independent uniform would drop the correlation wherever the exponential branch applies.
Series uniform_from_normal_(const Series& z) { return z.mul(0.7071067811865476).erf().add(1.0).mul(0.5); }

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

  const std::vector<double> z_vec = z_var.reshape({paths}).to(cpu_cfg.platform, cpu_cfg.dtype).host<double>();
  const std::vector<double> v_vec = v_safe.reshape({paths}).to(cpu_cfg.platform, cpu_cfg.dtype).host<double>();
  std::mt19937_64 rng(seed ^ (0x94D049BB133111EBULL + static_cast<std::uint64_t>(t_step + 1) * 0x9E3779B97F4A7C15ULL));
  if (degree > 1.0) {
    // v' = c ((Z + sqrt(lambda))^2 + chi), lambda = v e^{-kappa dt} / c: the chi-squared part does not depend on v, so the step is written with ordinary
    // operations on the incoming variance and is differentiable in it
    std::gamma_distribution<double> chisq((degree - 1.0) * 0.5, 2.0);
    std::vector<double> chi(static_cast<std::size_t>(paths));
    for (std::int64_t i = 0; i < paths; ++i) chi[static_cast<std::size_t>(i)] = chisq(rng);
    const Series chi_series = pluss::make_series(chi, {1, paths}, cpu_cfg).to(cfg.platform, cfg.dtype);
    const Series lambda_v = v_safe.mul(exp_kdt / c).clamp_min(0.0);
    return z_var.reshape({1, paths}).add(lambda_v.sqrt()).square().add(chi_series).mul(c).clamp_min(p.variance_floor);
  }
  std::vector<double> out(static_cast<std::size_t>(paths), p.variance_floor);
  for (std::int64_t i = 0; i < paths; ++i) {
    const double lambda_v = std::max(0.0, (v_vec[static_cast<std::size_t>(i)] * exp_kdt) / c);
    std::poisson_distribution<std::int64_t> pois(0.5 * lambda_v);
    const std::int64_t n = pois(rng);
    std::gamma_distribution<double> chisq(0.5 * degree + static_cast<double>(n), 2.0);
    const double draw = c * chisq(rng);
    out[static_cast<std::size_t>(i)] = std::max(draw, p.variance_floor);
  }
  (void)z_vec;

  return pluss::make_series(out, {1, paths}, cpu_cfg).to(cfg.platform, cfg.dtype);
}

// Row of the initial level (variance, or forward variance for rough Bergomi) over the paths; differentiable in options.tracked_level when it is set.
Series level_row_(const SimOptions& options, double level, double floor, std::int64_t paths, Config cfg) {
  if (options.tracked_level == nullptr) return pluss::series::full({1, paths}, std::max(level, floor), cfg);
  return pluss::series::full({1, paths}, 1.0, cfg).mul(options.tracked_level->reshape({1, 1})).clamp_min(floor);
}

Series stack_rows_(const std::vector<Series>& rows, const char* op) {
  PLUSS_CHECK(!rows.empty(), std::string("subsp::math::finance::volatility::") + op + ": row history is empty");
  return pluss::series::pile(pluss::sign_list(rows), 0);
}

} // namespace

StochVolPaths detail::gbm_paths(std::int64_t steps,
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
  const Series z_spot = draw_normals_(steps, paths, options, out_cfg, seed, 0x7011ULL, DrawBlock{0, 1}, true);
  const Series z_var = pluss::series::zeros({steps, paths}, out_cfg);
  // a tracked volatility makes the paths differentiable in it
  const Series log_inc = options.tracked_level == nullptr
                             ? z_spot.mul(params.sigma * std::sqrt(dt)).add((params.mu - 0.5 * sigma2) * dt)
                             : z_spot.mul(options.tracked_level->reshape({1, 1}).mul(std::sqrt(dt))).add(options.tracked_level->reshape({1, 1}).square().mul(-0.5 * dt).add(params.mu * dt));
  const Series spot_tail = log_inc.cumsum(0, false).exp().mul(spot0);
  const Series spot = pluss::series::full({1, paths}, spot0, out_cfg).unite({spot_tail}, 0);
  const Series variance = pluss::series::full({n, paths}, sigma2, out_cfg);

  PLUSS_CHECK(spot.isfinite().all().item<double>() == 1.0,
              "subsp::math::finance::volatility::gbm_paths: spot path produced non-finite values");
  return finalize_paths_(spot, variance, z_spot, z_var, options, out_cfg);
}

StochVolPaths detail::gbm_paths(std::int64_t steps,
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

StochVolPaths detail::heston_paths(std::int64_t steps,
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

  const Series z_spot = draw_normals_(steps, paths, options, out_cfg, seed, 0x1021ULL, DrawBlock{0, 3}, true);
  const Series z_aux = draw_normals_(steps, paths, options, out_cfg, seed, 0x2303ULL, DrawBlock{1, 3});
  // The exact transition of the variance is sampled as (Z + sqrt(lambda))^2 + chi-squared, with Z the correlated shock, only when the
  // degree 4 kappa theta / sigma^2 of the non-central chi-squared law exceeds 1. Below that the exact sampler is a Poisson mixture that
  // cannot be driven by the correlated shock and would drop the correlation with the spot, so the quadratic-exponential step, which keeps
  // it, is used in that regime.
  const bool transition_uses_qe = (options.scheme == SimScheme::Transition) && (4.0 * params.theta * params.kappa <= params.sigma * params.sigma);
  const bool use_qe = (options.scheme == SimScheme::QuadExp) || transition_uses_qe;
  const std::uint64_t transition_seed = effective_seed_(options, seed, 0x5843ULL);

  const double rho_scale = std::sqrt(std::max(0.0, 1.0 - params.rho * params.rho));
  const Series z_var = z_spot.mul(params.rho).add(z_aux.mul(rho_scale));
  const Series u_qe = use_qe ? uniform_from_normal_(z_var) : pluss::series::zeros({steps, paths}, out_cfg);

  Series st = pluss::series::full({1, paths}, spot0, out_cfg);
  Series vt = level_row_(options, params.v0, params.variance_floor, paths, out_cfg);
  std::vector<Series> spot_rows;
  std::vector<Series> variance_rows;
  spot_rows.reserve(static_cast<std::size_t>(n));
  variance_rows.reserve(static_cast<std::size_t>(n));
  spot_rows.push_back(st.reshape({paths}));
  variance_rows.push_back(vt.reshape({paths}));

  for (std::int64_t t = 0; t < steps; ++t) {
    const Series eps_s = z_spot.select(0, t).reshape({1, paths});
    const Series eps_v = z_var.select(0, t).reshape({1, paths});
    const Series v_next = (options.scheme == SimScheme::Transition && !transition_uses_qe)
                              ? heston_transition_step_(vt, eps_v, dt, params, transition_seed, t)
                              : heston_variance_step_(vt,
                                                     eps_v,
                                                     u_qe.select(0, t).reshape({1, paths}),
                                                     dt,
                                                     params,
                                                     transition_uses_qe ? SimScheme::QuadExp : options.scheme);
    // The spot step uses the variance at the start of the step, which does not depend on this step's shocks, so that conditional on it
    // the step is exactly a martingale after discounting. Using the average of the start and end variances instead would make the
    // diffusion coefficient depend on the variance shock and add a spurious drift of rho * sigma / 4 per unit time.
    const Series v_start = vt.clamp_min(params.variance_floor);

    const Series log_inc = v_start.mul(-0.5 * dt)
                               .add(params.mu * dt)
                               .add(v_start.mul(dt).sqrt().mul(eps_s));
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

StochVolPaths detail::heston_paths(std::int64_t steps,
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

StochVolPaths detail::bates_paths(std::int64_t steps,
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

  const Series z_spot = draw_normals_(steps, paths, options, out_cfg, seed, 0xA129ULL, DrawBlock{0, 5}, true);
  const Series z_aux = draw_normals_(steps, paths, options, out_cfg, seed, 0xB273ULL, DrawBlock{1, 5});
  const bool transition_uses_qe = (options.scheme == SimScheme::Transition) && (4.0 * h_params.theta * h_params.kappa <= h_params.sigma * h_params.sigma);  // see heston_paths
  const bool use_qe = (options.scheme == SimScheme::QuadExp) || transition_uses_qe;
  const Series z_jump = draw_normals_(steps, paths, options, out_cfg, seed, 0xD401ULL, DrawBlock{3, 5});
  const Series u_jump = draw_uniforms_(steps, paths, options, out_cfg, seed, 0xE511ULL, DrawBlock{4, 5});
  const std::uint64_t transition_seed = effective_seed_(options, seed, 0xF713ULL);

  const double rho_scale = std::sqrt(std::max(0.0, 1.0 - params.rho * params.rho));
  const Series z_var = z_spot.mul(params.rho).add(z_aux.mul(rho_scale));
  const Series u_qe = use_qe ? uniform_from_normal_(z_var) : pluss::series::zeros({steps, paths}, out_cfg);
  const double p_jump = 1.0 - std::exp(-params.jump_intensity * dt);
  // `mu` is the expected growth rate of the spot, jumps included, as for the other generators: each step subtracts the log of the
  // expected jump factor 1 + p_jump * k, with k = E[e^J] - 1, so that E[S_T] = S_0 exp(mu T).
  const double mean_jump_factor = std::exp(params.jump_mean + 0.5 * params.jump_vol * params.jump_vol) - 1.0;
  const double jump_compensator = std::log1p(p_jump * mean_jump_factor);

  Series st = pluss::series::full({1, paths}, spot0, out_cfg);
  Series vt = level_row_(options, params.v0, params.variance_floor, paths, out_cfg);
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

    const Series v_next = (options.scheme == SimScheme::Transition && !transition_uses_qe)
                              ? heston_transition_step_(vt, eps_v, dt, h_params, transition_seed, t)
                              : heston_variance_step_(vt,
                                                     eps_v,
                                                     u_qe.select(0, t).reshape({1, paths}),
                                                     dt,
                                                     h_params,
                                                     transition_uses_qe ? SimScheme::QuadExp : options.scheme);
    const Series v_start = vt.clamp_min(params.variance_floor);  // see heston_paths

    const Series jump_mask = uj.le(p_jump);
    const Series jump_log = zj.mul(params.jump_vol).add(params.jump_mean);
    const Series jump_term = jump_mask.where(jump_log, pluss::series::zeros({1, paths}, out_cfg));
    const Series log_inc = v_start.mul(-0.5 * dt)
                               .add(params.mu * dt - jump_compensator)
                               .add(v_start.mul(dt).sqrt().mul(eps_s))
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

StochVolPaths detail::bates_paths(std::int64_t steps,
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

// ---------------------------------------------------------------------------------------------------------------------------------------
// Several assets in one pass. States are [A, paths] with the asset axis leading, so per-asset parameters are [A, 1] columns that broadcast.
// ---------------------------------------------------------------------------------------------------------------------------------------
namespace {

Series asset_column_(const Series& x, std::int64_t assets, Config cfg, const char* name) {
  PLUSS_CHECK(x.defined(), std::string("subsp::math::finance::module::Chord::simulate_paths: ") + name + " must be defined");
  Series v = x.to(cfg.platform, ScalarType::Float64);
  v = v.reshape({v.numel()});
  PLUSS_CHECK(v.numel() == 1 || v.numel() == assets,
              std::string("subsp::math::finance::module::Chord::simulate_paths: ") + name + " must have one element or one per asset");
  if (v.numel() == 1 && assets != 1) v = v.expand({assets});
  return v.reshape({assets, 1});
}

// Lower Cholesky factor of a correlation matrix, validated: symmetric, unit diagonal, positive definite.
Series correlation_factor_(const Series& r, std::int64_t assets, Config cfg, const char* name) {
  if (!r.defined()) return Series();
  Series m = r.to(cfg.platform, ScalarType::Float64);   // an [assets, assets] correlation matrix
  PLUSS_CHECK(m.sizes().size() == 2 && m.sizes()[0] == assets && m.sizes()[1] == assets,
              std::string("subsp::math::finance::module::Chord::simulate_paths: ") + name + " must be an [assets, assets] matrix");
  const auto host = m.contiguous().host<double>();
  for (std::int64_t a = 0; a < assets; ++a) {
    PLUSS_CHECK(std::abs(host[static_cast<std::size_t>(a * assets + a)] - 1.0) < 1e-9,
                std::string("subsp::math::finance::module::Chord::simulate_paths: ") + name + " must have a unit diagonal");
    for (std::int64_t b = 0; b < a; ++b) {
      PLUSS_CHECK(std::abs(host[static_cast<std::size_t>(a * assets + b)] - host[static_cast<std::size_t>(b * assets + a)]) < 1e-9,
                  std::string("subsp::math::finance::module::Chord::simulate_paths: ") + name + " must be symmetric");
    }
  }
  try {
    return pluss::subsp::linearalg::cholesky(m);
  } catch (const std::exception&) {
    PLUSS_CHECK(false, std::string("subsp::math::finance::module::Chord::simulate_paths: ") + name + " must be positive definite");
  }
  return Series();
}

struct VarianceColumns_ {
  Series kappa, theta, sigma;
  double floor;
};

// The variance schemes of heston_variance_step_ with per-asset parameter columns ([A, 1]) against states [A, paths].
Series multi_variance_step_(const Series& vt, const Series& z_var, const Series& u_qe, double dt, const VarianceColumns_& p, SimScheme scheme) {
  const Series v_safe = vt.clamp_min(p.floor);
  const Series sqrt_v = v_safe.sqrt();
  if (scheme == SimScheme::Euler) {
    return vt.sub(vt.mul(p.kappa).mul(dt)).add(p.kappa.mul(p.theta).mul(dt)).add(sqrt_v.mul(p.sigma).mul(z_var).mul(std::sqrt(dt))).clamp_min(p.floor);
  }
  if (scheme == SimScheme::Milstein || scheme == SimScheme::Milstein2) {
    const Series dW = z_var.mul(std::sqrt(dt));
    Series v_next = vt.sub(vt.mul(p.kappa).mul(dt)).add(p.kappa.mul(p.theta).mul(dt)).add(sqrt_v.mul(p.sigma).mul(dW))
                        .add(dW.square().sub(dt).mul(p.sigma.square().mul(0.25)));
    if (scheme == SimScheme::Milstein2) v_next = v_next.add(sqrt_v.mul(p.kappa).mul(p.sigma).mul(dW).mul(0.5 * dt));
    return v_next.clamp_min(p.floor);
  }
  PLUSS_CHECK(scheme == SimScheme::QuadExp,
              "subsp::math::finance::module::Chord::simulate_paths: the scheme must be Euler, Milstein, Milstein2 or QuadExp (Transition needs one asset at a time)");
  const Series exp_kdt = p.kappa.mul(-dt).exp();
  const Series one_minus_exp = exp_kdt.mul(-1.0).add(1.0);
  const Series sigma2 = p.sigma.square();
  const Series m = p.theta.add(vt.sub(p.theta).mul(exp_kdt));
  const Series s2 = vt.mul(sigma2).mul(exp_kdt).mul(one_minus_exp).div(p.kappa.clamp_min(kEps))
                        .add(p.theta.mul(sigma2).mul(one_minus_exp.square()).div(p.kappa.mul(2.0).clamp_min(kEps)));
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
  const Series v2 = u.le(p_zero).where(v1.mul(0.0), v2_nonzero);
  return use_q1.where(v1, v2).clamp_min(p.floor);
}

}  // namespace

MultiSvPaths detail::multi_sv_paths(std::int64_t steps, std::int64_t paths, double dt, const MultiSvParams& params, const SimOptions& options, Config cfg, std::uint64_t seed) {
  constexpr const char* op = "multi_sv_paths";
  PLUSS_CHECK(steps > 0 && paths > 0, "subsp::math::finance::module::Chord::simulate_paths: steps and paths must be positive");
  PLUSS_CHECK(std::isfinite(dt) && dt > 0.0, "subsp::math::finance::module::Chord::simulate_paths: dt must be finite and positive");
  validate_options_(options, op);
  const auto out_cfg = f64_cfg_(cfg);
  PLUSS_CHECK(params.spot0.defined(), "subsp::math::finance::module::Chord::simulate_paths: spot0 must be defined");
  const std::int64_t assets = std::max<std::int64_t>({params.spot0.numel(), params.kappa.defined() ? params.kappa.numel() : 1, params.v0.defined() ? params.v0.numel() : 1,
                                                      params.theta.defined() ? params.theta.numel() : 1, params.sigma.defined() ? params.sigma.numel() : 1,
                                                      params.rho.defined() ? params.rho.numel() : 1, params.growth.defined() ? params.growth.numel() : 1});
  const Series spot0 = asset_column_(params.spot0, assets, out_cfg, "spot0"), growth = asset_column_(params.growth, assets, out_cfg, "growth");
  const Series kappa = asset_column_(params.kappa, assets, out_cfg, "kappa"), theta = asset_column_(params.theta, assets, out_cfg, "theta");
  const Series sigma = asset_column_(params.sigma, assets, out_cfg, "sigma"), rho = asset_column_(params.rho, assets, out_cfg, "rho");
  const Series v0 = asset_column_(params.v0, assets, out_cfg, "v0");
  const bool mixed = !params.kind.empty();
  PLUSS_CHECK(!mixed || static_cast<std::int64_t>(params.kind.size()) == assets, "subsp::math::finance::module::Chord::simulate_paths: one model kind per asset is required");
  std::vector<std::int64_t> sv_idx, bs_idx, sabr_idx, rh_idx, rb_idx;
  for (std::int64_t a = 0; a < assets; ++a) {
    const MultiKind k = mixed ? params.kind[static_cast<std::size_t>(a)] : MultiKind::Sv;
    (k == MultiKind::BlackScholes ? bs_idx : k == MultiKind::Sabr ? sabr_idx : k == MultiKind::RoughHeston ? rh_idx : k == MultiKind::RoughBergomi ? rb_idx : sv_idx).push_back(a);
  }
  const auto idx_series = [](const std::vector<std::int64_t>& v) { return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}); };
  const bool all_sv = sv_idx.size() == static_cast<std::size_t>(assets);
  // the part of a per-asset column ([A, 1]) or of a [steps, A, paths] shock that belongs to the given assets
  const auto pick_col = [&](const Series& c, const std::vector<std::int64_t>& idx) { return c.index_select(0, idx_series(idx)); };
  const auto pick_z = [&](const Series& z, const std::vector<std::int64_t>& idx) { return z.index_select(1, idx_series(idx)); };
  const auto only = [&](const Series& c, const std::vector<std::int64_t>& idx, bool all) { return all ? c : pick_col(c, idx); };
  if (!sv_idx.empty()) {
    const Series kappa_g = only(kappa, sv_idx, all_sv), theta_g = only(theta, sv_idx, all_sv), sigma_g = only(sigma, sv_idx, all_sv), v0_g = only(v0, sv_idx, all_sv);
    for (const auto& [x, what] : {std::pair<const Series*, const char*>{&kappa_g, "kappa"}, {&theta_g, "theta"}, {&sigma_g, "sigma"}}) {
      PLUSS_CHECK(x->gt(0.0).all().item<bool>(), std::string("subsp::math::finance::module::Chord::simulate_paths: ") + what + " must be > 0");
    }
    PLUSS_CHECK(v0_g.ge(0.0).all().item<bool>(), "subsp::math::finance::module::Chord::simulate_paths: v0 must be >= 0");
  }
  PLUSS_CHECK(rho.gt(-1.0).all().item<bool>() && rho.lt(1.0).all().item<bool>(), "subsp::math::finance::module::Chord::simulate_paths: rho must lie in (-1, 1)");
  PLUSS_CHECK(spot0.gt(0.0).all().item<bool>(), "subsp::math::finance::module::Chord::simulate_paths: spot0 must be > 0");
  const bool jumps = params.jump_intensity.defined();
  Series lambda, jmean, jvol;
  if (jumps) {
    lambda = asset_column_(params.jump_intensity, assets, out_cfg, "jump_intensity");
    jmean = asset_column_(params.jump_mean, assets, out_cfg, "jump_mean");
    jvol = asset_column_(params.jump_vol, assets, out_cfg, "jump_vol");
    PLUSS_CHECK(lambda.ge(0.0).all().item<bool>() && jvol.ge(0.0).all().item<bool>(), "subsp::math::finance::module::Chord::simulate_paths: the jump intensity and volatility must be >= 0");
  }
  const bool joint = params.price_variance_correlation.defined();
  const Series L_s = joint ? Series() : correlation_factor_(params.price_correlation, assets, out_cfg, "price_corr");
  // var_corr is the correlation of the variance shocks themselves. Without an explicit cross correlation each variance shock is its own price shock
  // times rho plus an independent remainder, dW^v = rho dW^S + sqrt(1 - rho^2) dW^perp, so the cross terms are corr(price a, variance b) = rho_b R_s[a, b], and the
  // remainders must carry the correlation  R_perp[a, b] = (R_v[a, b] - rho_a rho_b R_s[a, b]) / (sqrt(1 - rho_a^2) sqrt(1 - rho_b^2)).
  Series L_v;
  if (!joint && params.variance_correlation.defined()) {
    correlation_factor_(params.variance_correlation, assets, out_cfg, "var_corr");   // validates R_v itself: unit diagonal, symmetric, positive definite
    const auto host = [&](const Series& m) {
      const auto h = m.to(pluss::pCPU, ScalarType::Float64).contiguous().host<double>();
      return std::vector<double>(h.begin(), h.end());
    };
    const auto Rv = host(params.variance_correlation);
    const auto rho_h = host(rho);
    std::vector<double> Rs(static_cast<std::size_t>(assets * assets), 0.0);
    if (params.price_correlation.defined()) {
      Rs = host(params.price_correlation);
    } else {
      for (std::int64_t a = 0; a < assets; ++a) Rs[static_cast<std::size_t>(a * assets + a)] = 1.0;
    }
    std::vector<double> perp(static_cast<std::size_t>(assets * assets), 0.0);
    for (std::int64_t a = 0; a < assets; ++a) {
      for (std::int64_t b = 0; b < assets; ++b) {
        const auto i = static_cast<std::size_t>(a * assets + b);
        const double ra = rho_h[static_cast<std::size_t>(a)], rb = rho_h[static_cast<std::size_t>(b)];
        perp[i] = a == b ? 1.0 : (Rv[i] - ra * rb * Rs[i]) / (std::sqrt(1.0 - ra * ra) * std::sqrt(1.0 - rb * rb));
      }
    }
    L_v = correlation_factor_(pluss::make_series(perp, {assets, assets}, out_cfg), assets, out_cfg,
                              "the correlation of the independent remainders of the variance shocks implied by var_corr, price_corr and rho (var_corr is too far from what the price correlation and rho allow; give price_var_cross to set the cross terms yourself)");
  }
  // With an explicit cross correlation the price and variance shocks of all assets are one 2A-dimensional Gaussian vector with the correlation matrix
  //   [[R_s, C], [C', R_v]],  C[a][a] = rho_a,  and R_v the correlation of the variance shocks themselves.
  Series L_joint;
  if (joint) {
    const auto host_matrix = [&](const Series& m, const char* name) -> std::vector<double> {
      if (!m.defined()) {
        std::vector<double> eye(static_cast<std::size_t>(assets * assets), 0.0);
        for (std::int64_t a = 0; a < assets; ++a) eye[static_cast<std::size_t>(a * assets + a)] = 1.0;
        return eye;
      }
      PLUSS_CHECK(m.sizes().size() == 2 && m.sizes()[0] == assets && m.sizes()[1] == assets,
                  std::string("subsp::math::finance::module::Chord::simulate_paths: ") + name + " must be an [assets, assets] matrix");
      const auto h = m.to(pluss::pCPU, ScalarType::Float64).contiguous().host<double>();
      return std::vector<double>(h.begin(), h.end());
    };
    const auto Rs = host_matrix(params.price_correlation, "price_corr"), Rv = host_matrix(params.variance_correlation, "var_corr"),
               C = host_matrix(params.price_variance_correlation, "price_var_cross");
    const auto rho_host_v = rho.to(pluss::pCPU, ScalarType::Float64).contiguous().host<double>();
    const std::vector<double> rho_host(rho_host_v.begin(), rho_host_v.end());
    const std::int64_t n2 = 2 * assets;
    std::vector<double> big(static_cast<std::size_t>(n2 * n2), 0.0);
    for (std::int64_t a = 0; a < assets; ++a) {
      for (std::int64_t b = 0; b < assets; ++b) {
        const double cross = a == b ? rho_host[static_cast<std::size_t>(a)] : C[static_cast<std::size_t>(a * assets + b)];
        big[static_cast<std::size_t>(a * n2 + b)] = Rs[static_cast<std::size_t>(a * assets + b)];
        big[static_cast<std::size_t>((assets + a) * n2 + assets + b)] = Rv[static_cast<std::size_t>(a * assets + b)];
        big[static_cast<std::size_t>(a * n2 + assets + b)] = cross;
        big[static_cast<std::size_t>((assets + b) * n2 + a)] = cross;
      }
    }
    L_joint = correlation_factor_(pluss::make_series(big, {n2, n2}, out_cfg), n2, out_cfg, "the joint correlation of the price and variance shocks (price_corr, var_corr, price_var_cross and rho)");
  }
  const bool use_qe = options.scheme == SimScheme::QuadExp;

  // independent standard normals, one block per asset (so that antithetic pairs stay within an asset), then correlated across assets with the Cholesky factor
  const auto draw_assets = [&](std::uint64_t bump, std::int64_t slot, std::int64_t slots, bool uniform, bool shift) {
    std::vector<Series> blocks;
    for (std::int64_t a = 0; a < assets; ++a) {
      const DrawBlock block{a * slots + slot, assets * slots};
      blocks.push_back(uniform ? draw_uniforms_(steps, paths, options, out_cfg, seed, bump + static_cast<std::uint64_t>(a) * 0x9E37ULL, block)
                               : draw_normals_(steps, paths, options, out_cfg, seed, bump + static_cast<std::uint64_t>(a) * 0x9E37ULL, block, shift));
    }
    return pluss::unite(pluss::msharp::Lister::Series([&] { std::vector<Series> v; for (auto& b : blocks) v.push_back(b.reshape({steps, 1, paths})); return v; }()), 1);   // [steps, A, paths]
  };
  const auto mix = [&](const Series& z, const Series& factor) { return factor.defined() ? factor.unsqueeze(0).matmul(z) : z; };
  Series z_price, z_var;
  if (joint) {
    const Series both = L_joint.unsqueeze(0).matmul(pluss::unite({draw_assets(0x1021ULL, 0, 5, false, true), draw_assets(0x2303ULL, 1, 5, false, false)}, 1));
    z_price = both.slice(1, 0, assets);
    z_var = both.slice(1, assets, 2 * assets);
  } else {
    z_price = mix(draw_assets(0x1021ULL, 0, 5, false, true), L_s);
    const Series z_ind = mix(draw_assets(0x2303ULL, 1, 5, false, false), L_v);
    z_var = z_price.mul(rho.unsqueeze(0)).add(z_ind.mul(rho.square().neg().add(1.0).clamp_min(0.0).sqrt().unsqueeze(0)));
  }
  const Series u_qe = use_qe ? uniform_from_normal_(z_var) : pluss::series::zeros({steps, assets, paths}, out_cfg);
  Series z_jump, u_jump;
  if (jumps) {
    z_jump = draw_assets(0xD401ULL, 3, 5, false, false);
    u_jump = draw_assets(0xE511ULL, 4, 5, true, false);
  }
  Series p_jump, compensator;
  if (jumps) {
    p_jump = lambda.mul(-dt).exp().neg().add(1.0);
    compensator = p_jump.mul(jmean.add(jvol.square().mul(0.5)).exp().sub(1.0)).log1p();
  }

  // [steps + 1 rows of [G, paths]] -> [paths, steps + 1, G]
  const auto lay = [&](const std::vector<Series>& rows) {
    return pluss::unite(pluss::msharp::Lister::Series([&] { std::vector<Series> v; for (const auto& r : rows) v.push_back(r.unsqueeze(0)); return v; }()), 0).transpose(0, 2).transpose(1, 2).contiguous();
  };
  const auto lay_shock = [&](const Series& z) { return z.transpose(0, 2).transpose(1, 2).contiguous(); };   // [steps, A, paths] -> [paths, steps, A]
  const double sqdt = std::sqrt(dt);

  struct Group { std::vector<std::int64_t> idx; Series spot, variance; };   // spot and variance as [paths, steps + 1, G]
  std::vector<Group> groups;

  // ---- Heston and Bates assets
  if (!sv_idx.empty()) {
    const Series growth_g = only(growth, sv_idx, all_sv), spot0_g = only(spot0, sv_idx, all_sv), v0_g = only(v0, sv_idx, all_sv);
    const VarianceColumns_ cols{only(kappa, sv_idx, all_sv), only(theta, sv_idx, all_sv), only(sigma, sv_idx, all_sv), params.variance_floor};
    const std::int64_t n_sv = static_cast<std::int64_t>(sv_idx.size());
    const Series zp = all_sv ? z_price : pick_z(z_price, sv_idx), zv = all_sv ? z_var : pick_z(z_var, sv_idx), uq = all_sv ? u_qe : pick_z(u_qe, sv_idx);
    Series jvol_g, jmean_g, p_jump_g, compensator_g, zj_g, uj_g;
    if (jumps) {
      jvol_g = only(jvol, sv_idx, all_sv);
      jmean_g = only(jmean, sv_idx, all_sv);
      p_jump_g = only(p_jump, sv_idx, all_sv);
      compensator_g = only(compensator, sv_idx, all_sv);
      zj_g = all_sv ? z_jump : pick_z(z_jump, sv_idx);
      uj_g = all_sv ? u_jump : pick_z(u_jump, sv_idx);
    }
    Series st = spot0_g.add(pluss::series::zeros({n_sv, paths}, out_cfg));
    Series vt = v0_g.clamp_min(params.variance_floor).add(pluss::series::zeros({n_sv, paths}, out_cfg));
    std::vector<Series> spot_rows{st}, variance_rows{vt};
    for (std::int64_t t = 0; t < steps; ++t) {
      const Series eps_s = zp.select(0, t), eps_v = zv.select(0, t);
      const Series v_next = multi_variance_step_(vt, eps_v, uq.select(0, t), dt, cols, options.scheme);
      const Series v_start = vt.clamp_min(params.variance_floor);
      Series log_inc = v_start.mul(-0.5 * dt).add(growth_g.mul(dt)).add(v_start.mul(dt).sqrt().mul(eps_s));
      if (jumps) {
        const Series jump_log = zj_g.select(0, t).mul(jvol_g).add(jmean_g);
        log_inc = log_inc.sub(compensator_g).add(uj_g.select(0, t).le(p_jump_g).where(jump_log, jump_log.mul(0.0)));
      }
      st = st.mul(log_inc.exp()).clamp_min(params.variance_floor).clamp_max(1.0e12);
      vt = v_next;
      spot_rows.push_back(st);
      variance_rows.push_back(vt);
    }
    groups.push_back({sv_idx, lay(spot_rows), lay(variance_rows)});
  }

  // ---- Black-Scholes assets: the exact lognormal step with a constant variance
  if (!bs_idx.empty()) {
    PLUSS_CHECK(params.bs_vol.defined(), "subsp::math::finance::module::Chord::simulate_paths: the Black-Scholes volatilities must be defined");
    const Series vol = pick_col(asset_column_(params.bs_vol, assets, out_cfg, "bs_vol"), bs_idx);
    PLUSS_CHECK(vol.isfinite().all().item<bool>() && vol.ge(0.0).all().item<bool>(), "subsp::math::finance::module::Chord::simulate_paths: a Black-Scholes volatility must be finite and >= 0");
    const std::int64_t n_bs = static_cast<std::int64_t>(bs_idx.size());
    const Series drift = pick_col(growth, bs_idx).sub(vol.square().mul(0.5)).mul(dt), diffusion = vol.mul(sqdt);
    const Series zp = pick_z(z_price, bs_idx);
    Series st = pick_col(spot0, bs_idx).add(pluss::series::zeros({n_bs, paths}, out_cfg));
    const Series var_row = vol.square().add(pluss::series::zeros({n_bs, paths}, out_cfg));
    std::vector<Series> spot_rows{st}, variance_rows{var_row};
    for (std::int64_t t = 0; t < steps; ++t) {
      st = st.mul(zp.select(0, t).mul(diffusion).add(drift).exp());
      spot_rows.push_back(st);
      variance_rows.push_back(var_row);
    }
    groups.push_back({bs_idx, lay(spot_rows), lay(variance_rows)});
  }

  // ---- SABR assets: the forward with a CEV diffusion and the exact lognormal step of alpha (as sabr_paths does for one asset); the variance returned is alpha squared
  if (!sabr_idx.empty()) {
    PLUSS_CHECK(params.alpha0.defined() && params.beta.defined() && params.nu.defined(), "subsp::math::finance::module::Chord::simulate_paths: the SABR parameters must be defined");
    const Series a0 = pick_col(asset_column_(params.alpha0, assets, out_cfg, "alpha0"), sabr_idx), beta = pick_col(asset_column_(params.beta, assets, out_cfg, "beta"), sabr_idx),
                 nu = pick_col(asset_column_(params.nu, assets, out_cfg, "nu"), sabr_idx);
    PLUSS_CHECK(a0.gt(0.0).all().item<bool>() && a0.isfinite().all().item<bool>(), "subsp::math::finance::module::Chord::simulate_paths: alpha0 must be finite and > 0");
    PLUSS_CHECK(beta.ge(0.0).all().item<bool>() && beta.le(1.0).all().item<bool>(), "subsp::math::finance::module::Chord::simulate_paths: beta must be in [0, 1]");
    PLUSS_CHECK(nu.ge(0.0).all().item<bool>() && nu.isfinite().all().item<bool>(), "subsp::math::finance::module::Chord::simulate_paths: nu must be finite and >= 0");
    const std::int64_t n_sa = static_cast<std::int64_t>(sabr_idx.size());
    const double floor = params.alpha_floor;
    const Series mu = pick_col(growth, sabr_idx), f0 = pick_col(spot0, sabr_idx);
    const Series positive = beta.gt(0.0).to(ScalarType::Float64);   // [G, 1]: 1 where the forward stays positive
    const Series zp = pick_z(z_price, sabr_idx), za = pick_z(z_var, sabr_idx);
    const Series zeros_gp = pluss::series::zeros({n_sa, paths}, out_cfg);
    Series ft = f0.add(zeros_gp), at = a0.add(zeros_gp);
    std::vector<Series> forward_rows{ft}, alpha_rows{at};
    const bool milstein = options.scheme == SimScheme::Milstein || options.scheme == SimScheme::Milstein2;
    for (std::int64_t t = 0; t < steps; ++t) {
      const Series f_c = ft.clamp_min(floor).mul(positive).add(ft.mul(positive.neg().add(1.0)));
      const Series a_c = at.clamp_min(floor);
      const Series eps_f = zp.select(0, t), eps_a = za.select(0, t);
      const Series alpha_log_inc = eps_a.mul(nu.mul(sqdt)).add(nu.square().mul(-0.5 * dt)).clamp_min(-30.0).clamp_max(30.0);
      const Series a_next = a_c.mul(alpha_log_inc.exp()).clamp_min(floor).clamp_max(1.0e6);
      const Series log_f = f_c.clamp_min(floor).log();
      const Series sigma_f = a_c.mul(log_f.mul(beta).exp());
      Series f_next = f_c.add(f_c.mul(mu.mul(dt))).add(sigma_f.mul(sqdt).mul(eps_f));
      if (milstein) {
        f_next = f_next.add(a_c.square().mul(beta.mul(0.5 * dt)).mul(log_f.mul(beta.mul(2.0).sub(1.0)).exp()).mul(eps_f.square().sub(1.0)));
      }
      const Series f_safe = f_next.isfinite().where(f_next, f0.add(zeros_gp));
      const Series f_bounded = f_safe.clamp_min(floor).clamp_max(1.0e12).mul(positive).add(f_safe.clamp_min(-1.0e12).clamp_max(1.0e12).mul(positive.neg().add(1.0)));
      at = a_next;
      ft = f_bounded;
      forward_rows.push_back(ft);
      alpha_rows.push_back(at);
    }
    groups.push_back({sabr_idx, lay(forward_rows), lay(alpha_rows).square()});
  }


  // ---- rough Heston assets: v_t = v0 + int K(t - s) dY_s with K(u) = u^alpha / Gamma(alpha + 1), each increment of Y weighted by the kernel averaged over its cell
  if (!rh_idx.empty()) {
    PLUSS_CHECK(params.rough_alpha.defined(), "subsp::math::finance::module::Chord::simulate_paths: the rough alpha (the Hurst exponent minus 1/2) must be defined");
    const Series alpha = pick_col(asset_column_(params.rough_alpha, assets, out_cfg, "alpha"), rh_idx);
    PLUSS_CHECK(alpha.gt(-0.5).all().item<bool>() && alpha.lt(0.0).all().item<bool>(), "subsp::math::finance::module::Chord::simulate_paths: a rough Heston alpha must be in (-0.5, 0)");
    const std::int64_t n_rh = static_cast<std::int64_t>(rh_idx.size());
    const Series kappa_g = pick_col(kappa, rh_idx), theta_g = pick_col(theta, rh_idx), sigma_g = pick_col(sigma, rh_idx), v0_g = pick_col(v0, rh_idx).clamp_min(params.variance_floor);
    PLUSS_CHECK(kappa_g.gt(0.0).all().item<bool>() && theta_g.gt(0.0).all().item<bool>() && sigma_g.gt(0.0).all().item<bool>(), "subsp::math::finance::module::Chord::simulate_paths: a rough Heston kappa, theta and sigma must be > 0");
    const auto alpha_h = alpha.to(pluss::pCPU, ScalarType::Float64).contiguous().host<double>();
    std::vector<double> kern(static_cast<std::size_t>(n_rh * steps), 0.0);
    for (std::int64_t g = 0; g < n_rh; ++g) {
      const double al = alpha_h[static_cast<std::size_t>(g)], scale = std::pow(dt, al) / std::tgamma(al + 2.0);
      for (std::int64_t k = 0; k < steps; ++k) {
        const double hi = std::pow(static_cast<double>(k + 1), al + 1.0), lo = k == 0 ? 0.0 : std::pow(static_cast<double>(k), al + 1.0);
        kern[static_cast<std::size_t>(g * steps + k)] = scale * (hi - lo);
      }
    }
    const Series kernel = pluss::make_series(kern, {n_rh, steps}, out_cfg);
    const Series zp = pick_z(z_price, rh_idx), zv = pick_z(z_var, rh_idx);
    const Series kernel_flat = kernel.unsqueeze(1).expand({n_rh, paths, steps}).reshape({n_rh * paths, steps}).contiguous();   // one kernel row per (asset, path)
    Series hist = pluss::series::zeros({n_rh * paths, steps}, out_cfg);
    Series st = pick_col(spot0, rh_idx).add(pluss::series::zeros({n_rh, paths}, out_cfg));
    Series vt = v0_g.add(pluss::series::zeros({n_rh, paths}, out_cfg));
    const Series growth_g = pick_col(growth, rh_idx);
    std::vector<Series> spot_rows{st}, variance_rows{vt};
    for (std::int64_t t = 0; t < steps; ++t) {
      st = st.clamp_min(params.variance_floor);
      vt = vt.clamp_min(params.variance_floor);
      const Series eps_s = zp.select(0, t), eps_v = zv.select(0, t);
      const Series drive = kappa_g.mul(theta_g).mul(dt).sub(vt.mul(kappa_g.mul(dt))).add(vt.sqrt().mul(sigma_g.mul(sqdt)).mul(eps_v));
      hist.index_put_({pluss::Slicing::SliceAll(), t}, drive.reshape({n_rh * paths}));
      const Series prefix = hist.slice(1, 0, t + 1);
      const Series k_rev = kernel_flat.slice(1, 0, t + 1).flip(1);
      const Series frac = prefix.mul(k_rev).sum(1, false).reshape({n_rh, paths});
      const Series v_next = v0_g.add(frac).clamp_min(params.variance_floor);
      const Series log_inc = vt.mul(-0.5 * dt).add(growth_g.mul(dt)).add(vt.mul(dt).sqrt().mul(eps_s));
      st = st.mul(log_inc.exp()).clamp_min(params.variance_floor).clamp_max(1.0e12);
      vt = v_next;
      spot_rows.push_back(st);
      variance_rows.push_back(vt);
    }
    groups.push_back({rh_idx, lay(spot_rows), lay(variance_rows)});
  }

  // ---- rough Bergomi assets: v_t = xi exp(eta Y_t - eta^2 t^(2H) / 2) with Y_t the Riemann-Liouville process, each normal weighted by the root of the kernel's variance over its cell
  if (!rb_idx.empty()) {
    PLUSS_CHECK(params.rough_alpha.defined() && params.xi.defined() && params.eta.defined(), "subsp::math::finance::module::Chord::simulate_paths: the rough Bergomi xi, eta and alpha must be defined");
    const Series alpha = pick_col(asset_column_(params.rough_alpha, assets, out_cfg, "alpha"), rb_idx), xi = pick_col(asset_column_(params.xi, assets, out_cfg, "xi"), rb_idx),
                 eta = pick_col(asset_column_(params.eta, assets, out_cfg, "eta"), rb_idx);
    PLUSS_CHECK(alpha.gt(-0.5).all().item<bool>() && alpha.lt(0.0).all().item<bool>(), "subsp::math::finance::module::Chord::simulate_paths: a rough Bergomi alpha must be in (-0.5, 0)");
    PLUSS_CHECK(xi.gt(0.0).all().item<bool>() && eta.gt(0.0).all().item<bool>(), "subsp::math::finance::module::Chord::simulate_paths: a rough Bergomi xi and eta must be > 0");
    const std::int64_t n_rb = static_cast<std::int64_t>(rb_idx.size());
    const Series hurst = alpha.add(0.5);
    const auto hurst_h = hurst.to(pluss::pCPU, ScalarType::Float64).contiguous().host<double>();
    std::vector<double> kern(static_cast<std::size_t>(n_rb * steps), 0.0);
    for (std::int64_t g = 0; g < n_rb; ++g) {
      const double H = hurst_h[static_cast<std::size_t>(g)], scale = std::pow(dt, H);
      for (std::int64_t k = 0; k < steps; ++k) {
        const double hi = std::pow(static_cast<double>(k + 1), 2.0 * H), lo = k == 0 ? 0.0 : std::pow(static_cast<double>(k), 2.0 * H);
        kern[static_cast<std::size_t>(g * steps + k)] = scale * std::sqrt(hi - lo);
      }
    }
    const Series kernel = pluss::make_series(kern, {n_rb, steps}, out_cfg);
    const Series zp = pick_z(z_price, rb_idx), zv = pick_z(z_var, rb_idx);
    const Series xi_row = xi.clamp_min(params.variance_floor).add(pluss::series::zeros({n_rb, paths}, out_cfg));
    const Series kernel_flat = kernel.unsqueeze(1).expand({n_rb, paths, steps}).reshape({n_rb * paths, steps}).contiguous();
    Series hist = pluss::series::zeros({n_rb * paths, steps}, out_cfg);
    Series st = pick_col(spot0, rb_idx).add(pluss::series::zeros({n_rb, paths}, out_cfg));
    Series vt = xi_row;
    const Series growth_g = pick_col(growth, rb_idx);
    const Series two_h = hurst.mul(2.0);
    std::vector<Series> spot_rows{st}, variance_rows{vt};
    for (std::int64_t t = 0; t < steps; ++t) {
      st = st.clamp_min(params.variance_floor);
      vt = vt.clamp_min(params.variance_floor);
      const Series eps_s = zp.select(0, t), eps_v = zv.select(0, t);
      hist.index_put_({pluss::Slicing::SliceAll(), t}, eps_v.reshape({n_rb * paths}));
      const Series log_inc = vt.mul(-0.5 * dt).add(growth_g.mul(dt)).add(vt.mul(dt).sqrt().mul(eps_s));
      const Series s_next = st.mul(log_inc.exp()).clamp_min(params.variance_floor).clamp_max(1.0e12);
      const Series prefix = hist.slice(1, 0, t + 1);
      const Series k_rev = kernel_flat.slice(1, 0, t + 1).flip(1);
      const Series y_next = prefix.mul(k_rev).sum(1, false).reshape({n_rb, paths});
      const double t_next = static_cast<double>(t + 1) * dt;
      const Series det_term = eta.square().mul(-0.5).mul(two_h.mul(std::log(t_next)).exp());
      const Series v_next = y_next.mul(eta).add(det_term).exp().mul(xi_row).clamp_min(params.variance_floor).clamp_max(1.0e6);
      st = s_next;
      vt = v_next;
      spot_rows.push_back(st);
      variance_rows.push_back(vt);
    }
    groups.push_back({rb_idx, lay(spot_rows), lay(variance_rows)});
  }

  // back to the order of the assets
  Series spot, variance;
  if (groups.size() == 1 && (all_sv || !mixed)) {
    spot = groups[0].spot;
    variance = groups[0].variance;
  } else {
    std::vector<Series> spots, variances;
    std::vector<std::int64_t> position(static_cast<std::size_t>(assets), 0);
    std::int64_t next = 0;
    for (const auto& g : groups) {
      spots.push_back(g.spot);
      variances.push_back(g.variance);
      for (const auto a : g.idx) position[static_cast<std::size_t>(a)] = next++;
    }
    const Series order = idx_series(position);
    spot = (spots.size() == 1 ? spots[0] : pluss::unite(pluss::msharp::Lister::Series(spots), 2)).index_select(2, order);
    variance = (variances.size() == 1 ? variances[0] : pluss::unite(pluss::msharp::Lister::Series(variances), 2)).index_select(2, order);
  }
  PLUSS_CHECK(spot.isfinite().all().item<bool>() && variance.isfinite().all().item<bool>(),
              "subsp::math::finance::module::Chord::simulate_paths: the simulation produced non-finite values");
  return MultiSvPaths{.spot = spot, .variance = variance, .innovation_spot = lay_shock(z_price), .innovation_var = lay_shock(z_var)};
}

StochVolPaths detail::sv_paths(std::int64_t steps,
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

  const Series z_spot = draw_normals_(steps, paths, options, out_cfg, seed, 0x7811ULL, DrawBlock{0, 2}, true);
  const Series z_aux = draw_normals_(steps, paths, options, out_cfg, seed, 0x9119ULL, DrawBlock{1, 2});
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

StochVolPaths detail::sv_paths(std::int64_t steps,
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

StochVolPaths detail::heston_nandi_paths(std::int64_t steps,
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

  const Series z_raw = draw_normals_(steps, paths, options, out_cfg, seed, 0xF311ULL, DrawBlock{0, 1}, true);
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

StochVolPaths detail::heston_nandi_paths(std::int64_t steps,
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

StochVolPaths detail::sabr_paths(std::int64_t steps,
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
  const Series z1 = draw_normals_(steps, paths, options, out_cfg, seed, 0xAA11ULL, DrawBlock{0, 2}, true);
  const Series z2 = draw_normals_(steps, paths, options, out_cfg, seed, 0xBB31ULL, DrawBlock{1, 2});
  const double rho_scale = std::sqrt(std::max(0.0, 1.0 - params.rho * params.rho));
  const Series z_alpha = z1.mul(params.rho).add(z2.mul(rho_scale));
  const double sqdt = std::sqrt(dt);
  const bool positive_forward = params.beta > 0.0;

  Series ft_state = options.tracked_spot == nullptr ? pluss::series::full({1, paths}, forward0, out_cfg) : options.tracked_spot->reshape({1, paths}).mul(1.0);
  Series at_state = level_row_(options, params.alpha0, params.alpha_floor, paths, out_cfg);
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
    const Series drift_term = options.tracked_drift == nullptr ? ft.mul(params.mu * dt) : ft.mul(options.tracked_drift->reshape({1, 1}).mul(dt));
    Series f_next = ft.add(drift_term).add(sigma_f.mul(sqdt).mul(eps_f));

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

StochVolPaths detail::sabr_paths(std::int64_t steps,
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

StochVolPaths detail::slv_paths(std::int64_t steps,
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
  const Series z_spot = draw_normals_(steps, paths, options, out_cfg, seed, 0xCC17ULL, DrawBlock{0, 2}, true);
  const Series z_aux = draw_normals_(steps, paths, options, out_cfg, seed, 0xDD29ULL, DrawBlock{1, 2});
  const double rho_scale = std::sqrt(std::max(0.0, 1.0 - params.rho * params.rho));
  const Series z_var = z_spot.mul(params.rho).add(z_aux.mul(rho_scale));

  Series st = pluss::series::full({1, paths}, spot0, out_cfg);
  Series vt = level_row_(options, params.v0, params.variance_floor, paths, out_cfg);
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

StochVolPaths detail::slv_paths(std::int64_t steps,
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

StochVolPaths detail::rough_heston_paths(std::int64_t steps,
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

  const Series z_spot = draw_normals_(steps, paths, options, out_cfg, seed, 0xAB11ULL, DrawBlock{0, 2}, true);
  const Series z_aux = draw_normals_(steps, paths, options, out_cfg, seed, 0xCD29ULL, DrawBlock{1, 2});
  const double rho_scale = std::sqrt(std::max(0.0, 1.0 - params.rho * params.rho));
  const Series z_var = z_spot.mul(params.rho).add(z_aux.mul(rho_scale));

  // The variance is v0 + int_0^t K(t - s) dY_s with K(u) = u^alpha / Gamma(alpha + 1) and dY = kappa (theta - v) ds + sigma sqrt(v) dW. Each
  // increment of Y is weighted by the kernel averaged over its cell, dt^alpha ((k + 1)^(alpha + 1) - k^(alpha + 1)) / Gamma(alpha + 2),
  // which is 1 at alpha = 0 (ordinary Heston).
  std::vector<double> kernel(static_cast<std::size_t>(steps), 0.0);
  const double kernel_scale = std::pow(dt, params.alpha) / std::tgamma(params.alpha + 2.0);
  for (std::int64_t k = 0; k < steps; ++k) {
    const double a = std::pow(static_cast<double>(k + 1), params.alpha + 1.0);
    const double b = (k == 0) ? 0.0 : std::pow(static_cast<double>(k), params.alpha + 1.0);
    kernel[static_cast<std::size_t>(k)] = kernel_scale * (a - b);
  }
  const Series kernel_series = pluss::make_series(kernel, {steps}, out_cfg);
  Series drives_hist = pluss::series::zeros({paths, steps}, out_cfg);

  Series st = pluss::series::full({1, paths}, spot0, out_cfg);
  Series vt = level_row_(options, params.v0, params.variance_floor, paths, out_cfg);
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
        level_row_(options, params.v0, params.variance_floor, paths, out_cfg).add(frac_acc).clamp_min(params.variance_floor);
    const Series log_inc = vt.mul(-0.5 * dt).add(params.mu * dt).add(vt.mul(dt).sqrt().mul(eps_s));  // vt is clamped at the top of the step
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

StochVolPaths detail::rough_heston_paths(std::int64_t steps,
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

StochVolPaths detail::rough_bergomi_paths(std::int64_t steps,
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

  const Series z_spot = draw_normals_(steps, paths, options, out_cfg, seed, 0xD519ULL, DrawBlock{0, 2}, true);
  const Series z_aux = draw_normals_(steps, paths, options, out_cfg, seed, 0xF731ULL, DrawBlock{1, 2});
  const double rho_scale = std::sqrt(std::max(0.0, 1.0 - params.rho * params.rho));
  const Series z_var = z_spot.mul(params.rho).add(z_aux.mul(rho_scale));

  // Y_t = sqrt(2H) int_0^t (t - s)^(H - 1/2) dW_s has variance t^(2H). The weight of each standard normal is the square root of the kernel's
  // variance over its cell, dt^H sqrt((k + 1)^(2H) - k^(2H)), so Var(Y) = t^(2H) exactly and exp(eta Y - eta^2 t^(2H) / 2) has mean one.
  std::vector<double> kernel(static_cast<std::size_t>(steps), 0.0);
  const double kernel_scale = std::pow(dt, H);
  for (std::int64_t k = 0; k < steps; ++k) {
    const double a = std::pow(static_cast<double>(k + 1), 2.0 * H);
    const double b = (k == 0) ? 0.0 : std::pow(static_cast<double>(k), 2.0 * H);
    kernel[static_cast<std::size_t>(k)] = kernel_scale * std::sqrt(a - b);
  }
  const Series kernel_series = pluss::make_series(kernel, {steps}, out_cfg);
  Series dWv_hist = pluss::series::zeros({paths, steps}, out_cfg);

  Series st = pluss::series::full({1, paths}, spot0, out_cfg);
  const Series xi_row = level_row_(options, params.xi, params.variance_floor, paths, out_cfg);
  Series vt = xi_row;
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
    dWv_hist.index_put_({pluss::Slicing::SliceAll(), t}, eps_v.reshape({paths}));

    const Series log_inc = vt.mul(-0.5 * dt).add(params.mu * dt).add(vt.mul(dt).sqrt().mul(eps_s));
    const Series s_next = st.mul(log_inc.exp()).clamp_min(params.variance_floor).clamp_max(1.0e12);

    const Series dW_prefix = dWv_hist.slice(1, 0, t + 1);
    const Series k_rev = kernel_series.slice(0, 0, t + 1).flip(0).reshape({1, t + 1});
    Series y_next = dW_prefix.mul(k_rev).sum(1, false).reshape({1, paths});

    const double t_next = static_cast<double>(t + 1) * dt;
    const double det_term = -0.5 * params.eta * params.eta * std::pow(t_next, 2.0 * H);
    const Series v_next = y_next.mul(params.eta)
                              .add(det_term)
                              .exp()
                              .mul(xi_row)
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

StochVolPaths detail::rough_bergomi_paths(std::int64_t steps,
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
