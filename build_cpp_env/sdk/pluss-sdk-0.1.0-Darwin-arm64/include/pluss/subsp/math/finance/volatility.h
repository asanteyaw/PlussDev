#pragma once

#include "pluss/codem/codem.h"
#include "pluss/config.h"
#include "pluss/export.h"

#include <cstdint>
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

} // namespace pluss::subsp::math::finance::volatility
