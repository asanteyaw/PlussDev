#pragma once

#include "pluss/codem/codem.h"
#include "pluss/config.h"
#include "pluss/export.h"
#include "pluss/subsp/math/finance/pricing.h"
#include "pluss/subsp/math/finance/volatility.h"

#include <cstdint>
#include <limits>

namespace pluss::subsp::math::finance::object {

struct Vanilla {
  pricing::OptionType type = pricing::OptionType::Call;
  pluss::codem::Series is_call = pluss::codem::Series::undefined(); // required when type == OptionType::Auto
  pluss::codem::Series strike = pluss::codem::Series::undefined();
  pluss::codem::Series maturity = pluss::codem::Series::undefined();
};

struct MarketState {
  pluss::codem::Series spot = pluss::codem::Series::undefined();
  pluss::codem::Series rate = pluss::codem::Series::undefined();
  pluss::codem::Series dividend_yield = pluss::codem::Series::undefined();
};

namespace model {

struct BlackScholes {
  pluss::codem::Series volatility = pluss::codem::Series::undefined();
};

struct Heston {
  pluss::codem::Series kappa = pluss::codem::Series::undefined();
  pluss::codem::Series theta = pluss::codem::Series::undefined();
  pluss::codem::Series sigma = pluss::codem::Series::undefined();
  pluss::codem::Series rho = pluss::codem::Series::undefined();
  pluss::codem::Series v0 = pluss::codem::Series::undefined();
  pluss::codem::Series mu = pluss::codem::Series::undefined();
  pluss::codem::Series variance_floor = pluss::codem::Series::undefined();
};

struct Bates {
  pluss::codem::Series kappa = pluss::codem::Series::undefined();
  pluss::codem::Series theta = pluss::codem::Series::undefined();
  pluss::codem::Series sigma = pluss::codem::Series::undefined();
  pluss::codem::Series rho = pluss::codem::Series::undefined();
  pluss::codem::Series v0 = pluss::codem::Series::undefined();
  pluss::codem::Series mu = pluss::codem::Series::undefined();
  pluss::codem::Series variance_floor = pluss::codem::Series::undefined();
  pluss::codem::Series jump_intensity = pluss::codem::Series::undefined();
  pluss::codem::Series jump_mean = pluss::codem::Series::undefined();
  pluss::codem::Series jump_vol = pluss::codem::Series::undefined();
};

struct Sabr {
  pluss::codem::Series alpha0 = pluss::codem::Series::undefined();
  pluss::codem::Series beta = pluss::codem::Series::undefined();
  pluss::codem::Series nu = pluss::codem::Series::undefined();
  pluss::codem::Series rho = pluss::codem::Series::undefined();
  pluss::codem::Series mu = pluss::codem::Series::undefined();
  pluss::codem::Series alpha_floor = pluss::codem::Series::undefined();
  pricing::SabrVolType vol_type = pricing::SabrVolType::Lognormal;
  pricing::SabrApproxOptions approx{};
};

struct RoughHeston {
  pluss::codem::Series kappa = pluss::codem::Series::undefined();
  pluss::codem::Series theta = pluss::codem::Series::undefined();
  pluss::codem::Series sigma = pluss::codem::Series::undefined();
  pluss::codem::Series alpha = pluss::codem::Series::undefined();
  pluss::codem::Series rho = pluss::codem::Series::undefined();
  pluss::codem::Series v0 = pluss::codem::Series::undefined();
  pluss::codem::Series mu = pluss::codem::Series::undefined();
  pluss::codem::Series variance_floor = pluss::codem::Series::undefined();
};

struct RoughBergomi {
  pluss::codem::Series xi = pluss::codem::Series::undefined();
  pluss::codem::Series eta = pluss::codem::Series::undefined();
  pluss::codem::Series alpha = pluss::codem::Series::undefined();
  pluss::codem::Series rho = pluss::codem::Series::undefined();
  pluss::codem::Series mu = pluss::codem::Series::undefined();
  pluss::codem::Series variance_floor = pluss::codem::Series::undefined();
};

} // namespace model

namespace pricer {

struct Analytic {
  bool with_greeks = true;
};

struct MonteCarlo {
  pricing::MonteCarloOptions options{};
};

struct Fft {
  pricing::HestonFftOptions options{};
  bool return_grid = false;
};

struct Cos {
  pricing::CosOptions options{};
};

} // namespace pricer

struct PriceResult {
  pluss::codem::Series price = pluss::codem::Series::undefined();
  pluss::codem::Series delta = pluss::codem::Series::undefined();
  pluss::codem::Series gamma = pluss::codem::Series::undefined();
  pluss::codem::Series vega = pluss::codem::Series::undefined();
  pluss::codem::Series theta = pluss::codem::Series::undefined();
  pluss::codem::Series rho = pluss::codem::Series::undefined();
  pluss::codem::Series strike_grid = pluss::codem::Series::undefined();
};

CODEM_API PriceResult price(const Vanilla& inst,
                            const model::BlackScholes& mdl,
                            const MarketState& mkt,
                            const pricer::Analytic& eng = {},
                            pluss::codem::Config cfg = {});

CODEM_API PriceResult price(const Vanilla& inst,
                            const model::BlackScholes& mdl,
                            const MarketState& mkt,
                            const pricer::MonteCarlo& eng,
                            pluss::codem::Config cfg = {});

CODEM_API PriceResult price(const Vanilla& inst,
                            const model::Heston& mdl,
                            const MarketState& mkt,
                            const pricer::Fft& eng = {},
                            pluss::codem::Config cfg = {});

CODEM_API PriceResult price(const Vanilla& inst,
                            const model::Heston& mdl,
                            const MarketState& mkt,
                            const pricer::MonteCarlo& eng,
                            pluss::codem::Config cfg = {});

CODEM_API PriceResult price(const Vanilla& inst,
                            const model::Bates& mdl,
                            const MarketState& mkt,
                            const pricer::Fft& eng,
                            pluss::codem::Config cfg = {});

CODEM_API PriceResult price(const Vanilla& inst,
                            const model::Heston& mdl,
                            const MarketState& mkt,
                            const pricer::Cos& eng,
                            pluss::codem::Config cfg = {});

CODEM_API PriceResult price(const Vanilla& inst,
                            const model::Bates& mdl,
                            const MarketState& mkt,
                            const pricer::Cos& eng,
                            pluss::codem::Config cfg = {});

CODEM_API PriceResult price(const Vanilla& inst,
                            const model::Bates& mdl,
                            const MarketState& mkt,
                            const pricer::MonteCarlo& eng,
                            pluss::codem::Config cfg = {});

CODEM_API PriceResult price(const Vanilla& inst,
                            const model::Sabr& mdl,
                            const MarketState& mkt,
                            const pricer::Analytic& eng,
                            pluss::codem::Config cfg = {});

CODEM_API PriceResult price(const Vanilla& inst,
                            const model::Sabr& mdl,
                            const MarketState& mkt,
                            const pricer::MonteCarlo& eng,
                            pluss::codem::Config cfg = {});

CODEM_API PriceResult price(const Vanilla& inst,
                            const model::RoughHeston& mdl,
                            const MarketState& mkt,
                            const pricer::MonteCarlo& eng,
                            pluss::codem::Config cfg = {});

CODEM_API PriceResult price(const Vanilla& inst,
                            const model::RoughBergomi& mdl,
                            const MarketState& mkt,
                            const pricer::MonteCarlo& eng,
                            pluss::codem::Config cfg = {});

} // namespace pluss::subsp::math::finance::object

namespace pluss::subsp::math::finance {

namespace instrument {
using Vanilla = object::Vanilla;

CODEM_API Vanilla make(pricing::OptionType type,
                       double strike,
                       double maturity,
                       pluss::codem::Config cfg = {});
}

namespace market {
using State = object::MarketState;

CODEM_API State make(double spot, double rate, double dividend_yield, pluss::codem::Config cfg = {});
}

namespace model {
using BlackScholes = object::model::BlackScholes;
using Heston = object::model::Heston;
using Bates = object::model::Bates;
using Sabr = object::model::Sabr;
using RoughHeston = object::model::RoughHeston;
using RoughBergomi = object::model::RoughBergomi;

CODEM_API Heston make(double kappa,
                      double theta,
                      double sigma,
                      double rho,
                      double v0,
                      pluss::codem::Config cfg = {});

CODEM_API Bates make(double kappa,
                     double theta,
                     double sigma,
                     double rho,
                     double v0,
                     double jump_intensity,
                     double jump_mean,
                     double jump_vol,
                     pluss::codem::Config cfg = {});

CODEM_API Sabr make(double alpha0,
                    double beta,
                    double nu,
                    double rho,
                    pricing::SabrVolType vol_type = pricing::SabrVolType::Lognormal,
                    pricing::SabrApproxOptions approx = {},
                    pluss::codem::Config cfg = {});
}

namespace pricer {

enum class RiskSpec : std::uint8_t {
  PriceOnly = 0,
  All = 1,
};

struct Greeks {
  pluss::codem::Series delta = pluss::codem::Series::undefined();
  pluss::codem::Series gamma = pluss::codem::Series::undefined();
  pluss::codem::Series vega = pluss::codem::Series::undefined();
  pluss::codem::Series theta = pluss::codem::Series::undefined();
  pluss::codem::Series rho = pluss::codem::Series::undefined();
};

struct Result {
  pluss::codem::Series price = pluss::codem::Series::undefined();
  Greeks greeks{};
  pluss::codem::Series strike_grid = pluss::codem::Series::undefined();
};

struct Analytic {
  bool with_greeks = true;

  CODEM_API Result price(const instrument::Vanilla& inst,
                         const model::BlackScholes& mdl,
                         const market::State& mkt,
                         RiskSpec risk = RiskSpec::All,
                         pluss::codem::Config cfg = {}) const;

  CODEM_API Result price(const instrument::Vanilla& inst,
                         const model::Sabr& mdl,
                         const market::State& mkt,
                         RiskSpec risk = RiskSpec::PriceOnly,
                         pluss::codem::Config cfg = {}) const;
};

struct MonteCarlo {
  std::int64_t paths = 40000;
  std::int64_t steps = 252;
  volatility::SimScheme scheme = volatility::SimScheme::Euler;
  volatility::RandMethod rand_method = volatility::RandMethod::Pseudo;
  bool randomized_quasi = false;
  int sobol_scrambling = 0;
  bool antithetic = false;
  bool control_variate = false;
  bool importance_sampling = false;
  double importance_shift = 0.0;
  double control_vol = 0.0;
  std::uint64_t offset = 0;
  std::uint64_t seed = 0;

  CODEM_API Result price(const instrument::Vanilla& inst,
                         const model::BlackScholes& mdl,
                         const market::State& mkt,
                         RiskSpec risk = RiskSpec::PriceOnly,
                         pluss::codem::Config cfg = {}) const;

  CODEM_API Result price(const instrument::Vanilla& inst,
                         const model::Heston& mdl,
                         const market::State& mkt,
                         RiskSpec risk = RiskSpec::PriceOnly,
                         pluss::codem::Config cfg = {}) const;

  CODEM_API Result price(const instrument::Vanilla& inst,
                         const model::Bates& mdl,
                         const market::State& mkt,
                         RiskSpec risk = RiskSpec::PriceOnly,
                         pluss::codem::Config cfg = {}) const;

  CODEM_API Result price(const instrument::Vanilla& inst,
                         const model::Sabr& mdl,
                         const market::State& mkt,
                         RiskSpec risk = RiskSpec::PriceOnly,
                         pluss::codem::Config cfg = {}) const;

  CODEM_API Result price(const instrument::Vanilla& inst,
                         const model::RoughHeston& mdl,
                         const market::State& mkt,
                         RiskSpec risk = RiskSpec::PriceOnly,
                         pluss::codem::Config cfg = {}) const;

  CODEM_API Result price(const instrument::Vanilla& inst,
                         const model::RoughBergomi& mdl,
                         const market::State& mkt,
                         RiskSpec risk = RiskSpec::PriceOnly,
                         pluss::codem::Config cfg = {}) const;
};

struct Fft {
  double vol_risk_premium = 0.0;
  pricing::HestonCfMode mode = pricing::HestonCfMode::LittleTrap;
  std::int64_t num_fft = 4096;
  double du = 0.01;
  double dk = std::numeric_limits<double>::quiet_NaN();
  double alpha = 1.5;
  pricing::HestonFftQuadrature quadrature = pricing::HestonFftQuadrature::Simpson;
  bool return_grid = false;

  CODEM_API Result price(const instrument::Vanilla& inst,
                         const model::Heston& mdl,
                         const market::State& mkt,
                         RiskSpec risk = RiskSpec::PriceOnly,
                         pluss::codem::Config cfg = {}) const;

  CODEM_API Result price(const instrument::Vanilla& inst,
                         const model::Bates& mdl,
                         const market::State& mkt,
                         RiskSpec risk = RiskSpec::PriceOnly,
                         pluss::codem::Config cfg = {}) const;
};

struct COS {
  std::int64_t num_terms = 1024;
  double truncation = 12.0;
  double cumulant_step = 1e-3;

  CODEM_API Result price(const instrument::Vanilla& inst,
                         const model::Heston& mdl,
                         const market::State& mkt,
                         RiskSpec risk = RiskSpec::PriceOnly,
                         pluss::codem::Config cfg = {}) const;

  CODEM_API Result price(const instrument::Vanilla& inst,
                         const model::Bates& mdl,
                         const market::State& mkt,
                         RiskSpec risk = RiskSpec::PriceOnly,
                         pluss::codem::Config cfg = {}) const;
};

} // namespace pricer

} // namespace pluss::subsp::math::finance
