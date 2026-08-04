#include "pluss/subsp/math/finance/module.h"

#include "CodeM/core/Errors.h"
#include "pluss/codem/losses.h"
#include "pluss/codem/series_factory.h"

#include <cmath>
#include <string>
#include <type_traits>

namespace pluss::subsp::math::finance::module {
namespace {

using pluss::Config;
using pluss::ScalarType;
using pluss::Series;
using volatility::BatesParams;
using volatility::GbmParams;
using volatility::HestonParams;
using volatility::RoughBergomiParams;
using volatility::RoughHestonParams;
using volatility::SabrParams;
using volatility::SimOptions;

pricer::RiskSpec to_risk_(Risk risk) {
  return (risk == Risk::All) ? pricer::RiskSpec::All : pricer::RiskSpec::PriceOnly;
}

Config f64_cfg_(Config cfg, Platform platform) {
  return Config{}.with_dtype(ScalarType::Float64).with_platform(platform).with_pinned(cfg.pinned);
}

Series as_series_vec_(const Series& x, const char* op, const char* name) {
  PLUSS_CHECK(x.defined(), std::string("subsp::math::finance::module::") + op + ": " + name + " must be defined");
  const auto sz = x.sizes();
  if (sz.empty()) return x.reshape({1});
  if (sz.size() == 1) return x;
  PLUSS_CHECK(sz.size() == 2 && (sz[0] == 1 || sz[1] == 1),
              std::string("subsp::math::finance::module::") + op + ": " + name + " must be vector-like");
  return x.reshape({x.numel()});
}

double scalar_from_series_(const Series& x, const char* op, const char* name, Config cfg) {
  const Series v = as_series_vec_(x, op, name).to(cfg.platform, cfg.dtype);
  PLUSS_CHECK(v.numel() == 1,
              std::string("subsp::math::finance::module::") + op + ": " + name + " must have exactly one element");
  return v.reshape({1}).item<double>();
}

double first_from_series_(const Series& x, const char* op, const char* name, Config cfg) {
  const Series v = as_series_vec_(x, op, name).to(cfg.platform, cfg.dtype).reshape({x.numel()});
  PLUSS_CHECK(v.numel() > 0, std::string("subsp::math::finance::module::") + op + ": " + name + " must be non-empty");
  return v.select(0, 0).reshape({1}).item<double>();
}

double scalar_or_default_(const Series& x, double fallback, const char* op, const char* name, Config cfg) {
  if (!x.defined()) return fallback;
  return scalar_from_series_(x, op, name, cfg);
}

HestonParams heston_params_from_model_(const model::Heston& mdl, Config cfg) {
  return HestonParams{
      .kappa = scalar_from_series_(mdl.kappa, "simulate_paths", "model.heston.kappa", cfg),
      .theta = scalar_from_series_(mdl.theta, "simulate_paths", "model.heston.theta", cfg),
      .sigma = scalar_from_series_(mdl.sigma, "simulate_paths", "model.heston.sigma", cfg),
      .rho = scalar_from_series_(mdl.rho, "simulate_paths", "model.heston.rho", cfg),
      .v0 = scalar_from_series_(mdl.v0, "simulate_paths", "model.heston.v0", cfg),
      .mu = scalar_or_default_(mdl.mu, 0.0, "simulate_paths", "model.heston.mu", cfg),
      .variance_floor = scalar_or_default_(mdl.variance_floor, 1.0e-12, "simulate_paths", "model.heston.variance_floor", cfg),
  };
}

BatesParams bates_params_from_model_(const model::Bates& mdl, Config cfg) {
  return BatesParams{
      .kappa = scalar_from_series_(mdl.kappa, "simulate_paths", "model.bates.kappa", cfg),
      .theta = scalar_from_series_(mdl.theta, "simulate_paths", "model.bates.theta", cfg),
      .sigma = scalar_from_series_(mdl.sigma, "simulate_paths", "model.bates.sigma", cfg),
      .rho = scalar_from_series_(mdl.rho, "simulate_paths", "model.bates.rho", cfg),
      .v0 = scalar_from_series_(mdl.v0, "simulate_paths", "model.bates.v0", cfg),
      .mu = scalar_or_default_(mdl.mu, 0.0, "simulate_paths", "model.bates.mu", cfg),
      .variance_floor = scalar_or_default_(mdl.variance_floor, 1.0e-12, "simulate_paths", "model.bates.variance_floor", cfg),
      .jump_intensity = scalar_or_default_(mdl.jump_intensity, 0.15, "simulate_paths", "model.bates.jump_intensity", cfg),
      .jump_mean = scalar_or_default_(mdl.jump_mean, -0.08, "simulate_paths", "model.bates.jump_mean", cfg),
      .jump_vol = scalar_or_default_(mdl.jump_vol, 0.20, "simulate_paths", "model.bates.jump_vol", cfg),
  };
}

SabrParams sabr_params_from_model_(const model::Sabr& mdl, Config cfg) {
  return SabrParams{
      .alpha0 = scalar_from_series_(mdl.alpha0, "simulate_paths", "model.sabr.alpha0", cfg),
      .beta = scalar_from_series_(mdl.beta, "simulate_paths", "model.sabr.beta", cfg),
      .nu = scalar_from_series_(mdl.nu, "simulate_paths", "model.sabr.nu", cfg),
      .rho = scalar_from_series_(mdl.rho, "simulate_paths", "model.sabr.rho", cfg),
      .mu = scalar_or_default_(mdl.mu, 0.0, "simulate_paths", "model.sabr.mu", cfg),
      .alpha_floor = scalar_or_default_(mdl.alpha_floor, 1.0e-12, "simulate_paths", "model.sabr.alpha_floor", cfg),
  };
}

RoughHestonParams rough_heston_params_from_model_(const model::RoughHeston& mdl, Config cfg) {
  return RoughHestonParams{
      .mu = scalar_or_default_(mdl.mu, 0.0, "simulate_paths", "model.rough_heston.mu", cfg),
      .v0 = scalar_from_series_(mdl.v0, "simulate_paths", "model.rough_heston.v0", cfg),
      .theta = scalar_from_series_(mdl.theta, "simulate_paths", "model.rough_heston.theta", cfg),
      .kappa = scalar_from_series_(mdl.kappa, "simulate_paths", "model.rough_heston.kappa", cfg),
      .sigma = scalar_from_series_(mdl.sigma, "simulate_paths", "model.rough_heston.sigma", cfg),
      .alpha = scalar_from_series_(mdl.alpha, "simulate_paths", "model.rough_heston.alpha", cfg),
      .rho = scalar_from_series_(mdl.rho, "simulate_paths", "model.rough_heston.rho", cfg),
      .variance_floor =
          scalar_or_default_(mdl.variance_floor, 1.0e-12, "simulate_paths", "model.rough_heston.variance_floor", cfg),
  };
}

RoughBergomiParams rough_bergomi_params_from_model_(const model::RoughBergomi& mdl, Config cfg) {
  return RoughBergomiParams{
      .mu = scalar_or_default_(mdl.mu, 0.0, "simulate_paths", "model.rough_bergomi.mu", cfg),
      .xi = scalar_from_series_(mdl.xi, "simulate_paths", "model.rough_bergomi.xi", cfg),
      .eta = scalar_from_series_(mdl.eta, "simulate_paths", "model.rough_bergomi.eta", cfg),
      .alpha = scalar_from_series_(mdl.alpha, "simulate_paths", "model.rough_bergomi.alpha", cfg),
      .rho = scalar_from_series_(mdl.rho, "simulate_paths", "model.rough_bergomi.rho", cfg),
      .variance_floor =
          scalar_or_default_(mdl.variance_floor, 1.0e-12, "simulate_paths", "model.rough_bergomi.variance_floor", cfg),
  };
}

double max_maturity_(const instrument::Vanilla& inst, Config cfg) {
  const Series tau = as_series_vec_(inst.maturity, "simulate_paths", "instrument.maturity").to(cfg.platform, cfg.dtype);
  PLUSS_CHECK(tau.numel() > 0, "subsp::math::finance::module::simulate_paths: instrument.maturity must be non-empty");
  const double tmax = tau.max().reshape({1}).item<double>();
  PLUSS_CHECK(std::isfinite(tmax) && tmax > 0.0,
              "subsp::math::finance::module::simulate_paths: max maturity must be finite and > 0");
  return tmax;
}

SimOptions to_sim_options_(const Controls& controls) {
  SimOptions sim;
  sim.scheme = controls.mc.scheme;
  sim.method = controls.mc.rand_method;
  sim.randomized_quasi = controls.mc.randomized_quasi;
  sim.sobol_scrambling = controls.mc.sobol_scrambling;
  sim.antithetic = controls.mc.antithetic;
  sim.normal_shift = controls.mc.importance_sampling ? controls.mc.importance_shift : 0.0;
  sim.paths_first = true;
  sim.seed = controls.mc.seed;
  sim.offset = controls.mc.offset;
  return sim;
}

Paths to_paths_(const volatility::StochVolPaths& raw) {
  return Paths{
      .asset = raw.spot,
      .vol = raw.variance.clamp_min(0.0).sqrt(),
      .variance = raw.variance,
      .asset_shocks = raw.innovation_spot,
      .vol_shocks = raw.innovation_var,
  };
}

pluss::codem::Series evaluate_loss_(Loss loss,
                                    const pluss::codem::Series& prediction,
                                    const pluss::codem::Series& target,
                                    double huber_delta) {
  switch (loss) {
    case Loss::MSE:
      return pluss::mse(prediction, target);
    case Loss::RelativeMSE:
      return pluss::rel_mse(prediction, target);
    case Loss::MAE:
      return pluss::mae(prediction, target);
    case Loss::Huber:
      PLUSS_CHECK(std::isfinite(huber_delta) && huber_delta > 0.0,
                  "subsp::math::finance::module::Field: huber_delta must be finite and > 0");
      return pluss::huber_loss(prediction, target, huber_delta);
  }
  return pluss::mse(prediction, target);
}

template <typename ModelT>
const char* model_name_() {
  if constexpr (std::is_same_v<ModelT, model::Heston>) return "Heston";
  if constexpr (std::is_same_v<ModelT, model::Bates>) return "Bates";
  if constexpr (std::is_same_v<ModelT, model::BlackScholes>) return "BlackScholes";
  if constexpr (std::is_same_v<ModelT, model::Sabr>) return "Sabr";
  if constexpr (std::is_same_v<ModelT, model::RoughHeston>) return "RoughHeston";
  if constexpr (std::is_same_v<ModelT, model::RoughBergomi>) return "RoughBergomi";
  return "UnknownModel";
}

const char* engine_name_(Engine engine) {
  switch (engine) {
    case Engine::Fft:
      return "Fft";
    case Engine::Cos:
      return "Cos";
    case Engine::MonteCarlo:
      return "MonteCarlo";
    case Engine::Analytic:
      return "Analytic";
  }
  return "UnknownEngine";
}

template <typename ModelT>
const char* supported_engines_() {
  if constexpr (std::is_same_v<ModelT, model::Heston> || std::is_same_v<ModelT, model::Bates>) {
    return "Fft, Cos, MonteCarlo";
  }
  if constexpr (std::is_same_v<ModelT, model::BlackScholes> || std::is_same_v<ModelT, model::Sabr>) {
    return "Analytic, MonteCarlo";
  }
  if constexpr (std::is_same_v<ModelT, model::RoughHeston> || std::is_same_v<ModelT, model::RoughBergomi>) {
    return "MonteCarlo";
  }
  return "None";
}

template <typename ModelT>
bool supports_engine_(Engine engine) {
  if constexpr (std::is_same_v<ModelT, model::Heston> || std::is_same_v<ModelT, model::Bates>) {
    return engine == Engine::Fft || engine == Engine::Cos || engine == Engine::MonteCarlo;
  }
  if constexpr (std::is_same_v<ModelT, model::BlackScholes> || std::is_same_v<ModelT, model::Sabr>) {
    return engine == Engine::Analytic || engine == Engine::MonteCarlo;
  }
  if constexpr (std::is_same_v<ModelT, model::RoughHeston> || std::is_same_v<ModelT, model::RoughBergomi>) {
    return engine == Engine::MonteCarlo;
  }
  return false;
}

template <typename ModelT>
void validate_engine_(Engine engine) {
  PLUSS_CHECK(supports_engine_<ModelT>(engine),
              std::string("subsp::math::finance::module::Field: controls.engine=") + engine_name_(engine) +
                  " is not supported for model " + model_name_<ModelT>() + ". Supported engines: " +
                  supported_engines_<ModelT>());
}

template <typename ModelT>
[[noreturn]] void unsupported_engine_(Engine engine) {
  PLUSS_CHECK(false,
              std::string("subsp::math::finance::module::Field: controls.engine=") + engine_name_(engine) +
                  " is not supported for model " + model_name_<ModelT>() + ". Supported engines: " +
                  supported_engines_<ModelT>());
}

pricer::Result price_one_(const instrument::Vanilla& inst,
                          const market::State& mkt,
                          const model::Heston& mdl,
                          const Controls& controls,
                          pluss::codem::Config cfg) {
  if (controls.engine == Engine::Fft) {
    return controls.fft.price(inst, mdl, mkt, to_risk_(controls.risk), cfg);
  }
  if (controls.engine == Engine::Cos) {
    return controls.cos.price(inst, mdl, mkt, to_risk_(controls.risk), cfg);
  }
  if (controls.engine == Engine::MonteCarlo) {
    return controls.mc.price(inst, mdl, mkt, to_risk_(controls.risk), cfg);
  }
  unsupported_engine_<model::Heston>(controls.engine);
}

pricer::Result price_one_(const instrument::Vanilla& inst,
                          const market::State& mkt,
                          const model::Bates& mdl,
                          const Controls& controls,
                          pluss::codem::Config cfg) {
  if (controls.engine == Engine::Fft) {
    return controls.fft.price(inst, mdl, mkt, to_risk_(controls.risk), cfg);
  }
  if (controls.engine == Engine::Cos) {
    return controls.cos.price(inst, mdl, mkt, to_risk_(controls.risk), cfg);
  }
  if (controls.engine == Engine::MonteCarlo) {
    return controls.mc.price(inst, mdl, mkt, to_risk_(controls.risk), cfg);
  }
  unsupported_engine_<model::Bates>(controls.engine);
}

pricer::Result price_one_(const instrument::Vanilla& inst,
                          const market::State& mkt,
                          const model::BlackScholes& mdl,
                          const Controls& controls,
                          pluss::codem::Config cfg) {
  if (controls.engine == Engine::Analytic) {
    return controls.analytic.price(inst, mdl, mkt, to_risk_(controls.risk), cfg);
  }
  if (controls.engine == Engine::MonteCarlo) {
    return controls.mc.price(inst, mdl, mkt, to_risk_(controls.risk), cfg);
  }
  unsupported_engine_<model::BlackScholes>(controls.engine);
}

pricer::Result price_one_(const instrument::Vanilla& inst,
                          const market::State& mkt,
                          const model::Sabr& mdl,
                          const Controls& controls,
                          pluss::codem::Config cfg) {
  if (controls.engine == Engine::Analytic) {
    return controls.analytic.price(inst, mdl, mkt, to_risk_(controls.risk), cfg);
  }
  if (controls.engine == Engine::MonteCarlo) {
    return controls.mc.price(inst, mdl, mkt, to_risk_(controls.risk), cfg);
  }
  unsupported_engine_<model::Sabr>(controls.engine);
}

pricer::Result price_one_(const instrument::Vanilla& inst,
                          const market::State& mkt,
                          const model::RoughHeston& mdl,
                          const Controls& controls,
                          pluss::codem::Config cfg) {
  if (controls.engine == Engine::MonteCarlo) {
    return controls.mc.price(inst, mdl, mkt, to_risk_(controls.risk), cfg);
  }
  unsupported_engine_<model::RoughHeston>(controls.engine);
}

pricer::Result price_one_(const instrument::Vanilla& inst,
                          const market::State& mkt,
                          const model::RoughBergomi& mdl,
                          const Controls& controls,
                          pluss::codem::Config cfg) {
  if (controls.engine == Engine::MonteCarlo) {
    return controls.mc.price(inst, mdl, mkt, to_risk_(controls.risk), cfg);
  }
  unsupported_engine_<model::RoughBergomi>(controls.engine);
}

template <typename ModelT>
Paths simulate_one_(const instrument::Vanilla&,
                    const market::State&,
                    const ModelT&,
                    const Controls&,
                    Config) {
  PLUSS_CHECK(false, "subsp::math::finance::module::simulate_paths: unsupported model type");
}

Paths simulate_one_(const instrument::Vanilla& inst,
                    const market::State& mkt,
                    const model::BlackScholes& mdl,
                    const Controls& controls,
                    Config cfg) {
  const std::int64_t steps = controls.mc.steps;
  const std::int64_t paths = controls.mc.paths;
  PLUSS_CHECK(steps > 0, "subsp::math::finance::module::simulate_paths: controls.mc.steps must be > 0");
  PLUSS_CHECK(paths > 0, "subsp::math::finance::module::simulate_paths: controls.mc.paths must be > 0");
  const double tmax = max_maturity_(inst, cfg);
  const double dt = tmax / static_cast<double>(steps);
  const double spot0 = first_from_series_(mkt.spot, "simulate_paths", "market.spot", cfg);
  GbmParams p{
      .mu = first_from_series_(mkt.rate, "simulate_paths", "market.rate", cfg) -
            first_from_series_(mkt.dividend_yield, "simulate_paths", "market.dividend_yield", cfg),
      .sigma = scalar_from_series_(mdl.volatility, "simulate_paths", "model.black_scholes.volatility", cfg),
  };
  return to_paths_(volatility::gbm_paths(steps, paths, spot0, dt, p, to_sim_options_(controls), cfg, controls.mc.seed));
}

Paths simulate_one_(const instrument::Vanilla& inst,
                    const market::State& mkt,
                    const model::Heston& mdl,
                    const Controls& controls,
                    Config cfg) {
  const std::int64_t steps = controls.mc.steps;
  const std::int64_t paths = controls.mc.paths;
  PLUSS_CHECK(steps > 0, "subsp::math::finance::module::simulate_paths: controls.mc.steps must be > 0");
  PLUSS_CHECK(paths > 0, "subsp::math::finance::module::simulate_paths: controls.mc.paths must be > 0");
  const double tmax = max_maturity_(inst, cfg);
  const double dt = tmax / static_cast<double>(steps);
  const double spot0 = first_from_series_(mkt.spot, "simulate_paths", "market.spot", cfg);
  auto p = heston_params_from_model_(mdl, cfg);
  p.mu = first_from_series_(mkt.rate, "simulate_paths", "market.rate", cfg) -
         first_from_series_(mkt.dividend_yield, "simulate_paths", "market.dividend_yield", cfg);
  return to_paths_(volatility::heston_paths(steps, paths, spot0, dt, p, to_sim_options_(controls), cfg, controls.mc.seed));
}

Paths simulate_one_(const instrument::Vanilla& inst,
                    const market::State& mkt,
                    const model::Bates& mdl,
                    const Controls& controls,
                    Config cfg) {
  const std::int64_t steps = controls.mc.steps;
  const std::int64_t paths = controls.mc.paths;
  PLUSS_CHECK(steps > 0, "subsp::math::finance::module::simulate_paths: controls.mc.steps must be > 0");
  PLUSS_CHECK(paths > 0, "subsp::math::finance::module::simulate_paths: controls.mc.paths must be > 0");
  const double tmax = max_maturity_(inst, cfg);
  const double dt = tmax / static_cast<double>(steps);
  const double spot0 = first_from_series_(mkt.spot, "simulate_paths", "market.spot", cfg);
  auto p = bates_params_from_model_(mdl, cfg);
  p.mu = first_from_series_(mkt.rate, "simulate_paths", "market.rate", cfg) -
         first_from_series_(mkt.dividend_yield, "simulate_paths", "market.dividend_yield", cfg);
  return to_paths_(volatility::bates_paths(steps, paths, spot0, dt, p, to_sim_options_(controls), cfg, controls.mc.seed));
}

Paths simulate_one_(const instrument::Vanilla& inst,
                    const market::State& mkt,
                    const model::Sabr& mdl,
                    const Controls& controls,
                    Config cfg) {
  const std::int64_t steps = controls.mc.steps;
  const std::int64_t paths = controls.mc.paths;
  PLUSS_CHECK(steps > 0, "subsp::math::finance::module::simulate_paths: controls.mc.steps must be > 0");
  PLUSS_CHECK(paths > 0, "subsp::math::finance::module::simulate_paths: controls.mc.paths must be > 0");
  const double tmax = max_maturity_(inst, cfg);
  const double dt = tmax / static_cast<double>(steps);
  const double forward0 = first_from_series_(mkt.spot, "simulate_paths", "market.spot", cfg);
  auto p = sabr_params_from_model_(mdl, cfg);
  p.mu = first_from_series_(mkt.rate, "simulate_paths", "market.rate", cfg) -
         first_from_series_(mkt.dividend_yield, "simulate_paths", "market.dividend_yield", cfg);
  return to_paths_(volatility::sabr_paths(steps, paths, forward0, dt, p, to_sim_options_(controls), cfg, controls.mc.seed));
}

Paths simulate_one_(const instrument::Vanilla& inst,
                    const market::State& mkt,
                    const model::RoughHeston& mdl,
                    const Controls& controls,
                    Config cfg) {
  const std::int64_t steps = controls.mc.steps;
  const std::int64_t paths = controls.mc.paths;
  PLUSS_CHECK(steps > 0, "subsp::math::finance::module::simulate_paths: controls.mc.steps must be > 0");
  PLUSS_CHECK(paths > 0, "subsp::math::finance::module::simulate_paths: controls.mc.paths must be > 0");
  const double tmax = max_maturity_(inst, cfg);
  const double dt = tmax / static_cast<double>(steps);
  const double spot0 = first_from_series_(mkt.spot, "simulate_paths", "market.spot", cfg);
  auto p = rough_heston_params_from_model_(mdl, cfg);
  p.mu = first_from_series_(mkt.rate, "simulate_paths", "market.rate", cfg) -
         first_from_series_(mkt.dividend_yield, "simulate_paths", "market.dividend_yield", cfg);
  return to_paths_(
      volatility::rough_heston_paths(steps, paths, spot0, dt, p, to_sim_options_(controls), cfg, controls.mc.seed));
}

Paths simulate_one_(const instrument::Vanilla& inst,
                    const market::State& mkt,
                    const model::RoughBergomi& mdl,
                    const Controls& controls,
                    Config cfg) {
  const std::int64_t steps = controls.mc.steps;
  const std::int64_t paths = controls.mc.paths;
  PLUSS_CHECK(steps > 0, "subsp::math::finance::module::simulate_paths: controls.mc.steps must be > 0");
  PLUSS_CHECK(paths > 0, "subsp::math::finance::module::simulate_paths: controls.mc.paths must be > 0");
  const double tmax = max_maturity_(inst, cfg);
  const double dt = tmax / static_cast<double>(steps);
  const double spot0 = first_from_series_(mkt.spot, "simulate_paths", "market.spot", cfg);
  auto p = rough_bergomi_params_from_model_(mdl, cfg);
  p.mu = first_from_series_(mkt.rate, "simulate_paths", "market.rate", cfg) -
         first_from_series_(mkt.dividend_yield, "simulate_paths", "market.dividend_yield", cfg);
  return to_paths_(
      volatility::rough_bergomi_paths(steps, paths, spot0, dt, p, to_sim_options_(controls), cfg, controls.mc.seed));
}

Config model_param_cfg_(std::initializer_list<const Series*> refs, Config cfg) {
  for (const Series* ref : refs) {
    if (ref != nullptr && ref->defined()) {
      return Config{}.with_dtype(ref->scalar_type()).with_platform(ref->platform()).with_pinned(cfg.pinned);
    }
  }
  return f64_cfg_(cfg, cfg.platform);
}

Series make_param_series_(std::span<const double> params, Config cfg) {
  const std::array<std::int64_t, 1> shape{static_cast<std::int64_t>(params.size())};
  return pluss::codem::make_series<double>(params, std::span<const std::int64_t>(shape.data(), shape.size()), cfg);
}

Series scalar_series_from_(const Series& x, const char* name, Config cfg) {
  const Series v = as_series_vec_(x, "model.parameters", name).to(cfg.platform, cfg.dtype);
  PLUSS_CHECK(v.numel() == 1,
              std::string("subsp::math::finance::module::Model::parameters: ") + name + " must have exactly one element");
  return v.reshape({1});
}

Series heston_parameters_(const model::Heston& mdl, Config cfg) {
  const Config out_cfg = model_param_cfg_({&mdl.kappa, &mdl.theta, &mdl.sigma, &mdl.rho, &mdl.v0}, cfg);
  const Series kappa = scalar_series_from_(mdl.kappa, "model.heston.kappa", out_cfg);
  const Series theta = scalar_series_from_(mdl.theta, "model.heston.theta", out_cfg);
  const Series sigma = scalar_series_from_(mdl.sigma, "model.heston.sigma", out_cfg);
  const Series rho = scalar_series_from_(mdl.rho, "model.heston.rho", out_cfg);
  const Series v0 = scalar_series_from_(mdl.v0, "model.heston.v0", out_cfg);
  return kappa.unite({theta, sigma, rho, v0}, 0).reshape({5});
}

Series bates_parameters_(const model::Bates& mdl, Config cfg) {
  const Config out_cfg = model_param_cfg_({&mdl.kappa,
                                           &mdl.theta,
                                           &mdl.sigma,
                                           &mdl.rho,
                                           &mdl.v0,
                                           &mdl.jump_intensity,
                                           &mdl.jump_mean,
                                           &mdl.jump_vol},
                                          cfg);
  const Series kappa = scalar_series_from_(mdl.kappa, "model.bates.kappa", out_cfg);
  const Series theta = scalar_series_from_(mdl.theta, "model.bates.theta", out_cfg);
  const Series sigma = scalar_series_from_(mdl.sigma, "model.bates.sigma", out_cfg);
  const Series rho = scalar_series_from_(mdl.rho, "model.bates.rho", out_cfg);
  const Series v0 = scalar_series_from_(mdl.v0, "model.bates.v0", out_cfg);
  const Series lambda = scalar_series_from_(mdl.jump_intensity, "model.bates.jump_intensity", out_cfg);
  const Series mu_j = scalar_series_from_(mdl.jump_mean, "model.bates.jump_mean", out_cfg);
  const Series sigma_j = scalar_series_from_(mdl.jump_vol, "model.bates.jump_vol", out_cfg);
  return kappa.unite({theta, sigma, rho, v0, lambda, mu_j, sigma_j}, 0).reshape({8});
}

Series black_scholes_parameters_(const model::BlackScholes& mdl, Config cfg) {
  const Config out_cfg = model_param_cfg_({&mdl.volatility}, cfg);
  return scalar_series_from_(mdl.volatility, "model.black_scholes.volatility", out_cfg);
}

Series sabr_parameters_(const model::Sabr& mdl, Config cfg) {
  const Config out_cfg = model_param_cfg_({&mdl.alpha0, &mdl.beta, &mdl.nu, &mdl.rho}, cfg);
  const Series alpha0 = scalar_series_from_(mdl.alpha0, "model.sabr.alpha0", out_cfg);
  const Series beta = scalar_series_from_(mdl.beta, "model.sabr.beta", out_cfg);
  const Series nu = scalar_series_from_(mdl.nu, "model.sabr.nu", out_cfg);
  const Series rho = scalar_series_from_(mdl.rho, "model.sabr.rho", out_cfg);
  return alpha0.unite({beta, nu, rho}, 0).reshape({4});
}

Series rough_heston_parameters_(const model::RoughHeston& mdl, Config cfg) {
  const Config out_cfg = model_param_cfg_({&mdl.kappa, &mdl.theta, &mdl.sigma, &mdl.alpha, &mdl.rho, &mdl.v0}, cfg);
  const Series kappa = scalar_series_from_(mdl.kappa, "model.rough_heston.kappa", out_cfg);
  const Series theta = scalar_series_from_(mdl.theta, "model.rough_heston.theta", out_cfg);
  const Series sigma = scalar_series_from_(mdl.sigma, "model.rough_heston.sigma", out_cfg);
  const Series alpha = scalar_series_from_(mdl.alpha, "model.rough_heston.alpha", out_cfg);
  const Series rho = scalar_series_from_(mdl.rho, "model.rough_heston.rho", out_cfg);
  const Series v0 = scalar_series_from_(mdl.v0, "model.rough_heston.v0", out_cfg);
  return kappa.unite({theta, sigma, alpha, rho, v0}, 0).reshape({6});
}

Series rough_bergomi_parameters_(const model::RoughBergomi& mdl, Config cfg) {
  const Config out_cfg = model_param_cfg_({&mdl.xi, &mdl.eta, &mdl.alpha, &mdl.rho}, cfg);
  const Series xi = scalar_series_from_(mdl.xi, "model.rough_bergomi.xi", out_cfg);
  const Series eta = scalar_series_from_(mdl.eta, "model.rough_bergomi.eta", out_cfg);
  const Series alpha = scalar_series_from_(mdl.alpha, "model.rough_bergomi.alpha", out_cfg);
  const Series rho = scalar_series_from_(mdl.rho, "model.rough_bergomi.rho", out_cfg);
  return xi.unite({eta, alpha, rho}, 0).reshape({4});
}

Series param_slot_(const Series& p, std::int64_t idx, Config cfg) {
  return p.select(0, idx).reshape({1}).to(cfg.platform, cfg.dtype);
}

void require_param_count_(const Series& p, std::int64_t expected, const char* model_name) {
  PLUSS_CHECK(p.numel() == expected,
              std::string("subsp::math::finance::module::Model::parameters: ") + model_name + " expects " +
                  std::to_string(expected) + " parameters");
}

void set_heston_parameters_series_(model::Heston& mdl, const Series& params, Config cfg) {
  const Series p = as_series_vec_(params, "model.parameters", "params").to(cfg.platform, cfg.dtype).reshape({params.numel()});
  require_param_count_(p, 5, "Heston");
  mdl.kappa = param_slot_(p, 0, cfg);
  mdl.theta = param_slot_(p, 1, cfg);
  mdl.sigma = param_slot_(p, 2, cfg);
  mdl.rho = param_slot_(p, 3, cfg);
  mdl.v0 = param_slot_(p, 4, cfg);
}

void set_bates_parameters_series_(model::Bates& mdl, const Series& params, Config cfg) {
  const Series p = as_series_vec_(params, "model.parameters", "params").to(cfg.platform, cfg.dtype).reshape({params.numel()});
  require_param_count_(p, 8, "Bates");
  mdl.kappa = param_slot_(p, 0, cfg);
  mdl.theta = param_slot_(p, 1, cfg);
  mdl.sigma = param_slot_(p, 2, cfg);
  mdl.rho = param_slot_(p, 3, cfg);
  mdl.v0 = param_slot_(p, 4, cfg);
  mdl.jump_intensity = param_slot_(p, 5, cfg);
  mdl.jump_mean = param_slot_(p, 6, cfg);
  mdl.jump_vol = param_slot_(p, 7, cfg);
}

void set_black_scholes_parameters_series_(model::BlackScholes& mdl, const Series& params, Config cfg) {
  const Series p = as_series_vec_(params, "model.parameters", "params").to(cfg.platform, cfg.dtype).reshape({params.numel()});
  require_param_count_(p, 1, "BlackScholes");
  mdl.volatility = param_slot_(p, 0, cfg);
}

void set_sabr_parameters_series_(model::Sabr& mdl, const Series& params, Config cfg) {
  const Series p = as_series_vec_(params, "model.parameters", "params").to(cfg.platform, cfg.dtype).reshape({params.numel()});
  require_param_count_(p, 4, "Sabr");
  mdl.alpha0 = param_slot_(p, 0, cfg);
  mdl.beta = param_slot_(p, 1, cfg);
  mdl.nu = param_slot_(p, 2, cfg);
  mdl.rho = param_slot_(p, 3, cfg);
}

void set_rough_heston_parameters_series_(model::RoughHeston& mdl, const Series& params, Config cfg) {
  const Series p = as_series_vec_(params, "model.parameters", "params").to(cfg.platform, cfg.dtype).reshape({params.numel()});
  require_param_count_(p, 6, "RoughHeston");
  mdl.kappa = param_slot_(p, 0, cfg);
  mdl.theta = param_slot_(p, 1, cfg);
  mdl.sigma = param_slot_(p, 2, cfg);
  mdl.alpha = param_slot_(p, 3, cfg);
  mdl.rho = param_slot_(p, 4, cfg);
  mdl.v0 = param_slot_(p, 5, cfg);
}

void set_rough_bergomi_parameters_series_(model::RoughBergomi& mdl, const Series& params, Config cfg) {
  const Series p = as_series_vec_(params, "model.parameters", "params").to(cfg.platform, cfg.dtype).reshape({params.numel()});
  require_param_count_(p, 4, "RoughBergomi");
  mdl.xi = param_slot_(p, 0, cfg);
  mdl.eta = param_slot_(p, 1, cfg);
  mdl.alpha = param_slot_(p, 2, cfg);
  mdl.rho = param_slot_(p, 3, cfg);
}

void set_heston_parameters_(model::Heston& mdl, std::span<const double> params, Config cfg) {
  PLUSS_CHECK(params.size() == 5, "subsp::math::finance::module::Model::parameters: Heston expects 5 parameters");
  const Config out_cfg = model_param_cfg_({&mdl.kappa, &mdl.theta, &mdl.sigma, &mdl.rho, &mdl.v0}, cfg);
  mdl.kappa = pluss::series::full({1}, params[0], out_cfg);
  mdl.theta = pluss::series::full({1}, params[1], out_cfg);
  mdl.sigma = pluss::series::full({1}, params[2], out_cfg);
  mdl.rho = pluss::series::full({1}, params[3], out_cfg);
  mdl.v0 = pluss::series::full({1}, params[4], out_cfg);
}

void set_bates_parameters_(model::Bates& mdl, std::span<const double> params, Config cfg) {
  PLUSS_CHECK(params.size() == 8, "subsp::math::finance::module::Model::parameters: Bates expects 8 parameters");
  const Config out_cfg = model_param_cfg_({&mdl.kappa,
                                           &mdl.theta,
                                           &mdl.sigma,
                                           &mdl.rho,
                                           &mdl.v0,
                                           &mdl.jump_intensity,
                                           &mdl.jump_mean,
                                           &mdl.jump_vol},
                                          cfg);
  mdl.kappa = pluss::series::full({1}, params[0], out_cfg);
  mdl.theta = pluss::series::full({1}, params[1], out_cfg);
  mdl.sigma = pluss::series::full({1}, params[2], out_cfg);
  mdl.rho = pluss::series::full({1}, params[3], out_cfg);
  mdl.v0 = pluss::series::full({1}, params[4], out_cfg);
  mdl.jump_intensity = pluss::series::full({1}, params[5], out_cfg);
  mdl.jump_mean = pluss::series::full({1}, params[6], out_cfg);
  mdl.jump_vol = pluss::series::full({1}, params[7], out_cfg);
}

void set_black_scholes_parameters_(model::BlackScholes& mdl, std::span<const double> params, Config cfg) {
  PLUSS_CHECK(params.size() == 1, "subsp::math::finance::module::Model::parameters: BlackScholes expects 1 parameter");
  const Config out_cfg = model_param_cfg_({&mdl.volatility}, cfg);
  mdl.volatility = pluss::series::full({1}, params[0], out_cfg);
}

void set_sabr_parameters_(model::Sabr& mdl, std::span<const double> params, Config cfg) {
  PLUSS_CHECK(params.size() == 4, "subsp::math::finance::module::Model::parameters: Sabr expects 4 parameters");
  const Config out_cfg = model_param_cfg_({&mdl.alpha0, &mdl.beta, &mdl.nu, &mdl.rho}, cfg);
  mdl.alpha0 = pluss::series::full({1}, params[0], out_cfg);
  mdl.beta = pluss::series::full({1}, params[1], out_cfg);
  mdl.nu = pluss::series::full({1}, params[2], out_cfg);
  mdl.rho = pluss::series::full({1}, params[3], out_cfg);
}

void set_rough_heston_parameters_(model::RoughHeston& mdl, std::span<const double> params, Config cfg) {
  PLUSS_CHECK(params.size() == 6, "subsp::math::finance::module::Model::parameters: RoughHeston expects 6 parameters");
  const Config out_cfg = model_param_cfg_({&mdl.kappa, &mdl.theta, &mdl.sigma, &mdl.alpha, &mdl.rho, &mdl.v0}, cfg);
  mdl.kappa = pluss::series::full({1}, params[0], out_cfg);
  mdl.theta = pluss::series::full({1}, params[1], out_cfg);
  mdl.sigma = pluss::series::full({1}, params[2], out_cfg);
  mdl.alpha = pluss::series::full({1}, params[3], out_cfg);
  mdl.rho = pluss::series::full({1}, params[4], out_cfg);
  mdl.v0 = pluss::series::full({1}, params[5], out_cfg);
}

void set_rough_bergomi_parameters_(model::RoughBergomi& mdl, std::span<const double> params, Config cfg) {
  PLUSS_CHECK(params.size() == 4, "subsp::math::finance::module::Model::parameters: RoughBergomi expects 4 parameters");
  const Config out_cfg = model_param_cfg_({&mdl.xi, &mdl.eta, &mdl.alpha, &mdl.rho}, cfg);
  mdl.xi = pluss::series::full({1}, params[0], out_cfg);
  mdl.eta = pluss::series::full({1}, params[1], out_cfg);
  mdl.alpha = pluss::series::full({1}, params[2], out_cfg);
  mdl.rho = pluss::series::full({1}, params[3], out_cfg);
}

} // namespace

pluss::codem::Series Model::parameters(pluss::codem::Config cfg) const {
  return visit([&](const auto& m) -> Series {
    using M = std::decay_t<decltype(m)>;
    if constexpr (std::is_same_v<M, model::Heston>) {
      return heston_parameters_(m, cfg);
    } else if constexpr (std::is_same_v<M, model::Bates>) {
      return bates_parameters_(m, cfg);
    } else if constexpr (std::is_same_v<M, model::BlackScholes>) {
      return black_scholes_parameters_(m, cfg);
    } else if constexpr (std::is_same_v<M, model::Sabr>) {
      return sabr_parameters_(m, cfg);
    } else if constexpr (std::is_same_v<M, model::RoughHeston>) {
      return rough_heston_parameters_(m, cfg);
    } else {
      return rough_bergomi_parameters_(m, cfg);
    }
  });
}

void Model::parameters(const pluss::codem::Series& params, pluss::codem::Config cfg) {
  const Series pvec = as_series_vec_(params, "model.parameters", "params");
  const Config out_cfg =
      Config{}.with_dtype(params.scalar_type()).with_platform(params.platform()).with_pinned(cfg.pinned);
  visit([&](auto& m) {
    using M = std::decay_t<decltype(m)>;
    if constexpr (std::is_same_v<M, model::Heston>) {
      set_heston_parameters_series_(m, pvec, out_cfg);
    } else if constexpr (std::is_same_v<M, model::Bates>) {
      set_bates_parameters_series_(m, pvec, out_cfg);
    } else if constexpr (std::is_same_v<M, model::BlackScholes>) {
      set_black_scholes_parameters_series_(m, pvec, out_cfg);
    } else if constexpr (std::is_same_v<M, model::Sabr>) {
      set_sabr_parameters_series_(m, pvec, out_cfg);
    } else if constexpr (std::is_same_v<M, model::RoughHeston>) {
      set_rough_heston_parameters_series_(m, pvec, out_cfg);
    } else {
      set_rough_bergomi_parameters_series_(m, pvec, out_cfg);
    }
  });
}

void Model::parameters(std::span<const double> params, pluss::codem::Config cfg) {
  parameters(make_param_series_(params, f64_cfg_(cfg, cfg.platform)), cfg);
}

Paths Field::simulate_paths(pluss::codem::Config cfg) const {
  return simulate_paths(model, cfg);
}

Paths Field::simulate_paths(const Model& model_candidate, pluss::codem::Config cfg) const {
  PLUSS_CHECK(controls.engine == Engine::MonteCarlo,
              "subsp::math::finance::module::Field::simulate_paths: controls.engine must be MonteCarlo");
  Field tmp = *this;
  tmp.model = model_candidate;
  tmp.validate();
  const Platform platform = tmp.market.spot.defined() ? tmp.market.spot.platform() : cfg.platform;
  const auto out_cfg = f64_cfg_(cfg, platform);
  return model_candidate.visit(
      [&](const auto& m) {
        return simulate_one_(tmp.instrument, tmp.market, m, tmp.controls, out_cfg);
      });
}

void Field::simulate(pluss::codem::Config cfg) {
  paths = simulate_paths(cfg);
}

void Field::simulate(const Model& model_candidate, pluss::codem::Config cfg) {
  paths = simulate_paths(model_candidate, cfg);
}

void Field::clear_paths() {
  paths = Paths{};
}

std::string Field::model_name() const {
  return model.visit(
      [](const auto& m) -> std::string {
        using M = std::decay_t<decltype(m)>;
        return std::string(model_name_<M>());
      });
}

std::string Field::engine_name() const {
  return std::string(engine_name_(controls.engine));
}

void Field::validate() const {
  PLUSS_CHECK(instrument.strike.defined(), "subsp::math::finance::module::Field: instrument.strike must be defined");
  PLUSS_CHECK(instrument.maturity.defined(),
              "subsp::math::finance::module::Field: instrument.maturity must be defined");
  PLUSS_CHECK(market.spot.defined(), "subsp::math::finance::module::Field: market.spot must be defined");
  PLUSS_CHECK(market.rate.defined(), "subsp::math::finance::module::Field: market.rate must be defined");
  PLUSS_CHECK(market.dividend_yield.defined(),
              "subsp::math::finance::module::Field: market.dividend_yield must be defined");
  if (controls.loss == Loss::Huber) {
    PLUSS_CHECK(std::isfinite(controls.huber_delta) && controls.huber_delta > 0.0,
                "subsp::math::finance::module::Field: controls.huber_delta must be finite and > 0");
  }

  model.visit(
      [&](const auto& m) {
        using M = std::decay_t<decltype(m)>;
        (void)m;
        validate_engine_<M>(controls.engine);
      });
}

pricer::Result Field::price(pluss::codem::Config cfg) const {
  return price(model, cfg);
}

pricer::Result Field::price(const Model& model_candidate, pluss::codem::Config cfg) const {
  Field tmp = *this;
  tmp.model = model_candidate;
  tmp.validate();
  return model_candidate.visit(
      [&](const auto& m) {
        return price_one_(tmp.instrument, tmp.market, m, tmp.controls, cfg);
      });
}

pluss::codem::Series Field::residuals(const pluss::codem::Series& target_prices, pluss::codem::Config cfg) const {
  const auto out = price(cfg);
  return out.price.sub(target_prices);
}

pluss::codem::Series Field::loss(const pluss::codem::Series& target_prices, pluss::codem::Config cfg) const {
  const auto out = price(cfg);
  return evaluate_loss_(controls.loss, out.price, target_prices, controls.huber_delta);
}

double Field::loss_value(const pluss::codem::Series& target_prices, pluss::codem::Config cfg) const {
  return loss(target_prices, cfg).item<double>();
}

double Field::objective(const pluss::codem::Series& target_prices, pluss::codem::Config cfg) const {
  return loss(target_prices, cfg).item<double>();
}

std::function<double(const Model&)> Field::bind_objective(const pluss::codem::Series& target_prices,
                                                             pluss::codem::Config cfg) const {
  Field base = *this;
  return [base, target_prices, cfg](const Model& candidate) mutable -> double {
    base.model = candidate;
    return base.loss_value(target_prices, cfg);
  };
}

} // namespace pluss::subsp::math::finance::module
