#pragma once

#include "pluss/codem/codem.h"
#include "pluss/config.h"
#include "pluss/export.h"

#include <cstdint>
#include <vector>
#include <limits>

namespace pluss::subsp::math::finance::volatility {

enum class SimScheme : std::uint8_t {
  Euler = 0,
  Milstein = 1,
  Milstein2 = 2,
  QuadExp = 3,
  Transition = 4,
};

enum class RandMethod : std::uint8_t {
  Pseudo = 0,
  Sobol = 1,
  Halton = 2,
  Torus = 3,
};

struct SimOptions {
  SimScheme scheme = SimScheme::Euler;
  RandMethod method = RandMethod::Pseudo;
  bool randomized_quasi = false;
  int sobol_scrambling = 0;
  bool antithetic = false;
  double normal_shift = 0.0;
  bool paths_first = false;
  std::uint64_t seed = 0;
  std::uint64_t offset = 0;
  // When set, the initial variance (forward-variance level for rough Bergomi) of the path generators is read from this tracked scalar, so the paths are
  // differentiable in it (pathwise sensitivity); the numeric value in the parameters is ignored.
  const pluss::codem::Series* tracked_level = nullptr;
  // SABR only: a tracked initial forward of shape [paths] (one copy per path, so the sensitivity of the terminal forwards is the pathwise Jacobian).
  const pluss::codem::Series* tracked_spot = nullptr;
  // SABR only: the drift of the forward (rate - dividend yield) as a tracked scalar, replacing params.mu, for the pathwise rho of an early-exercise option.
  const pluss::codem::Series* tracked_drift = nullptr;
};

struct GbmParams {
  double mu = 0.0;
  double sigma = 0.2;
};

struct HestonParams {
  double kappa = 1.5;
  double theta = 0.04;
  double sigma = 0.3;
  double rho = -0.7;
  double v0 = 0.04;
  double mu = 0.0;
  double variance_floor = 1e-12;
};

struct BatesParams {
  double kappa = 1.5;
  double theta = 0.04;
  double sigma = 0.3;
  double rho = -0.7;
  double v0 = 0.04;
  double mu = 0.0;
  double variance_floor = 1e-12;
  double jump_intensity = 0.15;
  double jump_mean = -0.08;
  double jump_vol = 0.20;
};

struct SvParams {
  double mu = 0.0;
  double logvar_mean = -3.0;
  double phi = 0.98;
  double vol_of_logvar = 0.25;
  double rho = -0.5;
  double logvar0 = -3.0;
  double variance_floor = 1e-12;
};

struct HestonNandiParams {
  double omega = 1e-6;
  double alpha = 0.08;
  double beta = 0.90;
  double gamma = 0.0;
  double lambda = 0.0;
  double r = 0.0;
  double initial_variance = std::numeric_limits<double>::quiet_NaN();
  double variance_floor = 1e-12;
  double stationarity_margin = 1e-6;
};

struct SabrParams {
  double alpha0 = 0.2;
  double beta = 1.0;
  double nu = 0.4;
  double rho = -0.3;
  double mu = 0.0;
  double alpha_floor = 1e-12;
};

enum class LocalVolForm : std::uint8_t {
  Constant = 0,
  Cev = 1,
  LogAffine = 2,
};

struct SlvParams {
  double mu = 0.0;
  double kappa = 1.5;
  double theta = 0.04;
  double sigma = 0.25;
  double rho = -0.6;
  double v0 = 0.04;
  double variance_floor = 1e-12;
  LocalVolForm local_vol_form = LocalVolForm::Constant;
  double local_vol_level = 1.0;
  double local_vol_slope = 0.0;
  double local_vol_curvature = 0.0;
};

struct RoughHestonParams {
  double mu = 0.0;
  double v0 = 0.04;
  double theta = 0.04;
  double kappa = 1.5;
  double sigma = 0.3;
  double alpha = -0.2;
  double rho = 0.0;
  double variance_floor = 1e-12;
};

struct RoughBergomiParams {
  double mu = 0.0;
  double xi = 0.04;
  double eta = 1.5;
  double alpha = -0.4;
  double rho = 0.0;
  double variance_floor = 1e-12;
};

struct StochVolPaths {
  pluss::codem::Series spot = pluss::series::undefined();
  pluss::codem::Series variance = pluss::series::undefined();
  pluss::codem::Series innovation_spot = pluss::series::undefined();
  pluss::codem::Series innovation_var = pluss::series::undefined();
};



















// Path generators behind module::Field::simulate_paths and the Monte Carlo engine. Not a public interface.
namespace detail {

// Several assets (Heston, Bates, Black-Scholes, SABR) simulated in one pass. Every parameter is a Series with one entry per asset; the assets are the leading state axis, so
// one asset is the same code with an axis of length one. Heston is Bates with a zero jump intensity. The price shocks of different assets are
// correlated by `price_correlation`, the variance shocks by `variance_correlation` (each an [A, A] correlation matrix; undefined means independent), and
// within an asset the price and variance shocks have correlation `rho`. Outputs are [paths, steps + 1, A] (shocks [paths, steps, A]).
enum class MultiKind : std::uint8_t { Sv = 0, BlackScholes = 1, Sabr = 2, RoughHeston = 3, RoughBergomi = 4 };   // Sv is Heston or Bates
struct MultiSvParams {
  // Optional: the kind of each asset (empty means every asset is Heston or Bates). A Black-Scholes asset (volatility `bs_vol`) has a price shock and no variance
  // shock of its own; a SABR asset (alpha0, beta, nu) has a forward in place of the spot and a volatility factor alpha in place of the variance; a rough Heston (kappa, theta, sigma, rough_alpha, v0) or rough Bergomi (xi, eta, rough_alpha) asset has a variance that depends on the whole past of its shock. The columns
  // below are per asset for every kind: the entries kappa, theta, sigma, v0 and the jump columns are read only for the assets that use them (put any valid
  // filler elsewhere), and `rho` is the correlation of each asset's price shock with its own second factor (0 for Black-Scholes).
  std::vector<MultiKind> kind;
  pluss::codem::Series bs_vol, alpha0, beta, nu;                                              // Black-Scholes and SABR columns
  pluss::codem::Series rough_alpha, xi, eta;                                                  // rough models: alpha = H - 1/2 (rough Heston reuses kappa, theta, sigma, v0)
  double alpha_floor = 1e-12;
  pluss::codem::Series spot0, growth, kappa, theta, sigma, rho, v0;                          // growth = expected growth rate of the spot, r - q
  pluss::codem::Series jump_intensity, jump_mean, jump_vol;                                    // optional: undefined means no jumps
  pluss::codem::Series price_correlation, variance_correlation;                               // optional [A, A]
  pluss::codem::Series price_variance_correlation;                                            // optional [A, A]: corr(price shock a, variance shock b); the diagonal is rho
  double variance_floor = 1e-12;
};
struct MultiSvPaths {
  pluss::codem::Series spot, variance, innovation_spot, innovation_var;
};
CODEM_API MultiSvPaths multi_sv_paths(std::int64_t steps,
                                      std::int64_t paths,
                                      double dt,
                                      const MultiSvParams& params,
                                      const SimOptions& options = {},
                                      pluss::codem::Config cfg = {},
                                      std::uint64_t seed = 0);

CODEM_API StochVolPaths gbm_paths(std::int64_t steps,
                                  std::int64_t paths,
                                  double spot0,
                                  double dt,
                                  const GbmParams& params,
                                  const SimOptions& options,
                                  pluss::codem::Config cfg = {},
                                  std::uint64_t seed = 0);

CODEM_API StochVolPaths gbm_paths(std::int64_t steps,
                                  std::int64_t paths,
                                  double spot0,
                                  double dt,
                                  const GbmParams& params,
                                  pluss::codem::Config cfg = {},
                                  std::uint64_t seed = 0);

CODEM_API StochVolPaths heston_paths(std::int64_t steps,
                                     std::int64_t paths,
                                     double spot0,
                                     double dt,
                                     const HestonParams& params,
                                     const SimOptions& options,
                                     pluss::codem::Config cfg = {},
                                     std::uint64_t seed = 0);

CODEM_API StochVolPaths heston_paths(std::int64_t steps,
                                     std::int64_t paths,
                                     double spot0,
                                     double dt,
                                     const HestonParams& params,
                                     pluss::codem::Config cfg = {},
                                     std::uint64_t seed = 0);

CODEM_API StochVolPaths bates_paths(std::int64_t steps,
                                    std::int64_t paths,
                                    double spot0,
                                    double dt,
                                    const BatesParams& params,
                                    const SimOptions& options,
                                    pluss::codem::Config cfg = {},
                                    std::uint64_t seed = 0);

CODEM_API StochVolPaths bates_paths(std::int64_t steps,
                                    std::int64_t paths,
                                    double spot0,
                                    double dt,
                                    const BatesParams& params,
                                    pluss::codem::Config cfg = {},
                                    std::uint64_t seed = 0);

CODEM_API StochVolPaths sv_paths(std::int64_t steps,
                                 std::int64_t paths,
                                 double spot0,
                                 double dt,
                                 const SvParams& params,
                                 const SimOptions& options,
                                 pluss::codem::Config cfg = {},
                                 std::uint64_t seed = 0);

CODEM_API StochVolPaths sv_paths(std::int64_t steps,
                                 std::int64_t paths,
                                 double spot0,
                                 double dt,
                                 const SvParams& params,
                                 pluss::codem::Config cfg = {},
                                 std::uint64_t seed = 0);

CODEM_API StochVolPaths heston_nandi_paths(std::int64_t steps,
                                           std::int64_t paths,
                                           double spot0,
                                           double dt,
                                           const HestonNandiParams& params,
                                           const SimOptions& options,
                                           pluss::codem::Config cfg = {},
                                           std::uint64_t seed = 0);

CODEM_API StochVolPaths heston_nandi_paths(std::int64_t steps,
                                           std::int64_t paths,
                                           double spot0,
                                           double dt,
                                           const HestonNandiParams& params,
                                           pluss::codem::Config cfg = {},
                                           std::uint64_t seed = 0);

CODEM_API StochVolPaths sabr_paths(std::int64_t steps,
                                   std::int64_t paths,
                                   double forward0,
                                   double dt,
                                   const SabrParams& params,
                                   const SimOptions& options,
                                   pluss::codem::Config cfg = {},
                                   std::uint64_t seed = 0);

CODEM_API StochVolPaths sabr_paths(std::int64_t steps,
                                   std::int64_t paths,
                                   double forward0,
                                   double dt,
                                   const SabrParams& params,
                                   pluss::codem::Config cfg = {},
                                   std::uint64_t seed = 0);

CODEM_API StochVolPaths slv_paths(std::int64_t steps,
                                  std::int64_t paths,
                                  double spot0,
                                  double dt,
                                  const SlvParams& params,
                                  const SimOptions& options,
                                  pluss::codem::Config cfg = {},
                                  std::uint64_t seed = 0);

CODEM_API StochVolPaths slv_paths(std::int64_t steps,
                                  std::int64_t paths,
                                  double spot0,
                                  double dt,
                                  const SlvParams& params,
                                  pluss::codem::Config cfg = {},
                                  std::uint64_t seed = 0);

CODEM_API StochVolPaths rough_heston_paths(std::int64_t steps,
                                           std::int64_t paths,
                                           double spot0,
                                           double dt,
                                           const RoughHestonParams& params,
                                           const SimOptions& options,
                                           pluss::codem::Config cfg = {},
                                           std::uint64_t seed = 0);

CODEM_API StochVolPaths rough_heston_paths(std::int64_t steps,
                                           std::int64_t paths,
                                           double spot0,
                                           double dt,
                                           const RoughHestonParams& params,
                                           pluss::codem::Config cfg = {},
                                           std::uint64_t seed = 0);

CODEM_API StochVolPaths rough_bergomi_paths(std::int64_t steps,
                                            std::int64_t paths,
                                            double spot0,
                                            double dt,
                                            const RoughBergomiParams& params,
                                            const SimOptions& options,
                                            pluss::codem::Config cfg = {},
                                            std::uint64_t seed = 0);

CODEM_API StochVolPaths rough_bergomi_paths(std::int64_t steps,
                                            std::int64_t paths,
                                            double spot0,
                                            double dt,
                                            const RoughBergomiParams& params,
                                            pluss::codem::Config cfg = {},
                                            std::uint64_t seed = 0);

} // namespace detail

} // namespace pluss::subsp::math::finance::volatility
