#pragma once

#include "pluss/codem/codem.h"
#include "pluss/config.h"
#include "pluss/export.h"
#include "pluss/subsp/math/finance/volatility.h"

#include <complex>
#include <cstdint>
#include <functional>
#include <limits>

namespace pluss::subsp::math::finance::pricing {

enum class OptionType : std::uint8_t { Call, Put, Auto };
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

enum class HestonFftQuadrature : std::uint8_t {
  Simpson = 0,
  Trapezoidal = 1,
};

struct HestonIntegrationOptions {
  std::int64_t integration_steps = 2048;
  double integration_u_max = 150.0;
  HestonCfMode mode = HestonCfMode::LittleTrap;
};

struct HestonFftOptions {
  double vol_risk_premium = 0.0;
  HestonCfMode mode = HestonCfMode::LittleTrap;
  std::int64_t num_fft = 4096;
  double characteristic_fcn_step = 0.01;
  double log_strike_step = std::numeric_limits<double>::quiet_NaN();
  double damping_factor = 1.5;
  HestonFftQuadrature quadrature = HestonFftQuadrature::Simpson;
};

struct HestonFftResult {
  pluss::codem::Series price = pluss::codem::Series::undefined();
  pluss::codem::Series strike = pluss::codem::Series::undefined();
};

struct CosOptions {
  std::int64_t num_terms = 1024;
  double truncation = 12.0;
  double cumulant_step = 1e-3;
};

using CharacteristicFn = std::function<std::complex<double>(std::complex<double>)>;

struct ComplexSeries {
  pluss::codem::Series real = pluss::codem::Series::undefined();
  pluss::codem::Series imag = pluss::codem::Series::undefined();
};

using CharacteristicSeriesFn = std::function<ComplexSeries(const pluss::codem::Series&)>;

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

enum class SabrApproxModel : std::uint8_t {
  Hagan2002 = 0,
  Obloj2008 = 1,
};

enum class SabrVolType : std::uint8_t {
  Lognormal = 0,
  Normal = 1,
};

struct SabrApproxOptions {
  SabrApproxModel model = SabrApproxModel::Hagan2002;
  double shift = 0.0;
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

CODEM_API HestonFftResult heston_price_fft_grid(OptionType type,
                                                double spot,
                                                double rate,
                                                double dividend_yield,
                                                double maturity,
                                                const volatility::HestonParams& params,
                                                const HestonFftOptions& options = {},
                                                pluss::codem::Config cfg = {});

CODEM_API double heston_price_fft(OptionType type,
                                  double spot,
                                  double strike,
                                  double rate,
                                  double dividend_yield,
                                  double maturity,
                                  const volatility::HestonParams& params,
                                  const HestonFftOptions& options = {},
                                  pluss::codem::Config cfg = {});

CODEM_API HestonFftResult option_price_fft_grid(OptionType type,
                                                double spot,
                                                double rate,
                                                double dividend_yield,
                                                double maturity,
                                                const CharacteristicFn& char_fn,
                                                const HestonFftOptions& options = {},
                                                pluss::codem::Config cfg = {});

CODEM_API double option_price_fft(OptionType type,
                                  double spot,
                                  double strike,
                                  double rate,
                                  double dividend_yield,
                                  double maturity,
                                  const CharacteristicFn& char_fn,
                                  const HestonFftOptions& options = {},
                                  pluss::codem::Config cfg = {});

CODEM_API double option_price_cos(OptionType type,
                                  double spot,
                                  double strike,
                                  double rate,
                                  double dividend_yield,
                                  double maturity,
                                  const CharacteristicFn& char_fn,
                                  const CosOptions& options = {},
                                  pluss::codem::Config cfg = {});

CODEM_API pluss::codem::Series option_price_cos(OptionType type,
                                                const pluss::codem::Series& spot,
                                                const pluss::codem::Series& strike,
                                                double rate,
                                                double dividend_yield,
                                                const pluss::codem::Series& maturity,
                                                const CharacteristicSeriesFn& char_fn,
                                                const CosOptions& options = {},
                                                pluss::codem::Config cfg = {});

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

CODEM_API HestonFftResult bates_price_fft_grid(OptionType type,
                                               double spot,
                                               double rate,
                                               double dividend_yield,
                                               double maturity,
                                               const volatility::BatesParams& params,
                                               const HestonFftOptions& options = {},
                                               pluss::codem::Config cfg = {});

CODEM_API double bates_price_fft(OptionType type,
                                 double spot,
                                 double strike,
                                 double rate,
                                 double dividend_yield,
                                 double maturity,
                                 const volatility::BatesParams& params,
                                 const HestonFftOptions& options = {},
                                 pluss::codem::Config cfg = {});

CODEM_API double heston_price_cos(OptionType type,
                                  double spot,
                                  double strike,
                                  double rate,
                                  double dividend_yield,
                                  double maturity,
                                  const volatility::HestonParams& params,
                                  const CosOptions& options = {},
                                  pluss::codem::Config cfg = {});

CODEM_API pluss::codem::Series heston_price_cos(OptionType type,
                                                const pluss::codem::Series& spot,
                                                const pluss::codem::Series& strike,
                                                double rate,
                                                double dividend_yield,
                                                const pluss::codem::Series& maturity,
                                                const volatility::HestonParams& params,
                                                const CosOptions& options = {},
                                                pluss::codem::Config cfg = {});

CODEM_API double bates_price_cos(OptionType type,
                                 double spot,
                                 double strike,
                                 double rate,
                                 double dividend_yield,
                                 double maturity,
                                 const volatility::BatesParams& params,
                                 const CosOptions& options = {},
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

CODEM_API double sabr_black_vol(double forward,
                                double strike,
                                double maturity,
                                const volatility::SabrParams& params,
                                const SabrApproxOptions& options = {});

CODEM_API double sabr_normal_vol(double forward,
                                 double strike,
                                 double maturity,
                                 const volatility::SabrParams& params,
                                 const SabrApproxOptions& options = {});

CODEM_API double sabr_price_approx(OptionType type,
                                   double forward,
                                   double strike,
                                   double rate,
                                   double maturity,
                                   const volatility::SabrParams& params,
                                   SabrVolType vol_type = SabrVolType::Lognormal,
                                   const SabrApproxOptions& options = {});

CODEM_API double slv_price_mc(OptionType type,
                              double spot,
                              double strike,
                              double rate,
                              double dividend_yield,
                              double maturity,
                              const volatility::SlvParams& params,
                              const MonteCarloOptions& options = {},
                              pluss::codem::Config cfg = {});

CODEM_API double rough_heston_price_mc(OptionType type,
                                       double spot,
                                       double strike,
                                       double rate,
                                       double dividend_yield,
                                       double maturity,
                                       const volatility::RoughHestonParams& params,
                                       const MonteCarloOptions& options = {},
                                       pluss::codem::Config cfg = {});

CODEM_API double rough_bergomi_price_mc(OptionType type,
                                        double spot,
                                        double strike,
                                        double rate,
                                        double dividend_yield,
                                        double maturity,
                                        const volatility::RoughBergomiParams& params,
                                        const MonteCarloOptions& options = {},
                                        pluss::codem::Config cfg = {});

} // namespace pluss::subsp::math::finance::pricing
