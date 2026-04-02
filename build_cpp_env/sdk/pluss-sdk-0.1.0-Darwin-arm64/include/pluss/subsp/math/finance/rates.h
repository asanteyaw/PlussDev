#pragma once

#include "pluss/codem/codem.h"
#include "pluss/config.h"
#include "pluss/export.h"
#include "pluss/subsp/math/finance/volatility.h"

#include <cstdint>
#include <vector>

namespace pluss::subsp::math::finance::rates {

enum class Scheme : std::uint8_t {
  Euler = 0,
  Milstein = 1,
  Transition = 2,
};

struct SimOptions {
  Scheme scheme = Scheme::Euler;
  volatility::RandMethod method = volatility::RandMethod::Pseudo;
  bool randomized_quasi = false;
  int sobol_scrambling = 0;
  bool paths_first = false;
  std::uint64_t seed = 0;
  std::uint64_t offset = 0;
};

struct VasicekParams {
  double kappa = 0.7;
  double theta = 0.03;
  double sigma = 0.01;
  double r0 = 0.03;
};

struct CirParams {
  double kappa = 0.7;
  double theta = 0.03;
  double sigma = 0.12;
  double r0 = 0.03;
  double rate_floor = 1e-12;
};

struct HullWhiteParams {
  double kappa = 0.7;
  double sigma = 0.01;
  double r0 = 0.03;
  double theta0 = 0.03;
  double theta_slope = 0.0;
};

struct HjmParams {
  double r0 = 0.03;
  std::vector<double> tenors = {0.25, 0.5, 1.0, 2.0, 3.0, 5.0};
  std::vector<double> vol = {0.012, 0.011, 0.010, 0.009, 0.008, 0.007};
};

struct RatePaths {
  pluss::codem::Series rate = pluss::codem::Series::undefined();
  pluss::codem::Series innovation = pluss::codem::Series::undefined();
};

struct HjmPaths {
  pluss::codem::Series rate = pluss::codem::Series::undefined();
  pluss::codem::Series forward = pluss::codem::Series::undefined();      // [steps+1, paths, tenors]
  pluss::codem::Series innovation = pluss::codem::Series::undefined();   // [steps, paths]
};

CODEM_API RatePaths vasicek_paths(std::int64_t steps,
                                  std::int64_t paths,
                                  double dt,
                                  const VasicekParams& params,
                                  const SimOptions& options,
                                  pluss::codem::Config cfg = {},
                                  std::uint64_t seed = 0);

CODEM_API RatePaths vasicek_paths(std::int64_t steps,
                                  std::int64_t paths,
                                  double dt,
                                  const VasicekParams& params,
                                  pluss::codem::Config cfg = {},
                                  std::uint64_t seed = 0);

CODEM_API RatePaths cir_paths(std::int64_t steps,
                              std::int64_t paths,
                              double dt,
                              const CirParams& params,
                              const SimOptions& options,
                              pluss::codem::Config cfg = {},
                              std::uint64_t seed = 0);

CODEM_API RatePaths cir_paths(std::int64_t steps,
                              std::int64_t paths,
                              double dt,
                              const CirParams& params,
                              pluss::codem::Config cfg = {},
                              std::uint64_t seed = 0);

CODEM_API RatePaths hull_white_paths(std::int64_t steps,
                                     std::int64_t paths,
                                     double dt,
                                     const HullWhiteParams& params,
                                     const SimOptions& options,
                                     pluss::codem::Config cfg = {},
                                     std::uint64_t seed = 0);

CODEM_API RatePaths hull_white_paths(std::int64_t steps,
                                     std::int64_t paths,
                                     double dt,
                                     const HullWhiteParams& params,
                                     pluss::codem::Config cfg = {},
                                     std::uint64_t seed = 0);

CODEM_API HjmPaths hjm_paths(std::int64_t steps,
                             std::int64_t paths,
                             double dt,
                             const HjmParams& params,
                             const SimOptions& options,
                             pluss::codem::Config cfg = {},
                             std::uint64_t seed = 0);

CODEM_API HjmPaths hjm_paths(std::int64_t steps,
                             std::int64_t paths,
                             double dt,
                             const HjmParams& params,
                             pluss::codem::Config cfg = {},
                             std::uint64_t seed = 0);

CODEM_API double vasicek_zcb(double maturity, double short_rate, const VasicekParams& params);
CODEM_API double cir_zcb(double maturity, double short_rate, const CirParams& params);
CODEM_API double hull_white_zcb(double maturity, double short_rate, const HullWhiteParams& params);
CODEM_API double hjm_zcb_mc(double maturity,
                            const HjmParams& params,
                            std::int64_t paths = 20000,
                            std::int64_t steps = 252,
                            SimOptions options = {},
                            pluss::codem::Config cfg = {});

} // namespace pluss::subsp::math::finance::rates
