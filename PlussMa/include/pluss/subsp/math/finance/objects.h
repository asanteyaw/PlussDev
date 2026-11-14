#pragma once

#include "pluss/codem/codem.h"
#include "pluss/config.h"
#include "pluss/export.h"
#include "pluss/subsp/math/finance/pricing.h"
#include "pluss/subsp/math/finance/volatility.h"

#include <cstdint>
#include <limits>
#include <span>

namespace pluss::subsp::math::finance::object {

struct Vanilla {
  pricing::OptionType type = pricing::OptionType::Call;
  pluss::codem::Series is_call = pluss::series::undefined(); // required when type == OptionType::Auto
  pluss::codem::Series strike = pluss::series::undefined();
  pluss::codem::Series maturity = pluss::series::undefined();
  pricing::ExerciseStyle exercise = pricing::ExerciseStyle::European;
  pluss::codem::Series exercise_times = pluss::series::undefined(); // Bermudan: interim exercise dates in years (maturity is always exercisable)
};

struct MarketState {
  pluss::codem::Series spot = pluss::series::undefined();
  pluss::codem::Series rate = pluss::series::undefined();
  pluss::codem::Series dividend_yield = pluss::series::undefined();
};

namespace model {

struct BlackScholes {
  pluss::codem::Series volatility = pluss::series::undefined();
};

struct Heston {
  pluss::codem::Series kappa = pluss::series::undefined();
  pluss::codem::Series theta = pluss::series::undefined();
  pluss::codem::Series sigma = pluss::series::undefined();
  pluss::codem::Series rho = pluss::series::undefined();
  pluss::codem::Series v0 = pluss::series::undefined();
  pluss::codem::Series mu = pluss::series::undefined();
  pluss::codem::Series variance_floor = pluss::series::undefined();
};

struct Bates {
  pluss::codem::Series kappa = pluss::series::undefined();
  pluss::codem::Series theta = pluss::series::undefined();
  pluss::codem::Series sigma = pluss::series::undefined();
  pluss::codem::Series rho = pluss::series::undefined();
  pluss::codem::Series v0 = pluss::series::undefined();
  pluss::codem::Series mu = pluss::series::undefined();
  pluss::codem::Series variance_floor = pluss::series::undefined();
  pluss::codem::Series jump_intensity = pluss::series::undefined();
  pluss::codem::Series jump_mean = pluss::series::undefined();
  pluss::codem::Series jump_vol = pluss::series::undefined();
};

struct Sabr {
  pluss::codem::Series alpha0 = pluss::series::undefined();
  pluss::codem::Series beta = pluss::series::undefined();
  pluss::codem::Series nu = pluss::series::undefined();
  pluss::codem::Series rho = pluss::series::undefined();
  pluss::codem::Series mu = pluss::series::undefined();
  pluss::codem::Series alpha_floor = pluss::series::undefined();
  pricing::SabrVolType vol_type = pricing::SabrVolType::Lognormal;
  pricing::SabrApproxOptions approx{};
};

struct RoughHeston {
  pluss::codem::Series kappa = pluss::series::undefined();
  pluss::codem::Series theta = pluss::series::undefined();
  pluss::codem::Series sigma = pluss::series::undefined();
  pluss::codem::Series alpha = pluss::series::undefined();
  pluss::codem::Series rho = pluss::series::undefined();
  pluss::codem::Series v0 = pluss::series::undefined();
  pluss::codem::Series mu = pluss::series::undefined();
  pluss::codem::Series variance_floor = pluss::series::undefined();
};

struct RoughBergomi {
  pluss::codem::Series xi = pluss::series::undefined();
  pluss::codem::Series eta = pluss::series::undefined();
  pluss::codem::Series alpha = pluss::series::undefined();
  pluss::codem::Series rho = pluss::series::undefined();
  pluss::codem::Series mu = pluss::series::undefined();
  pluss::codem::Series variance_floor = pluss::series::undefined();
};

} // namespace model

namespace pricer {

struct Analytic {
  bool with_greeks = true;
};

struct Lattice {
  std::int64_t steps = 1000;
};

struct Spectral {
  pricing::SpectralOptions options{};
};

struct Pde {
  pricing::PdeOptions options{};
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
  pluss::codem::Series price = pluss::series::undefined();
  pluss::codem::Series delta = pluss::series::undefined();
  pluss::codem::Series gamma = pluss::series::undefined();
  pluss::codem::Series vega = pluss::series::undefined();
  pluss::codem::Series theta = pluss::series::undefined();
  pluss::codem::Series rho = pluss::series::undefined();
  pluss::codem::Series strike_grid = pluss::series::undefined();
};

CODEM_API PriceResult price(const Vanilla& inst,
                            const model::BlackScholes& mdl,
                            const MarketState& mkt,
                            const pricer::Analytic& eng = {},
                            pluss::codem::Config cfg = {});

CODEM_API PriceResult price(const Vanilla& inst,
                            const model::BlackScholes& mdl,
                            const MarketState& mkt,
                            const pricer::Lattice& eng,
                            pluss::codem::Config cfg = {});

CODEM_API PriceResult price(const Vanilla& inst,
                            const model::BlackScholes& mdl,
                            const MarketState& mkt,
                            const pricer::Cos& eng,
                            pluss::codem::Config cfg = {});

CODEM_API PriceResult price(const Vanilla& inst,
                            const model::BlackScholes& mdl,
                            const MarketState& mkt,
                            const pricer::Spectral& eng,
                            pluss::codem::Config cfg = {});

CODEM_API PriceResult price(const Vanilla& inst,
                            const model::Heston& mdl,
                            const MarketState& mkt,
                            const pricer::Pde& eng,
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

// European Heston / Bates options by the COS engine with the five Greeks in closed form, from the same characteristic-function pass as the price:
// delta and gamma in the spot, vega in the initial variance, theta in the time to maturity and rho in the rate.
CODEM_API PriceResult price_with_greeks(const Vanilla& inst,
                                        const model::Heston& mdl,
                                        const MarketState& mkt,
                                        const pricer::Cos& eng,
                                        pluss::codem::Config cfg = {});
CODEM_API PriceResult price_with_greeks(const Vanilla& inst,
                                        const model::Bates& mdl,
                                        const MarketState& mkt,
                                        const pricer::Cos& eng,
                                        pluss::codem::Config cfg = {});

// European Heston / Bates options by Monte Carlo with the Greeks from the shared paths: options on one asset share one simulation, delta and rho are
// pathwise, gamma and theta are differences over the same paths, and only vega needs two further simulations (same random numbers, bumped initial variance).
CODEM_API PriceResult price_with_greeks(const Vanilla& inst,
                                        const model::Heston& mdl,
                                        const MarketState& mkt,
                                        const pricer::MonteCarlo& eng,
                                        pluss::codem::Config cfg = {});
CODEM_API PriceResult price_with_greeks(const Vanilla& inst,
                                        const model::Bates& mdl,
                                        const MarketState& mkt,
                                        const pricer::MonteCarlo& eng,
                                        pluss::codem::Config cfg = {});
CODEM_API PriceResult price_with_greeks(const Vanilla& inst,
                                        const model::Sabr& mdl,
                                        const MarketState& mkt,
                                        const pricer::Analytic& eng,
                                        pluss::codem::Config cfg = {});
CODEM_API PriceResult price_with_greeks(const Vanilla& inst,
                                        const model::Sabr& mdl,
                                        const MarketState& mkt,
                                        const pricer::MonteCarlo& eng,
                                        pluss::codem::Config cfg = {});
CODEM_API PriceResult price_exercise_with_greeks(const Vanilla& inst,
                                                 const model::Heston& mdl,
                                                 const MarketState& mkt,
                                                 const pricer::Cos& eng,
                                                 pluss::codem::Config cfg = {});
CODEM_API PriceResult price_exercise_with_greeks(const Vanilla& inst,
                                                 const model::Bates& mdl,
                                                 const MarketState& mkt,
                                                 const pricer::Cos& eng,
                                                 pluss::codem::Config cfg = {});
CODEM_API PriceResult price_exercise_with_greeks(const Vanilla& inst,
                                                 const model::BlackScholes& mdl,
                                                 const MarketState& mkt,
                                                 const pricer::MonteCarlo& eng,
                                                 pluss::codem::Config cfg = {});
CODEM_API PriceResult price_exercise_with_greeks(const Vanilla& inst,
                                                 const model::Heston& mdl,
                                                 const MarketState& mkt,
                                                 const pricer::MonteCarlo& eng,
                                                 pluss::codem::Config cfg = {});
CODEM_API PriceResult price_exercise_with_greeks(const Vanilla& inst,
                                                 const model::Bates& mdl,
                                                 const MarketState& mkt,
                                                 const pricer::MonteCarlo& eng,
                                                 pluss::codem::Config cfg = {});
CODEM_API PriceResult price_exercise_with_greeks(const Vanilla& inst,
                                                 const model::Sabr& mdl,
                                                 const MarketState& mkt,
                                                 const pricer::MonteCarlo& eng,
                                                 pluss::codem::Config cfg = {});
CODEM_API PriceResult price_exercise_with_greeks(const Vanilla& inst,
                                                 const model::RoughHeston& mdl,
                                                 const MarketState& mkt,
                                                 const pricer::MonteCarlo& eng,
                                                 pluss::codem::Config cfg = {});
CODEM_API PriceResult price_exercise_with_greeks(const Vanilla& inst,
                                                 const model::RoughBergomi& mdl,
                                                 const MarketState& mkt,
                                                 const pricer::MonteCarlo& eng,
                                                 pluss::codem::Config cfg = {});
CODEM_API PriceResult price_with_greeks(const Vanilla& inst,
                                        const model::BlackScholes& mdl,
                                        const MarketState& mkt,
                                        const pricer::Cos& eng,
                                        pluss::codem::Config cfg = {});
CODEM_API PriceResult price_with_greeks(const Vanilla& inst,
                                        const model::BlackScholes& mdl,
                                        const MarketState& mkt,
                                        const pricer::Spectral& eng,
                                        pluss::codem::Config cfg = {});
CODEM_API PriceResult price_with_greeks(const Vanilla& inst,
                                        const model::BlackScholes& mdl,
                                        const MarketState& mkt,
                                        const pricer::MonteCarlo& eng,
                                        pluss::codem::Config cfg = {});
CODEM_API PriceResult price_with_greeks(const Vanilla& inst,
                                        const model::RoughHeston& mdl,
                                        const MarketState& mkt,
                                        const pricer::MonteCarlo& eng,
                                        pluss::codem::Config cfg = {});
CODEM_API PriceResult price_with_greeks(const Vanilla& inst,
                                        const model::RoughBergomi& mdl,
                                        const MarketState& mkt,
                                        const pricer::MonteCarlo& eng,
                                        pluss::codem::Config cfg = {});

// European Heston / Bates options by the Fourier-FFT engine with Greeks: one strike grid serves every strike (delta and gamma by homogeneity), and the
// characteristic function multiplied by the factor of each remaining Greek gives vega, rho and theta by three more transforms. LittleTrap form only.
CODEM_API PriceResult price_with_greeks(const Vanilla& inst,
                                        const model::Heston& mdl,
                                        const MarketState& mkt,
                                        const pricer::Fft& eng,
                                        pluss::codem::Config cfg = {});
CODEM_API PriceResult price_with_greeks(const Vanilla& inst,
                                        const model::Bates& mdl,
                                        const MarketState& mkt,
                                        const pricer::Fft& eng,
                                        pluss::codem::Config cfg = {});

// Greeks read off the solved grid: the lattice (Black-Scholes; any exercise style) takes delta, gamma and theta from the nodes of the first levels, the Heston PDE
// takes delta, gamma, vega and theta from the surface; only rho (and, for the lattice, vega) need further pricings.
CODEM_API PriceResult price_with_greeks(const Vanilla& inst,
                                        const model::BlackScholes& mdl,
                                        const MarketState& mkt,
                                        const pricer::Lattice& eng,
                                        pluss::codem::Config cfg = {});
CODEM_API PriceResult price_with_greeks(const Vanilla& inst,
                                        const model::Heston& mdl,
                                        const MarketState& mkt,
                                        const pricer::Pde& eng,
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

// Finance quantities are doubles: the factories below store Float64 unless told otherwise.
inline pluss::codem::Config f64_default() { return pluss::codem::Config{}.with_dtype(pluss::ScalarType::Float64); }

namespace instrument {
using Vanilla = object::Vanilla;

CODEM_API Vanilla make(pricing::OptionType type,
                       double strike,
                       double maturity,
                       pluss::codem::Config cfg = f64_default());

CODEM_API Vanilla american(pricing::OptionType type,
                           double strike,
                           double maturity,
                           pluss::codem::Config cfg = f64_default());

CODEM_API Vanilla bermudan(pricing::OptionType type,
                           double strike,
                           double maturity,
                           std::span<const double> dates,
                           pluss::codem::Config cfg = f64_default());
}

namespace market {
using State = object::MarketState;

CODEM_API State make(double spot, double rate, double dividend_yield, pluss::codem::Config cfg = f64_default());
}

namespace model {
using BlackScholes = object::model::BlackScholes;
using Heston = object::model::Heston;
using Bates = object::model::Bates;
using Sabr = object::model::Sabr;
using RoughHeston = object::model::RoughHeston;
using RoughBergomi = object::model::RoughBergomi;
}

namespace pricer {

enum class RiskSpec : std::uint8_t {
  PriceOnly = 0,
  All = 1,
};

struct Greeks {
  pluss::codem::Series delta = pluss::series::undefined();
  pluss::codem::Series gamma = pluss::series::undefined();
  pluss::codem::Series vega = pluss::series::undefined();
  pluss::codem::Series theta = pluss::series::undefined();
  pluss::codem::Series rho = pluss::series::undefined();
};

struct Result {
  pluss::codem::Series price = pluss::series::undefined();
  Greeks greeks{};
  pluss::codem::Series strike_grid = pluss::series::undefined();
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

struct Lattice {
  std::int64_t steps = 1000;

  CODEM_API Result price(const instrument::Vanilla& inst,
                         const model::BlackScholes& mdl,
                         const market::State& mkt,
                         RiskSpec risk = RiskSpec::All,
                         pluss::codem::Config cfg = {}) const;
};

struct Spectral {
  std::int64_t nodes = 24;
  std::int64_t quadrature = 48;
  std::int64_t max_iterations = 60;
  double tolerance = 1e-13;

  CODEM_API Result price(const instrument::Vanilla& inst,
                         const model::BlackScholes& mdl,
                         const market::State& mkt,
                         RiskSpec risk = RiskSpec::All,
                         pluss::codem::Config cfg = {}) const;
};

struct Pde {
  std::int64_t spot_nodes = 240;
  std::int64_t variance_nodes = 60;
  std::int64_t time_steps = 100;

  CODEM_API Result price(const instrument::Vanilla& inst,
                         const model::Heston& mdl,
                         const market::State& mkt,
                         RiskSpec risk = RiskSpec::All,
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
  std::int64_t basis_degree = 3;

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
  std::int64_t num_fft = 16384;
  double du = 0.1;
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
  double exercise_truncation = 9.0;
  std::int64_t exercise_terms = 128;
  std::int64_t extrapolation = -1;
  std::int64_t variance_nodes = 128;
  double variance_tolerance = 1e-8;

  CODEM_API Result price(const instrument::Vanilla& inst,
                         const model::BlackScholes& mdl,
                         const market::State& mkt,
                         RiskSpec risk = RiskSpec::All,
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
};

} // namespace pricer

} // namespace pluss::subsp::math::finance
