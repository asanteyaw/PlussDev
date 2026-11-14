#include "pluss/subsp/math/finance/rates.h"

#include "CodeM/core/Errors.h"
#include "pluss/codem/series_factory.h"
#include "pluss/subsp/stats/rng.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <random>
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

constexpr double kEps = 1e-12;

Config f64_cfg_(Config cfg) {
  return Config{}.with_dtype(ScalarType::Float64).with_platform(cfg.platform).with_pinned(cfg.pinned);
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

// A low-discrepancy sequence has one point per path and one dimension per time step; a sequence whose points were the time steps would have
// thousands of dimensions and a handful of points. The draw is returned as {steps, paths}.
template <typename Sequence>
Series quasi_(Sequence&& sequence, std::int64_t rows, std::int64_t points) {
  return sequence({points, rows}).transpose(0, 1).contiguous();
}

Series draw_normals_(std::int64_t rows,
                     std::int64_t cols,
                     const SimOptions& options,
                     Config cfg,
                     std::uint64_t compat_seed,
                     std::uint64_t bump) {
  const auto out_cfg = f64_cfg_(cfg);

  if (options.method == RandMethod::Pseudo) {
    return pluss::series::randn({rows, cols}, out_cfg, effective_seed_(options, compat_seed, bump));
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
      return quasi_([&](std::initializer_list<std::int64_t> sz) { return pluss::subsp::stats::rng::sobol(sz, o, out_cfg); }, rows, cols);
    }
    case RandMethod::Halton: {
      HaltonOptions o;
      o.init = true;
      o.normal = true;
      o.mixed = options.randomized_quasi;
      o.start = off + 1U;
      o.seed = seed;
      o.seed_set = true;
      return quasi_([&](std::initializer_list<std::int64_t> sz) { return pluss::subsp::stats::rng::halton(sz, o, out_cfg); }, rows, cols);
    }
    case RandMethod::Torus: {
      TorusOptions o;
      o.init = true;
      o.normal = true;
      o.mixed = options.randomized_quasi;
      o.start = off + 1U;
      o.seed = seed;
      o.seed_set = true;
      return quasi_([&](std::initializer_list<std::int64_t> sz) { return pluss::subsp::stats::rng::torus(sz, o, out_cfg); }, rows, cols);
    }
    case RandMethod::Pseudo:
      break;
  }

  return pluss::subsp::stats::rng::standard_normal(
      {rows, cols}, to_rng_method_(options.method), out_cfg, off);
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

Series cir_transition_step_(const Series& rt,
                            const Series& z,
                            double dt,
                            const CirParams& p,
                            std::uint64_t seed,
                            std::int64_t t_step) {
  const Series r_safe = rt.clamp_min(p.rate_floor);
  const Config cfg = Config{}.with_dtype(r_safe.scalar_type()).with_platform(r_safe.platform());
  const Config cpu_cfg = Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);
  const auto shape = r_safe.sizes();
  const std::int64_t paths = (shape.size() == 2) ? shape[1] : r_safe.numel();
  PLUSS_CHECK(paths > 0, "subsp::math::finance::rates::cir_transition_step_: invalid path count");

  const double exp_kdt = std::exp(-p.kappa * dt);
  const double c = (p.sigma * p.sigma) * (1.0 - exp_kdt) / std::max(4.0 * p.kappa, kEps);
  const double degree = 4.0 * p.theta * p.kappa / std::max(p.sigma * p.sigma, kEps);
  PLUSS_CHECK(std::isfinite(c) && c > 0.0,
              "subsp::math::finance::rates::cir_transition_step_: invalid scale constant");
  PLUSS_CHECK(std::isfinite(degree) && degree > 0.0,
              "subsp::math::finance::rates::cir_transition_step_: invalid degree");

  const std::vector<double> r_vec = r_safe.reshape({paths}).to(cpu_cfg.platform, cpu_cfg.dtype).host<double>();
  const std::vector<double> z_vec = z.reshape({paths}).to(cpu_cfg.platform, cpu_cfg.dtype).host<double>();

  std::vector<double> out(static_cast<std::size_t>(paths), p.rate_floor);
  std::mt19937_64 rng(seed ^ (0xD1B54A32D192ED03ULL + static_cast<std::uint64_t>(t_step + 1) * 0x9E3779B97F4A7C15ULL));

  if (degree > 1.0) {
    std::gamma_distribution<double> chisq((degree - 1.0) * 0.5, 2.0);
    for (std::int64_t i = 0; i < paths; ++i) {
      const double lambda_v = std::max(0.0, (r_vec[static_cast<std::size_t>(i)] * exp_kdt) / c);
      const double chi = chisq(rng);
      const double draw = c * (std::pow(z_vec[static_cast<std::size_t>(i)] + std::sqrt(lambda_v), 2.0) + chi);
      out[static_cast<std::size_t>(i)] = std::max(draw, p.rate_floor);
    }
  } else {
    for (std::int64_t i = 0; i < paths; ++i) {
      const double lambda_v = std::max(0.0, (r_vec[static_cast<std::size_t>(i)] * exp_kdt) / c);
      std::poisson_distribution<std::int64_t> pois(0.5 * lambda_v);
      const std::int64_t n = pois(rng);
      std::gamma_distribution<double> chisq(0.5 * degree + static_cast<double>(n), 2.0);
      const double draw = c * chisq(rng);
      out[static_cast<std::size_t>(i)] = std::max(draw, p.rate_floor);
    }
  }

  return pluss::make_series(out, {1, paths}, cpu_cfg).to(cfg.platform, cfg.dtype);
}

// Heath-Jarrow-Morton in the Musiela parametrisation: the state is the forward curve f(t, tau) on a grid of times to maturity, and under the
// pricing measure it moves by
//     df(t, tau) = [ d f / d tau + sigma(tau) * int_0^tau sigma(u) du ] dt + sigma(tau) dW.
// The first term is the transport of the curve along the tenor axis as calendar time passes, the second the no-arbitrage drift; without
// both, bond prices are not martingales after discounting. The grid carries an extra node at tau = 0 (the short rate r_t = f(t, 0)),
// whose volatility is that of the first tenor; sigma is taken piecewise constant on each tenor interval, equal to the value at its right end.
struct HjmGrid_ {
  std::vector<double> tau;     // K + 1 nodes, tau[0] = 0
  std::vector<double> sigma;   // volatility at each node
  std::vector<double> alpha;   // no-arbitrage drift sigma(tau) * int_0^tau sigma
  std::vector<double> spacing; // tau[j + 1] - tau[j], K entries
};

HjmGrid_ hjm_grid_(const std::vector<double>& tenors, const std::vector<double>& vol) {
  HjmGrid_ g;
  g.tau.push_back(0.0);
  g.sigma.push_back(vol.front());
  for (std::size_t j = 0; j < tenors.size(); ++j) {
    g.tau.push_back(tenors[j]);
    g.sigma.push_back(vol[j]);
  }
  g.alpha.assign(g.tau.size(), 0.0);
  double cumulative = 0.0;
  for (std::size_t j = 1; j < g.tau.size(); ++j) {
    cumulative += g.sigma[j] * (g.tau[j] - g.tau[j - 1]);
    g.alpha[j] = g.sigma[j] * cumulative;
    g.spacing.push_back(g.tau[j] - g.tau[j - 1]);
  }
  return g;
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

Series stack_rows_(const std::vector<Series>& rows, const char* op) {
  PLUSS_CHECK(!rows.empty(), std::string("subsp::math::finance::rates::") + op + ": row history is empty");
  return pluss::series::pile(pluss::sign_list(rows), 0);
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
  const Series z = draw_normals_(steps, paths, options, out_cfg, seed, 0x1619ULL);

  const double exp_kdt = std::exp(-params.kappa * dt);
  const double mean_scale = 1.0 - exp_kdt;
  const double var_scale = (params.sigma * params.sigma) * (1.0 - std::exp(-2.0 * params.kappa * dt)) /
                           std::max(2.0 * params.kappa, kEps);

  Series rt = pluss::series::full({1, paths}, params.r0, out_cfg);
  std::vector<Series> rate_rows;
  rate_rows.reserve(static_cast<std::size_t>(n));
  rate_rows.push_back(rt.reshape({paths}));

  for (std::int64_t t = 0; t < steps; ++t) {
    const Series eps = z.select(0, t).reshape({1, paths});

    Series r_next = pluss::series::undefined();
    if (options.scheme == Scheme::Transition) {
      r_next = pluss::series::full({1, paths}, params.theta, out_cfg)
                   .add(rt.sub(params.theta).mul(exp_kdt))
                   .add(eps.mul(std::sqrt(std::max(var_scale, 0.0))));
    } else {
      r_next = rt.add(rt.mul(-params.kappa * dt))
                   .add(params.kappa * params.theta * dt)
                   .add(eps.mul(params.sigma * std::sqrt(dt)));
    }

    rt = r_next;
    rate_rows.push_back(rt.reshape({paths}));
  }

  const Series rate = stack_rows_(rate_rows, "vasicek_paths");
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
  const Series z = draw_normals_(steps, paths, options, out_cfg, seed, 0x2713ULL);
  const std::uint64_t transition_seed = effective_seed_(options, seed, 0x3521ULL);

  Series rt = pluss::series::full({1, paths}, std::max(params.r0, params.rate_floor), out_cfg);
  std::vector<Series> rate_rows;
  rate_rows.reserve(static_cast<std::size_t>(n));
  rate_rows.push_back(rt.reshape({paths}));

  for (std::int64_t t = 0; t < steps; ++t) {
    const Series eps = z.select(0, t).reshape({1, paths});

    Series r_next = pluss::series::undefined();
    if (options.scheme == Scheme::Transition) {
      r_next = cir_transition_step_(rt, eps, dt, params, transition_seed, t);
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

    rt = r_next;
    rate_rows.push_back(rt.reshape({paths}));
  }

  const Series rate = stack_rows_(rate_rows, "cir_paths");
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
  const Series z = draw_normals_(steps, paths, options, out_cfg, seed, 0x4131ULL);

  const double exp_kdt = std::exp(-params.kappa * dt);
  const double var_scale = (params.sigma * params.sigma) * (1.0 - std::exp(-2.0 * params.kappa * dt)) /
                           std::max(2.0 * params.kappa, kEps);

  Series rt = pluss::series::full({1, paths}, params.r0, out_cfg);
  std::vector<Series> rate_rows;
  rate_rows.reserve(static_cast<std::size_t>(n));
  rate_rows.push_back(rt.reshape({paths}));

  for (std::int64_t t = 0; t < steps; ++t) {
    const Series eps = z.select(0, t).reshape({1, paths});
    const double time_t = static_cast<double>(t) * dt;
    const double theta_t = params.theta0 + params.theta_slope * time_t;

    Series r_next = pluss::series::undefined();
    if (options.scheme == Scheme::Transition) {
      // Exact for a level that is linear in time: r(t+dt) = r e^{-k dt} + int_0^dt k e^{-k (dt - u)} (theta_t + slope u) du + noise, and the
      // integral is theta_t (1 - e^{-k dt}) + slope (dt - (1 - e^{-k dt}) / k).
      const double level_gain = theta_t * (1.0 - exp_kdt) + params.theta_slope * (dt - (1.0 - exp_kdt) / params.kappa);
      r_next = rt.mul(exp_kdt).add(level_gain).add(eps.mul(std::sqrt(std::max(var_scale, 0.0))));
    } else {
      r_next = rt.add(rt.mul(-params.kappa * dt))
                   .add(params.kappa * theta_t * dt)
                   .add(eps.mul(params.sigma * std::sqrt(dt)));
    }
    rt = r_next;
    rate_rows.push_back(rt.reshape({paths}));
  }

  const Series rate = stack_rows_(rate_rows, "hull_white_paths");
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
  const HjmGrid_ grid = hjm_grid_(params.tenors, params.vol);
  const double smallest_spacing = *std::min_element(grid.spacing.begin(), grid.spacing.end());
  PLUSS_CHECK(dt <= smallest_spacing,
              "subsp::math::finance::rates::hjm_paths: dt must not exceed the smallest spacing of the tenor grid (" +
                  std::to_string(smallest_spacing) + "), or the transport of the forward curve along the tenor axis is unstable");

  const Series z = draw_normals_(steps, paths, options, out_cfg, seed, 0x5719ULL);

  Series f_state = pluss::series::full({paths, k + 1}, params.r0, out_cfg);
  const Series alpha_row = pluss::make_series(grid.alpha, {1, k + 1}, out_cfg).mul(dt);
  const Series vol_row = pluss::make_series(grid.sigma, {1, k + 1}, out_cfg).mul(std::sqrt(dt));
  const Series spacing_row = pluss::make_series(grid.spacing, {1, k}, out_cfg);

  std::vector<Series> forward_rows;
  std::vector<Series> rate_rows;
  forward_rows.reserve(static_cast<std::size_t>(n));
  rate_rows.reserve(static_cast<std::size_t>(n));
  forward_rows.push_back(f_state.slice(1, 1, k + 1, 1));
  rate_rows.push_back(f_state.select(1, 0).reshape({paths}));

  for (std::int64_t t = 0; t < steps; ++t) {
    const Series eps = z.select(0, t).reshape({paths, 1});
    // transport: the slope of the curve along the tenor axis (forward difference; the last node reuses the slope of the last interval)
    const Series slope = f_state.slice(1, 1, k + 1, 1).sub(f_state.slice(1, 0, k, 1)).div(spacing_row);
    const Series slope_full = slope.unite({slope.select(1, k - 1).reshape({paths, 1})}, 1);
    f_state = f_state.add(slope_full.mul(dt)).add(alpha_row).add(eps.mul(vol_row));

    forward_rows.push_back(f_state.slice(1, 1, k + 1, 1));
    rate_rows.push_back(f_state.select(1, 0).reshape({paths}));
  }

  const Series forward = stack_rows_(forward_rows, "hjm_paths");
  const Series rate = stack_rows_(rate_rows, "hjm_paths");
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

  // dr = kappa (theta0 + theta_slope t - r) dt + sigma dW is Gaussian: ln P = -E[int r] + Var[int r] / 2, with
  // E[int_0^T r] = r B + theta0 (T - B) + theta_slope (T^2 / 2 - (T - B) / kappa) and B = (1 - exp(-kappa T)) / kappa.
  const double k = params.kappa;
  const double s2 = params.sigma * params.sigma;
  const double b = (1.0 - std::exp(-k * maturity)) / k;
  const double mean_integral = short_rate * b + params.theta0 * (maturity - b) +
                               params.theta_slope * (0.5 * maturity * maturity - (maturity - b) / k);
  const double half_variance = (s2 / (2.0 * k * k)) * (maturity - b) - (s2 * b * b) / (4.0 * k);
  return std::exp(-mean_integral + half_variance);
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
