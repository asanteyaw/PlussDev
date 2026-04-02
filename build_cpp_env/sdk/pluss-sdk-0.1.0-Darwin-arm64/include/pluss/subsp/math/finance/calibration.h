#pragma once

#include "pluss/subsp/math/finance/pricing.h"
#include "pluss/subsp/math/finance/rates.h"
#include "pluss/subsp/math/finance/volatility.h"

#include <cstdint>
#include <limits>
#include <vector>

namespace pluss::subsp::math::finance::calibration {

struct OptionQuote {
  pricing::OptionType type = pricing::OptionType::Call;
  double strike = 100.0;
  double maturity = 1.0;
  double market_price = 0.0;
  double weight = 1.0;
};

enum class HestonCalibMethod : std::uint8_t {
  CharFn = 0,
  MonteCarlo = 1,
};

struct HestonBounds {
  double kappa_min = 1e-4;
  double kappa_max = 12.0;
  double theta_min = 1e-6;
  double theta_max = 4.0;
  double sigma_min = 1e-4;
  double sigma_max = 5.0;
  double rho_min = -0.999;
  double rho_max = 0.999;
  double v0_min = 1e-6;
  double v0_max = 4.0;
};

struct HestonCalibOptions {
  HestonCalibMethod method = HestonCalibMethod::CharFn;
  HestonBounds bounds{};
  std::int64_t random_starts = 12;
  std::int64_t max_iterations = 160;
  double step_fraction = 0.2;
  double step_decay = 0.65;
  double tolerance = 1e-7;
  std::uint64_t seed = 0;
  pricing::HestonIntegrationOptions integration{};
  pricing::MonteCarloOptions mc{};
};

struct HestonCalibResult {
  volatility::HestonParams params{};
  double objective = std::numeric_limits<double>::infinity();
  double rmse = std::numeric_limits<double>::infinity();
  std::int64_t evaluations = 0;
  bool converged = false;
};

struct BatesBounds {
  double kappa_min = 1e-4;
  double kappa_max = 12.0;
  double theta_min = 1e-6;
  double theta_max = 4.0;
  double sigma_min = 1e-4;
  double sigma_max = 5.0;
  double rho_min = -0.999;
  double rho_max = 0.999;
  double v0_min = 1e-6;
  double v0_max = 4.0;
  double jump_intensity_min = 0.0;
  double jump_intensity_max = 5.0;
  double jump_mean_min = -2.0;
  double jump_mean_max = 2.0;
  double jump_vol_min = 1e-6;
  double jump_vol_max = 3.0;
};

struct BatesCalibOptions {
  BatesBounds bounds{};
  std::int64_t random_starts = 12;
  std::int64_t max_iterations = 160;
  double step_fraction = 0.2;
  double step_decay = 0.65;
  double tolerance = 1e-7;
  std::uint64_t seed = 0;
  pricing::MonteCarloOptions mc{};
};

struct BatesCalibResult {
  volatility::BatesParams params{};
  double objective = std::numeric_limits<double>::infinity();
  double rmse = std::numeric_limits<double>::infinity();
  std::int64_t evaluations = 0;
  bool converged = false;
};

struct SvBounds {
  double logvar_mean_min = -12.0;
  double logvar_mean_max = 2.0;
  double phi_min = -0.999;
  double phi_max = 0.999;
  double vol_of_logvar_min = 1e-4;
  double vol_of_logvar_max = 3.0;
  double rho_min = -0.999;
  double rho_max = 0.999;
  double logvar0_min = -12.0;
  double logvar0_max = 2.0;
};

struct SvCalibOptions {
  SvBounds bounds{};
  std::int64_t random_starts = 10;
  std::int64_t max_iterations = 120;
  double step_fraction = 0.2;
  double step_decay = 0.65;
  double tolerance = 1e-7;
  std::uint64_t seed = 0;
  pricing::MonteCarloOptions mc{};
};

struct SvCalibResult {
  volatility::SvParams params{};
  double objective = std::numeric_limits<double>::infinity();
  double rmse = std::numeric_limits<double>::infinity();
  std::int64_t evaluations = 0;
  bool converged = false;
};

struct HestonNandiBounds {
  double omega_min = 1e-12;
  double omega_max = 2.0;
  double alpha_min = 0.0;
  double alpha_max = 4.0;
  double beta_min = 0.0;
  double beta_max = 0.999;
  double gamma_min = -10.0;
  double gamma_max = 10.0;
  double lambda_min = -10.0;
  double lambda_max = 10.0;
};

struct HestonNandiCalibOptions {
  HestonNandiBounds bounds{};
  std::int64_t random_starts = 10;
  std::int64_t max_iterations = 140;
  double step_fraction = 0.2;
  double step_decay = 0.65;
  double tolerance = 1e-7;
  double returns_weight = 1.0;
  double options_weight = 1.0;
  bool use_closed_form = true;
  pricing::HestonNandiIntegrationOptions integration{};
  pricing::MonteCarloOptions mc{};
  std::uint64_t seed = 0;
};

struct HestonNandiCalibResult {
  volatility::HestonNandiParams params{};
  double objective = std::numeric_limits<double>::infinity();
  double returns_nll = std::numeric_limits<double>::infinity();
  double options_sse = std::numeric_limits<double>::infinity();
  double options_rmse = std::numeric_limits<double>::infinity();
  std::int64_t evaluations = 0;
  bool converged = false;
};

struct RateQuote {
  double maturity = 1.0;
  double zcb_price = 1.0;
  double weight = 1.0;
};

struct VasicekBounds {
  double kappa_min = 1e-6;
  double kappa_max = 6.0;
  double theta_min = -0.2;
  double theta_max = 0.5;
  double sigma_min = 1e-6;
  double sigma_max = 1.0;
  double r0_min = -0.2;
  double r0_max = 0.5;
};

struct VasicekCalibOptions {
  VasicekBounds bounds{};
  std::int64_t random_starts = 10;
  std::int64_t max_iterations = 120;
  double step_fraction = 0.2;
  double step_decay = 0.65;
  double tolerance = 1e-8;
  std::uint64_t seed = 0;
};

struct VasicekCalibResult {
  rates::VasicekParams params{};
  double objective = std::numeric_limits<double>::infinity();
  double rmse = std::numeric_limits<double>::infinity();
  std::int64_t evaluations = 0;
  bool converged = false;
};

struct CirBounds {
  double kappa_min = 1e-6;
  double kappa_max = 6.0;
  double theta_min = 1e-6;
  double theta_max = 0.5;
  double sigma_min = 1e-6;
  double sigma_max = 3.0;
  double r0_min = 1e-8;
  double r0_max = 0.5;
};

struct CirCalibOptions {
  CirBounds bounds{};
  std::int64_t random_starts = 10;
  std::int64_t max_iterations = 120;
  double step_fraction = 0.2;
  double step_decay = 0.65;
  double tolerance = 1e-8;
  std::uint64_t seed = 0;
};

struct CirCalibResult {
  rates::CirParams params{};
  double objective = std::numeric_limits<double>::infinity();
  double rmse = std::numeric_limits<double>::infinity();
  std::int64_t evaluations = 0;
  bool converged = false;
};

struct HullWhiteBounds {
  double kappa_min = 1e-6;
  double kappa_max = 6.0;
  double sigma_min = 1e-6;
  double sigma_max = 1.0;
  double r0_min = -0.2;
  double r0_max = 0.5;
  double theta0_min = -0.2;
  double theta0_max = 0.5;
  double theta_slope_min = -0.5;
  double theta_slope_max = 0.5;
};

struct HullWhiteCalibOptions {
  HullWhiteBounds bounds{};
  std::int64_t random_starts = 10;
  std::int64_t max_iterations = 120;
  double step_fraction = 0.2;
  double step_decay = 0.65;
  double tolerance = 1e-8;
  std::uint64_t seed = 0;
};

struct HullWhiteCalibResult {
  rates::HullWhiteParams params{};
  double objective = std::numeric_limits<double>::infinity();
  double rmse = std::numeric_limits<double>::infinity();
  std::int64_t evaluations = 0;
  bool converged = false;
};

struct HjmBounds {
  double r0_min = -0.1;
  double r0_max = 0.5;
  double vol_scale_min = 1e-6;
  double vol_scale_max = 10.0;
};

struct HjmCalibOptions {
  HjmBounds bounds{};
  std::int64_t random_starts = 8;
  std::int64_t max_iterations = 80;
  double step_fraction = 0.2;
  double step_decay = 0.65;
  double tolerance = 1e-8;
  std::int64_t mc_paths = 2500;
  std::int64_t mc_steps = 64;
  rates::SimOptions sim_options{};
  std::uint64_t seed = 0;
};

struct HjmCalibResult {
  rates::HjmParams params{};
  double objective = std::numeric_limits<double>::infinity();
  double rmse = std::numeric_limits<double>::infinity();
  std::int64_t evaluations = 0;
  bool converged = false;
};

CODEM_API double heston_sse(const std::vector<OptionQuote>& quotes,
                            double spot,
                            double rate,
                            double dividend_yield,
                            const volatility::HestonParams& params,
                            const HestonCalibOptions& options = {},
                            pluss::codem::Config cfg = {});

CODEM_API HestonCalibResult calibrate_heston(const std::vector<OptionQuote>& quotes,
                                             double spot,
                                             double rate,
                                             double dividend_yield,
                                             const volatility::HestonParams& initial,
                                             const HestonCalibOptions& options = {},
                                             pluss::codem::Config cfg = {});

CODEM_API double bates_sse(const std::vector<OptionQuote>& quotes,
                           double spot,
                           double rate,
                           double dividend_yield,
                           const volatility::BatesParams& params,
                           const BatesCalibOptions& options = {},
                           pluss::codem::Config cfg = {});

CODEM_API BatesCalibResult calibrate_bates(const std::vector<OptionQuote>& quotes,
                                           double spot,
                                           double rate,
                                           double dividend_yield,
                                           const volatility::BatesParams& initial,
                                           const BatesCalibOptions& options = {},
                                           pluss::codem::Config cfg = {});

CODEM_API double sv_sse(const std::vector<OptionQuote>& quotes,
                        double spot,
                        double rate,
                        double dividend_yield,
                        const volatility::SvParams& params,
                        const SvCalibOptions& options = {},
                        pluss::codem::Config cfg = {});

CODEM_API SvCalibResult calibrate_sv(const std::vector<OptionQuote>& quotes,
                                     double spot,
                                     double rate,
                                     double dividend_yield,
                                     const volatility::SvParams& initial,
                                     const SvCalibOptions& options = {},
                                     pluss::codem::Config cfg = {});

CODEM_API double heston_nandi_returns_nll(const pluss::codem::Series& returns,
                                          const volatility::HestonNandiParams& params,
                                          pluss::codem::Config cfg = {});

CODEM_API double heston_nandi_sse(const std::vector<OptionQuote>& quotes,
                                  double spot,
                                  double rate,
                                  double dividend_yield,
                                  const volatility::HestonNandiParams& params,
                                  const HestonNandiCalibOptions& options = {},
                                  pluss::codem::Config cfg = {});

CODEM_API HestonNandiCalibResult calibrate_heston_nandi(const pluss::codem::Series& returns,
                                                        const std::vector<OptionQuote>& quotes,
                                                        double spot,
                                                        double rate,
                                                        double dividend_yield,
                                                        const volatility::HestonNandiParams& initial,
                                                        const HestonNandiCalibOptions& options = {},
                                                        pluss::codem::Config cfg = {});

CODEM_API double vasicek_sse(const std::vector<RateQuote>& quotes,
                             const rates::VasicekParams& params,
                             const VasicekCalibOptions& options = {});

CODEM_API VasicekCalibResult calibrate_vasicek(const std::vector<RateQuote>& quotes,
                                               const rates::VasicekParams& initial,
                                               const VasicekCalibOptions& options = {});

CODEM_API double cir_sse(const std::vector<RateQuote>& quotes,
                         const rates::CirParams& params,
                         const CirCalibOptions& options = {});

CODEM_API CirCalibResult calibrate_cir(const std::vector<RateQuote>& quotes,
                                       const rates::CirParams& initial,
                                       const CirCalibOptions& options = {});

CODEM_API double hull_white_sse(const std::vector<RateQuote>& quotes,
                                const rates::HullWhiteParams& params,
                                const HullWhiteCalibOptions& options = {});

CODEM_API HullWhiteCalibResult calibrate_hull_white(const std::vector<RateQuote>& quotes,
                                                    const rates::HullWhiteParams& initial,
                                                    const HullWhiteCalibOptions& options = {});

CODEM_API double hjm_sse(const std::vector<RateQuote>& quotes,
                         const rates::HjmParams& params,
                         const HjmCalibOptions& options = {});

CODEM_API HjmCalibResult calibrate_hjm(const std::vector<RateQuote>& quotes,
                                       const rates::HjmParams& initial,
                                       const HjmCalibOptions& options = {});

} // namespace pluss::subsp::math::finance::calibration
