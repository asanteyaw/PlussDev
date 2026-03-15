#pragma once

#include "pluss/codem/codem.h"
#include "pluss/config.h"
#include "pluss/export.h"
#include "pluss/subsp/math/finance/volatility.h"

#include <cstdint>

namespace pluss::subsp::math::finance::pricing {

enum class OptionType : std::uint8_t { Call, Put };
inline constexpr OptionType Call = OptionType::Call;
inline constexpr OptionType Put = OptionType::Put;

enum class ImpliedVolMethod : std::uint8_t { Auto, Newton, Bisection, Secant, Brent };
inline constexpr ImpliedVolMethod Auto = ImpliedVolMethod::Auto;
inline constexpr ImpliedVolMethod Newton = ImpliedVolMethod::Newton;
inline constexpr ImpliedVolMethod Bisection = ImpliedVolMethod::Bisection;
inline constexpr ImpliedVolMethod Secant = ImpliedVolMethod::Secant;
inline constexpr ImpliedVolMethod Brent = ImpliedVolMethod::Brent;

struct ImpliedVolOptions {
  ImpliedVolMethod method = ImpliedVolMethod::Auto;
  double lower_vol = 1e-8;
  double upper_vol = 10.0;
  double tolerance = 1e-8;
  std::int64_t max_iterations = 128;
  double vega_floor = 1e-12;
};

enum class HestonCfMode : std::uint8_t {
  Original = 0,
  LittleTrap = 1,
};

struct HestonIntegrationOptions {
  std::int64_t integration_steps = 2048;
  double integration_u_max = 150.0;
  HestonCfMode mode = HestonCfMode::LittleTrap;
};

struct HestonNandiIntegrationOptions {
  std::int64_t integration_steps = 4096;
  double integration_u_max = 200.0;
  std::int64_t periods_per_year = 252;
};

struct MonteCarloOptions {
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
};

struct BlackScholesGreeks {
  double delta = 0.0;
  double gamma = 0.0;
  double vega = 0.0;
  double theta = 0.0;
  double rho = 0.0;
};

struct BlackScholesSeriesGreeks {
  pluss::codem::Series delta = pluss::codem::Series::undefined();
  pluss::codem::Series gamma = pluss::codem::Series::undefined();
  pluss::codem::Series vega = pluss::codem::Series::undefined();
  pluss::codem::Series theta = pluss::codem::Series::undefined();
  pluss::codem::Series rho = pluss::codem::Series::undefined();
};

CODEM_API double black_scholes_price(OptionType type,
                                     double spot,
                                     double strike,
                                     double rate,
                                     double dividend_yield,
                                     double maturity,
                                     double volatility);

CODEM_API pluss::codem::Series black_scholes_price(OptionType type,
                                                   const pluss::codem::Series& spot,
                                                   double strike,
                                                   double rate,
                                                   double dividend_yield,
                                                   double maturity,
                                                   double volatility,
                                                   pluss::codem::Config cfg = {});

CODEM_API pluss::codem::Series black_scholes_price(OptionType type,
                                                   const pluss::codem::Series& spot,
                                                   const pluss::codem::Series& time_to_maturity,
                                                   double strike,
                                                   double rate,
                                                   double dividend_yield,
                                                   double volatility,
                                                   pluss::codem::Config cfg = {});

CODEM_API BlackScholesGreeks black_scholes_greeks(OptionType type,
                                                  double spot,
                                                  double strike,
                                                  double rate,
                                                  double dividend_yield,
                                                  double maturity,
                                                  double volatility);

CODEM_API BlackScholesSeriesGreeks black_scholes_greeks(OptionType type,
                                                        const pluss::codem::Series& spot,
                                                        double strike,
                                                        double rate,
                                                        double dividend_yield,
                                                        double maturity,
                                                        double volatility,
                                                        pluss::codem::Config cfg = {});

CODEM_API BlackScholesSeriesGreeks black_scholes_greeks(OptionType type,
                                                        const pluss::codem::Series& spot,
                                                        const pluss::codem::Series& time_to_maturity,
                                                        double strike,
                                                        double rate,
                                                        double dividend_yield,
                                                        double volatility,
                                                        pluss::codem::Config cfg = {});

CODEM_API double black_scholes_delta(OptionType type,
                                     double spot,
                                     double strike,
                                     double rate,
                                     double dividend_yield,
                                     double maturity,
                                     double volatility);
CODEM_API double black_scholes_gamma(double spot,
                                     double strike,
                                     double rate,
                                     double dividend_yield,
                                     double maturity,
                                     double volatility);
CODEM_API double black_scholes_vega(double spot,
                                    double strike,
                                    double rate,
                                    double dividend_yield,
                                    double maturity,
                                    double volatility);
CODEM_API double black_scholes_theta(OptionType type,
                                     double spot,
                                     double strike,
                                     double rate,
                                     double dividend_yield,
                                     double maturity,
                                     double volatility);
CODEM_API double black_scholes_rho(OptionType type,
                                   double spot,
                                   double strike,
                                   double rate,
                                   double dividend_yield,
                                   double maturity,
                                   double volatility);

CODEM_API double implied_vol(OptionType type,
                             double option_price,
                             double spot,
                             double strike,
                             double rate,
                             double dividend_yield,
                             double maturity,
                             const ImpliedVolOptions& options = {});

CODEM_API pluss::codem::Series implied_vol(OptionType type,
                                           const pluss::codem::Series& option_price,
                                           const pluss::codem::Series& spot,
                                           double strike,
                                           double rate,
                                           double dividend_yield,
                                           double maturity,
                                           const ImpliedVolOptions& options = {},
                                           pluss::codem::Config cfg = {});

CODEM_API pluss::codem::Series implied_vol(OptionType type,
                                           const pluss::codem::Series& option_price,
                                           const pluss::codem::Series& spot,
                                           const pluss::codem::Series& time_to_maturity,
                                           double strike,
                                           double rate,
                                           double dividend_yield,
                                           const ImpliedVolOptions& options = {},
                                           pluss::codem::Config cfg = {});

CODEM_API double heston_price(OptionType type,
                              double spot,
                              double strike,
                              double rate,
                              double dividend_yield,
                              double maturity,
                              const volatility::HestonParams& params,
                              const HestonIntegrationOptions& options = {});

CODEM_API double heston_price_mc(OptionType type,
                                 double spot,
                                 double strike,
                                 double rate,
                                 double dividend_yield,
                                 double maturity,
                                 const volatility::HestonParams& params,
                                 const MonteCarloOptions& options = {},
                                 pluss::codem::Config cfg = {});

CODEM_API double bates_price_mc(OptionType type,
                                double spot,
                                double strike,
                                double rate,
                                double dividend_yield,
                                double maturity,
                                const volatility::BatesParams& params,
                                const MonteCarloOptions& options = {},
                                pluss::codem::Config cfg = {});

CODEM_API double sv_price_mc(OptionType type,
                             double spot,
                             double strike,
                             double rate,
                             double dividend_yield,
                             double maturity,
                             const volatility::SvParams& params,
                             const MonteCarloOptions& options = {},
                             pluss::codem::Config cfg = {});

CODEM_API double heston_nandi_price(OptionType type,
                                    double spot,
                                    double strike,
                                    double rate,
                                    double dividend_yield,
                                    double maturity,
                                    const volatility::HestonNandiParams& params,
                                    const HestonNandiIntegrationOptions& options = {});

CODEM_API double heston_nandi_price_mc(OptionType type,
                                       double spot,
                                       double strike,
                                       double rate,
                                       double dividend_yield,
                                       double maturity,
                                       const volatility::HestonNandiParams& params,
                                       const MonteCarloOptions& options = {},
                                       pluss::codem::Config cfg = {});

CODEM_API double sabr_price_mc(OptionType type,
                               double forward,
                               double strike,
                               double rate,
                               double maturity,
                               const volatility::SabrParams& params,
                               const MonteCarloOptions& options = {},
                               pluss::codem::Config cfg = {});

CODEM_API double slv_price_mc(OptionType type,
                              double spot,
                              double strike,
                              double rate,
                              double dividend_yield,
                              double maturity,
                              const volatility::SlvParams& params,
                              const MonteCarloOptions& options = {},
                              pluss::codem::Config cfg = {});

} // namespace pluss::subsp::math::finance::pricing
