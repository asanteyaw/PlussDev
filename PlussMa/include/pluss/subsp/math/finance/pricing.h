#pragma once

#include "pluss/codem/codem.h"
#include "pluss/config.h"
#include "pluss/export.h"
#include "pluss/subsp/math/finance/volatility.h"

#include <complex>
#include <cstdint>
#include <functional>
#include <limits>
#include <span>

namespace pluss::subsp::math::finance::pricing {

enum class OptionType : std::uint8_t { Call, Put, Auto };
inline constexpr OptionType Call = OptionType::Call;
inline constexpr OptionType Put = OptionType::Put;

// When the holder may exercise: only at maturity (European), on a fixed schedule plus maturity (Bermudan), or at any time up to
// maturity (American).  A Bermudan with no interim dates is a European; an American is the limit of Bermudans on ever finer schedules.
enum class ExerciseStyle : std::uint8_t { European, Bermudan, American };

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
  std::int64_t num_fft = 16384;
  double characteristic_fcn_step = 0.1;
  double log_strike_step = std::numeric_limits<double>::quiet_NaN();
  double damping_factor = 1.5;
  HestonFftQuadrature quadrature = HestonFftQuadrature::Simpson;
};

struct HestonFftResult {
  pluss::codem::Series price = pluss::series::undefined();
  pluss::codem::Series strike = pluss::series::undefined();
};

struct CosOptions {
  std::int64_t num_terms = 1024;
  double truncation = 12.0;
  double cumulant_step = 1e-3;
  double exercise_truncation = 9.0;   // early exercise: half-width of the log-asset range in standard deviations (a wider range lets the growing call payoff leak error into the recursion, a narrower one truncates the tails)
  std::int64_t exercise_terms = 128;  // cosine terms of the early-exercise recursion (the value has a kink at the exercise point)
  std::int64_t extrapolation = -1;    // American: Richardson over Bermudans with 2^d ... 2^(d+3) dates; -1 means d = 3 for Black-Scholes, 2 for Heston and Bates
  std::int64_t variance_nodes = 128;   // Heston/Bates early exercise: Gauss-Legendre nodes in the log-variance dimension
  double variance_tolerance = 1e-8;   // Heston/Bates early exercise: probability left outside the log-variance range
};

using CharacteristicFn = std::function<std::complex<double>(std::complex<double>)>;

struct ComplexSeries {
  pluss::codem::Series real = pluss::series::undefined();
  pluss::codem::Series imag = pluss::series::undefined();
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
  std::int64_t basis_degree = 3; // American/Bermudan only: polynomial degree of the continuation-value regression
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
  pluss::codem::Series delta = pluss::series::undefined();
  pluss::codem::Series gamma = pluss::series::undefined();
  pluss::codem::Series vega = pluss::series::undefined();
  pluss::codem::Series theta = pluss::series::undefined();
  pluss::codem::Series rho = pluss::series::undefined();
};














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



















// Cox-Ross-Rubinstein binomial lattice for one option under Black-Scholes dynamics.  `dates` are the interim Bermudan exercise
// times in years (dates after `maturity` are not part of this option's life; maturity itself is always exercisable); they are
// snapped to the nearest lattice step.  Ignored unless style == Bermudan.

// Longstaff-Schwartz least-squares Monte Carlo for one option on simulated asset paths shaped [paths, steps + 1] on an equally
// spaced grid over [0, maturity].  The continuation value is regressed on 1, x, ..., x^degree with x = S / strike, using only the
// in-the-money paths.  Model-agnostic: pass the paths of any simulator.  The result is a low-biased estimate (the exercise policy
// is only as good as the regression).


struct SpectralOptions {
  std::int64_t nodes = 24;           // Chebyshev nodes of the boundary interpolant (n + 1 points)
  std::int64_t quadrature = 48;      // Gauss-Legendre points of each time integral
  std::int64_t max_iterations = 60;  // fixed-point sweeps of the boundary equation
  double tolerance = 1e-13;          // relative change of the boundary that ends the sweeps
};

// American option under Black-Scholes by the integral-equation formulation (Andersen, Lake and Offengenden): the exercise boundary
// B(tau) solves a fixed-point equation on Chebyshev nodes, and the price is the European value plus an early-exercise premium
// integrated along the boundary.  Calls are priced as puts by the put-call symmetry.  Accurate to about 10 digits; the cost is
// independent of the spot.

// The critical asset price at each time-to-maturity in `times` (each in [0, maturity]): exercise is optimal below it for a put
// and above it for a call.


struct PdeOptions {
  std::int64_t spot_nodes = 240;      // nodes of the asset grid, concentrated around the strike
  std::int64_t variance_nodes = 60;   // nodes of the variance grid, concentrated near zero
  std::int64_t time_steps = 100;      // steps from maturity back to today
};

// Heston price by a two-dimensional PDE: second-order finite differences on stretched grids, the Modified Craig-Sneyd ADI scheme
// in time (after two Rannacher-smoothing steps), and for American exercise the Ikonen-Toivanen splitting of the exercise constraint
// (Haentjens and in 't Hout).  A Bermudan option projects onto the payoff only on `dates`, in years from today; a European one never.


// Black-Scholes option by the COS method of Fang and Oosterlee (Numer. Math. 2009): European, Bermudan on `dates` (years from today, the
// maturity always included) or American, the latter by four-point Richardson extrapolation over Bermudans with 2^d ... 2^(d+3) equally
// spaced dates.  Exponentially convergent in the number of cosine terms.


// Heston (or Bates) option with early exercise by the COS method of Fang and Oosterlee (SIAM J. Financial Math. 2011): a cosine expansion
// in the log-asset dimension and Gauss-Legendre quadrature in the log-variance dimension.  Same conventions as black_scholes_price_cos;
// the American price is the same Richardson extrapolation over Bermudans.


// Implementation layer behind the pricing engines (module::Field, pricer::*). Not a public interface: reach these through a Field or an engine.
namespace detail {

// Parameters of Heston (or Bates, with the three jump members) as Series, one entry per option or a single entry shared by all.
struct SvParamsSeries {
  pluss::codem::Series kappa = pluss::series::undefined();
  pluss::codem::Series theta = pluss::series::undefined();
  pluss::codem::Series sigma = pluss::series::undefined();
  pluss::codem::Series rho = pluss::series::undefined();
  pluss::codem::Series v0 = pluss::series::undefined();
  pluss::codem::Series jump_intensity = pluss::series::undefined();
  pluss::codem::Series jump_mean = pluss::series::undefined();
  pluss::codem::Series jump_vol = pluss::series::undefined();
};

// The characteristic function of ln S_T and its two logarithmic derivatives, d log phi / d v0 and d log phi / d tau, at the frequencies u ([N, 1]) for every option
// (columns): what the closed-form COS Greeks need.
struct CosCfPieces {
  ComplexSeries phi;
  ComplexSeries dlog_v0;
  ComplexSeries dlog_tau;
};
using CosCfPiecesFn = std::function<CosCfPieces(const pluss::codem::Series&)>;

// Price and Greeks of European options by the COS series, from one pass over the characteristic function: delta and gamma in the spot, vega in the
// initial variance, theta in the time to maturity (as -dV/dtau) and rho in the rate. `is_call` is 1 for a call and 0 for a put, per option.
struct CosGreeksV {
  pluss::codem::Series price, delta, gamma, vega, theta, rho;
};
CODEM_API CosGreeksV cos_greeks_v(const pluss::codem::Series& is_call,
                                  const pluss::codem::Series& spot,
                                  const pluss::codem::Series& strike,
                                  const pluss::codem::Series& rate,
                                  const pluss::codem::Series& dividend_yield,
                                  const pluss::codem::Series& maturity,
                                  const CosCfPiecesFn& pieces,
                                  const CosOptions& options = {},
                                  pluss::codem::Config cfg = {});
CODEM_API CosGreeksV sv_cos_greeks_v(const pluss::codem::Series& is_call,
                                     const pluss::codem::Series& spot,
                                     const pluss::codem::Series& strike,
                                     const pluss::codem::Series& rate,
                                     const pluss::codem::Series& dividend_yield,
                                     const pluss::codem::Series& maturity,
                                     const SvParamsSeries& params,
                                     const CosOptions& options = {},
                                     pluss::codem::Config cfg = {});

// Monte Carlo prices and Greeks of several European options on ONE asset (same spot, rate, yield and maturity) from a single simulation of Heston or Bates
// paths: every strike reuses the paths. With `greeks`, delta is pathwise, gamma a central second difference of the payoff on the same paths with the spot scaled
// (paths scale with the spot), rho pathwise, theta a second-order backward difference over the earlier dates of the same paths, and vega pathwise
// (the derivative of the paths in the initial variance). Vectors have one entry per strike.
struct McStripGreeks {
  std::vector<double> price, delta, gamma, vega, theta, rho;
};
CODEM_API McStripGreeks sv_mc_strip(const volatility::BatesParams& params,
                                    bool bates,
                                    double spot,
                                    double rate,
                                    double dividend_yield,
                                    double maturity,
                                    const std::vector<double>& strikes,
                                    const std::vector<bool>& is_call,
                                    const MonteCarloOptions& options,
                                    bool greeks,
                                    pluss::codem::Config cfg = {});

// The same shared-path strip for the rough models (vega is the derivative in the initial variance for rough Heston and in the forward-variance level xi for rough Bergomi).
CODEM_API McStripGreeks rough_heston_mc_strip(const volatility::RoughHestonParams& params, double spot, double rate, double dividend_yield, double maturity,
                                              const std::vector<double>& strikes, const std::vector<bool>& is_call, const MonteCarloOptions& options, bool greeks,
                                              pluss::codem::Config cfg = {});
CODEM_API McStripGreeks rough_bergomi_mc_strip(const volatility::RoughBergomiParams& params, double spot, double rate, double dividend_yield, double maturity,
                                               const std::vector<double>& strikes, const std::vector<bool>& is_call, const MonteCarloOptions& options, bool greeks,
                                               pluss::codem::Config cfg = {});

// One asset's inputs to sv_mc_assets: the model, the market and the options of the strip (all at the group's common maturity).
struct McAsset {
  volatility::BatesParams params;
  bool bates = false;
  double spot = 100.0, rate = 0.0, dividend_yield = 0.0;
  std::vector<double> strikes;
  std::vector<bool> is_call;
};
// Monte Carlo prices (and Greeks) of the strips of SEVERAL assets from ONE simulation with an asset axis (volatility::detail::multi_sv_paths, independent assets):
// plain estimators only (no control variate or importance sampling). Vega is pathwise for all assets together.
CODEM_API std::vector<McStripGreeks> sv_mc_assets(const std::vector<McAsset>& assets,
                                                  double maturity,
                                                  const MonteCarloOptions& options,
                                                  bool greeks,
                                                  pluss::codem::Config cfg = {});

// Fourier-FFT prices (and Greeks) of several European options on ONE asset from a single transform: every strike is read off the same strike grid. Greeks
// come from the same grid (delta and gamma by homogeneity in spot and strike) and from three more transforms of the characteristic function multiplied by the
// factor of the Greek (vega in the initial variance, rho, theta): four transforms instead of the repricing of every strike. LittleTrap form only.
CODEM_API McStripGreeks sv_fft_strip(const volatility::BatesParams& params,
                                     bool bates,
                                     double spot,
                                     double rate,
                                     double dividend_yield,
                                     double maturity,
                                     const std::vector<double>& strikes,
                                     const std::vector<bool>& is_call,
                                     const HestonFftOptions& options,
                                     bool greeks,
                                     pluss::codem::Config cfg = {});

// Price and the five Greeks of one option from the solved grid instead of repricing it. Lattice: delta and gamma from the nodes of the first two levels, theta from
// the centre node of the second (the tree is recombining, so it sits at the spot two steps later), vega and rho by carrying their derivatives through the recursion. Heston
// PDE: delta, gamma and vega are read off the solved surface, theta follows from the PDE itself in the continuation region, and rho differentiates every step of the scheme.
struct ScalarGreeks {
  double price = 0.0, delta = 0.0, gamma = 0.0, vega = 0.0, theta = 0.0, rho = 0.0;
};
CODEM_API ScalarGreeks lattice_greeks(OptionType type, ExerciseStyle style, double spot, double strike, double rate, double dividend_yield, double maturity,
                                      double volatility, std::int64_t steps, std::span<const double> dates = {}, pluss::codem::Config cfg = {});
CODEM_API ScalarGreeks heston_pde_greeks(OptionType type, ExerciseStyle style, double spot, double strike, double rate, double dividend_yield, double maturity,
                                         const volatility::HestonParams& params, std::span<const double> dates = {}, const PdeOptions& options = {});
// American options by the spectral engine with the Greeks from the solved boundary: delta and gamma in the spot and theta in the time to maturity cost no further
// solve (the boundary does not depend on the spot and is stationary in the time to maturity), vega and rho differentiate the boundary equation exactly.
CODEM_API ScalarGreeks spectral_greeks(OptionType type, double spot, double strike, double rate, double dividend_yield, double maturity, double volatility,
                                       const SpectralOptions& options = {});

// SABR (the Analytic engine): the price Black(F, K, r, tau, sigma(F, tau, alpha0)) of the expansion, with the Greeks of Black's (or Bachelier's) formula exact and the
// derivatives of the smile taken on the expansion itself, so no price is bumped. Vega is the derivative in alpha0.
CODEM_API ScalarGreeks sabr_greeks(OptionType type, double spot, double strike, double rate, double dividend_yield, double maturity, const volatility::SabrParams& params,
                                   SabrVolType vol_type = SabrVolType::Lognormal, const SabrApproxOptions& options = {});

// Bermudan and American options under Black-Scholes by the COS recursion: delta and gamma are read off the final cosine expansion (no further recursion), the other
// Greeks differentiate the recursion itself (vega, rho, theta).
CODEM_API ScalarGreeks black_scholes_cos_greeks(OptionType type, ExerciseStyle style, double spot, double strike, double rate, double dividend_yield, double maturity,
                                                double volatility, std::span<const double> dates = {}, const CosOptions& options = {});
// The same for Bermudan and American Heston and Bates options by the COS recursion: delta, gamma and vega (in the initial variance) come from the final expansion,
// rho comes through the recursion and theta from the pricing equation.
CODEM_API ScalarGreeks heston_cos_exercise_greeks(OptionType type, ExerciseStyle style, double spot, double strike, double rate, double dividend_yield, double maturity,
                                                  const volatility::HestonParams& params, std::span<const double> dates = {}, const CosOptions& options = {});
CODEM_API ScalarGreeks bates_cos_exercise_greeks(OptionType type, ExerciseStyle style, double spot, double strike, double rate, double dividend_yield, double maturity,
                                                 const volatility::BatesParams& params, std::span<const double> dates = {}, const CosOptions& options = {});

// COS pricing with a rate and dividend yield per option, and Heston / Bates parameters per option: one call prices options on different assets.
CODEM_API pluss::codem::Series option_price_cos_v(OptionType type,
                                                  const pluss::codem::Series& spot,
                                                  const pluss::codem::Series& strike,
                                                  const pluss::codem::Series& rate,
                                                  const pluss::codem::Series& dividend_yield,
                                                  const pluss::codem::Series& maturity,
                                                  const CharacteristicSeriesFn& char_fn,
                                                  const CosOptions& options = {},
                                                  pluss::codem::Config cfg = {});
CODEM_API pluss::codem::Series heston_price_cos_v(OptionType type,
                                                  const pluss::codem::Series& spot,
                                                  const pluss::codem::Series& strike,
                                                  const pluss::codem::Series& rate,
                                                  const pluss::codem::Series& dividend_yield,
                                                  const pluss::codem::Series& maturity,
                                                  const SvParamsSeries& params,
                                                  const CosOptions& options = {},
                                                  pluss::codem::Config cfg = {});
CODEM_API pluss::codem::Series bates_price_cos_v(OptionType type,
                                                 const pluss::codem::Series& spot,
                                                 const pluss::codem::Series& strike,
                                                 const pluss::codem::Series& rate,
                                                 const pluss::codem::Series& dividend_yield,
                                                 const pluss::codem::Series& maturity,
                                                 const SvParamsSeries& params,
                                                 const CosOptions& options = {},
                                                 pluss::codem::Config cfg = {});

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

// SABR by Monte Carlo with the Greeks from one simulation: the initial forward (one copy per path) and the initial volatility are tracked so that delta, vega and the
// pathwise Jacobian of the terminal forward come from the paths themselves; gamma is the second difference of the payoff along that Jacobian, rho follows from the
// discount factor and the forward's growth, theta adds the time derivative of the forward process (backward difference over the earlier dates of the same paths).
CODEM_API ScalarGreeks sabr_mc_greeks(OptionType type, double spot, double strike, double rate, double dividend_yield, double maturity, const volatility::SabrParams& params,
                                      const MonteCarloOptions& options, pluss::codem::Config cfg = {});

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

CODEM_API double lattice_price(OptionType type,
                               ExerciseStyle style,
                               double spot,
                               double strike,
                               double rate,
                               double dividend_yield,
                               double maturity,
                               double volatility,
                               std::int64_t steps = 1000,
                               std::span<const double> dates = {},
                               pluss::codem::Config cfg = {});

CODEM_API double lsm_price(OptionType type,
                           ExerciseStyle style,
                           double strike,
                           double rate,
                           double maturity,
                           const pluss::codem::Series& asset_paths,
                           std::span<const double> dates = {},
                           std::int64_t degree = 3);
// What a Longstaff-Schwartz simulation may be asked to track: the volatility level (initial variance, forward variance, initial or constant volatility), the initial
// spot as one copy per path, and the drift rate - dividend yield, each a scalar (the spot a [paths] vector) the paths are differentiable in. `spot_scale` multiplies the
// initial spot of the simulation (gamma of a model whose paths are not proportional to the spot). Entries that are null are the numeric values of the model.
struct LsmTracked {
  const pluss::codem::Series* level = nullptr;
  const pluss::codem::Series* spot = nullptr;
  const pluss::codem::Series* drift = nullptr;
  double spot_scale = 1.0;
};
// How the model's paths respond to the spot, and the generator for the closed-form theta: `homogeneous` paths are proportional to the initial spot (and carry the
// drift as the factor exp((r - q) t)); otherwise delta, rho and gamma come from the tracked spot and drift. `bs_volatility` > 0 marks a constant-volatility model
// whose theta follows from its pricing equation.
struct LsmModel {
  bool homogeneous = true;
  double carry = 0.0;
  double bs_volatility = 0.0;
};
// Price and Greeks of an American or Bermudan option by Longstaff-Schwartz from the paths of one simulation, with no repricing. `simulate(tracked)` returns the paths
// [paths, steps + 1]. The regression is fitted once; delta and rho are pathwise along the fitted exercise rule (the value is stationary in the rule), gamma applies
// the same rule to the paths with the spot scaled, vega differentiates the paths in the level, and theta is the pricing equation at the spot (constant
// volatility) or the second-order backward difference of the fit over the two earlier horizons of the same paths.
CODEM_API ScalarGreeks lsm_greeks(OptionType type, ExerciseStyle style, double spot, double strike, double rate, double maturity, double level,
                                  const std::function<pluss::codem::Series(const LsmTracked&)>& simulate,
                                  std::span<const double> dates = {}, std::int64_t degree = 3, const LsmModel& model = {});

CODEM_API double spectral_price(OptionType type,
                                double spot,
                                double strike,
                                double rate,
                                double dividend_yield,
                                double maturity,
                                double volatility,
                                const SpectralOptions& options = {});

CODEM_API pluss::codem::Series spectral_boundary(OptionType type,
                                                 double strike,
                                                 double rate,
                                                 double dividend_yield,
                                                 double maturity,
                                                 double volatility,
                                                 const pluss::codem::Series& times,
                                                 const SpectralOptions& options = {});

CODEM_API double heston_price_pde(OptionType type,
                                  ExerciseStyle style,
                                  double spot,
                                  double strike,
                                  double rate,
                                  double dividend_yield,
                                  double maturity,
                                  const volatility::HestonParams& params,
                                  std::span<const double> dates = {},
                                  const PdeOptions& options = {});

CODEM_API double black_scholes_price_cos(OptionType type,
                                         ExerciseStyle style,
                                         double spot,
                                         double strike,
                                         double rate,
                                         double dividend_yield,
                                         double maturity,
                                         double volatility,
                                         std::span<const double> dates = {},
                                         const CosOptions& options = {});

CODEM_API double heston_price_cos_exercise(OptionType type,
                                           ExerciseStyle style,
                                           double spot,
                                           double strike,
                                           double rate,
                                           double dividend_yield,
                                           double maturity,
                                           const volatility::HestonParams& params,
                                           std::span<const double> dates = {},
                                           const CosOptions& options = {});

CODEM_API double bates_price_cos_exercise(OptionType type,
                                          ExerciseStyle style,
                                          double spot,
                                          double strike,
                                          double rate,
                                          double dividend_yield,
                                          double maturity,
                                          const volatility::BatesParams& params,
                                          std::span<const double> dates = {},
                                          const CosOptions& options = {});

} // namespace detail

} // namespace pluss::subsp::math::finance::pricing
