#include "pluss/subsp/math/finance/models.h"
#include "pluss/subsp/math/finance/objects.h"
#include "pluss/codem/series_factory.h"

#include <utility>

namespace pluss::subsp::math::finance::models {
namespace {

using pluss::codem::Config;
using pluss::codem::ScalarType;
using pluss::codem::Series;

Config f64_cfg_(Config cfg, Platform p) {
  return Config{}.with_dtype(ScalarType::Float64).with_platform(p).with_pinned(cfg.pinned);
}

Series scalar_series_(double v, Config cfg) {
  return pluss::full({1}, v, cfg);
}

object::model::Heston to_series_model_(const volatility::HestonParams& p, Config cfg) {
  return object::model::Heston{
      .kappa = scalar_series_(p.kappa, cfg),
      .theta = scalar_series_(p.theta, cfg),
      .sigma = scalar_series_(p.sigma, cfg),
      .rho = scalar_series_(p.rho, cfg),
      .v0 = scalar_series_(p.v0, cfg),
      .mu = scalar_series_(p.mu, cfg),
      .variance_floor = scalar_series_(p.variance_floor, cfg),
  };
}

object::model::Bates to_series_model_(const volatility::BatesParams& p, Config cfg) {
  return object::model::Bates{
      .kappa = scalar_series_(p.kappa, cfg),
      .theta = scalar_series_(p.theta, cfg),
      .sigma = scalar_series_(p.sigma, cfg),
      .rho = scalar_series_(p.rho, cfg),
      .v0 = scalar_series_(p.v0, cfg),
      .mu = scalar_series_(p.mu, cfg),
      .variance_floor = scalar_series_(p.variance_floor, cfg),
      .jump_intensity = scalar_series_(p.jump_intensity, cfg),
      .jump_mean = scalar_series_(p.jump_mean, cfg),
      .jump_vol = scalar_series_(p.jump_vol, cfg),
  };
}

} // namespace

Heston::Heston(volatility::HestonParams params) : params_(params) {}

const volatility::HestonParams& Heston::params() const noexcept {
  return params_;
}

void Heston::set_params(const volatility::HestonParams& params) {
  params_ = params;
}

volatility::StochVolPaths Heston::paths(std::int64_t steps,
                                        std::int64_t num_paths,
                                        double spot0,
                                        double dt,
                                        const volatility::SimOptions& options,
                                        pluss::codem::Config cfg,
                                        std::uint64_t seed) const {
  return volatility::heston_paths(steps, num_paths, spot0, dt, params_, options, cfg, seed);
}

double Heston::price(pricing::OptionType type,
                     double spot,
                     double strike,
                     double rate,
                     double dividend_yield,
                     double maturity,
                     const pricing::HestonIntegrationOptions& options) const {
  return pricing::heston_price(type, spot, strike, rate, dividend_yield, maturity, params_, options);
}

Series Heston::price(pricing::OptionType type,
                     const Series& spot,
                     const Series& strike,
                     const Series& rate,
                     const Series& dividend_yield,
                     const Series& maturity,
                     const pricing::HestonFftOptions& options,
                     pluss::codem::Config cfg) const {
  const auto out_cfg = f64_cfg_(cfg, spot.platform());
  const instrument::Vanilla inst{
      .type = type,
      .strike = strike,
      .maturity = maturity,
  };
  const market::State mkt{
      .spot = spot,
      .rate = rate,
      .dividend_yield = dividend_yield,
  };
  const auto mdl = to_series_model_(params_, out_cfg);
  const pricer::Fft fft{
      .vol_risk_premium = options.vol_risk_premium,
      .mode = options.mode,
      .num_fft = options.num_fft,
      .du = options.characteristic_fcn_step,
      .dk = options.log_strike_step,
      .alpha = options.damping_factor,
      .quadrature = options.quadrature,
      .return_grid = false,
  };
  return fft.price(inst, mdl, mkt, pricer::RiskSpec::PriceOnly, out_cfg).price;
}

double Heston::price_mc(pricing::OptionType type,
                        double spot,
                        double strike,
                        double rate,
                        double dividend_yield,
                        double maturity,
                        const pricing::MonteCarloOptions& options,
                        pluss::codem::Config cfg) const {
  return pricing::heston_price_mc(type, spot, strike, rate, dividend_yield, maturity, params_, options, cfg);
}

Series Heston::price_mc(pricing::OptionType type,
                        const Series& spot,
                        const Series& strike,
                        const Series& rate,
                        const Series& dividend_yield,
                        const Series& maturity,
                        const pricing::MonteCarloOptions& options,
                        pluss::codem::Config cfg) const {
  const auto out_cfg = f64_cfg_(cfg, spot.platform());
  const instrument::Vanilla inst{
      .type = type,
      .strike = strike,
      .maturity = maturity,
  };
  const market::State mkt{
      .spot = spot,
      .rate = rate,
      .dividend_yield = dividend_yield,
  };
  const auto mdl = to_series_model_(params_, out_cfg);
  const pricer::MonteCarlo mc{
      .paths = options.paths,
      .steps = options.steps,
      .scheme = options.scheme,
      .rand_method = options.rand_method,
      .randomized_quasi = options.randomized_quasi,
      .sobol_scrambling = options.sobol_scrambling,
      .antithetic = options.antithetic,
      .control_variate = options.control_variate,
      .importance_sampling = options.importance_sampling,
      .importance_shift = options.importance_shift,
      .control_vol = options.control_vol,
      .offset = options.offset,
      .seed = options.seed,
  };
  return mc.price(inst, mdl, mkt, pricer::RiskSpec::PriceOnly, out_cfg).price;
}

double Heston::sse(const std::vector<calibration::OptionQuote>& quotes,
                   double spot,
                   double rate,
                   double dividend_yield,
                   const calibration::HestonCalibOptions& options,
                   pluss::codem::Config cfg) const {
  return calibration::heston_sse(quotes, spot, rate, dividend_yield, params_, options, cfg);
}

calibration::HestonCalibResult Heston::calibrate(const std::vector<calibration::OptionQuote>& quotes,
                                                 double spot,
                                                 double rate,
                                                 double dividend_yield,
                                                 const calibration::HestonCalibOptions& options,
                                                 pluss::codem::Config cfg) const {
  return calibration::calibrate_heston(quotes, spot, rate, dividend_yield, params_, options, cfg);
}

calibration::HestonCalibResult Heston::calibrate_inplace(const std::vector<calibration::OptionQuote>& quotes,
                                                         double spot,
                                                         double rate,
                                                         double dividend_yield,
                                                         const calibration::HestonCalibOptions& options,
                                                         pluss::codem::Config cfg) {
  auto out = calibration::calibrate_heston(quotes, spot, rate, dividend_yield, params_, options, cfg);
  params_ = out.params;
  return out;
}

Bates::Bates(volatility::BatesParams params) : params_(params) {}

const volatility::BatesParams& Bates::params() const noexcept {
  return params_;
}

void Bates::set_params(const volatility::BatesParams& params) {
  params_ = params;
}

volatility::StochVolPaths Bates::paths(std::int64_t steps,
                                       std::int64_t num_paths,
                                       double spot0,
                                       double dt,
                                       const volatility::SimOptions& options,
                                       pluss::codem::Config cfg,
                                       std::uint64_t seed) const {
  return volatility::bates_paths(steps, num_paths, spot0, dt, params_, options, cfg, seed);
}

double Bates::price_mc(pricing::OptionType type,
                       double spot,
                       double strike,
                       double rate,
                       double dividend_yield,
                       double maturity,
                       const pricing::MonteCarloOptions& options,
                       pluss::codem::Config cfg) const {
  return pricing::bates_price_mc(type, spot, strike, rate, dividend_yield, maturity, params_, options, cfg);
}

double Bates::price_fft(pricing::OptionType type,
                        double spot,
                        double strike,
                        double rate,
                        double dividend_yield,
                        double maturity,
                        const pricing::HestonFftOptions& options,
                        pluss::codem::Config cfg) const {
  return pricing::bates_price_fft(type, spot, strike, rate, dividend_yield, maturity, params_, options, cfg);
}

Series Bates::price_mc(pricing::OptionType type,
                       const Series& spot,
                       const Series& strike,
                       const Series& rate,
                       const Series& dividend_yield,
                       const Series& maturity,
                       const pricing::MonteCarloOptions& options,
                       pluss::codem::Config cfg) const {
  const auto out_cfg = f64_cfg_(cfg, spot.platform());
  const instrument::Vanilla inst{
      .type = type,
      .strike = strike,
      .maturity = maturity,
  };
  const market::State mkt{
      .spot = spot,
      .rate = rate,
      .dividend_yield = dividend_yield,
  };
  const auto mdl = to_series_model_(params_, out_cfg);
  const pricer::MonteCarlo mc{
      .paths = options.paths,
      .steps = options.steps,
      .scheme = options.scheme,
      .rand_method = options.rand_method,
      .randomized_quasi = options.randomized_quasi,
      .sobol_scrambling = options.sobol_scrambling,
      .antithetic = options.antithetic,
      .control_variate = options.control_variate,
      .importance_sampling = options.importance_sampling,
      .importance_shift = options.importance_shift,
      .control_vol = options.control_vol,
      .offset = options.offset,
      .seed = options.seed,
  };
  return mc.price(inst, mdl, mkt, pricer::RiskSpec::PriceOnly, out_cfg).price;
}

Series Bates::price_fft(pricing::OptionType type,
                        const Series& spot,
                        const Series& strike,
                        const Series& rate,
                        const Series& dividend_yield,
                        const Series& maturity,
                        const pricing::HestonFftOptions& options,
                        pluss::codem::Config cfg) const {
  const auto out_cfg = f64_cfg_(cfg, spot.platform());
  const instrument::Vanilla inst{
      .type = type,
      .strike = strike,
      .maturity = maturity,
  };
  const market::State mkt{
      .spot = spot,
      .rate = rate,
      .dividend_yield = dividend_yield,
  };
  const auto mdl = to_series_model_(params_, out_cfg);
  const pricer::Fft fft{
      .vol_risk_premium = options.vol_risk_premium,
      .mode = options.mode,
      .num_fft = options.num_fft,
      .du = options.characteristic_fcn_step,
      .dk = options.log_strike_step,
      .alpha = options.damping_factor,
      .quadrature = options.quadrature,
      .return_grid = false,
  };
  return fft.price(inst, mdl, mkt, pricer::RiskSpec::PriceOnly, out_cfg).price;
}

double Bates::sse(const std::vector<calibration::OptionQuote>& quotes,
                  double spot,
                  double rate,
                  double dividend_yield,
                  const calibration::BatesCalibOptions& options,
                  pluss::codem::Config cfg) const {
  return calibration::bates_sse(quotes, spot, rate, dividend_yield, params_, options, cfg);
}

calibration::BatesCalibResult Bates::calibrate(const std::vector<calibration::OptionQuote>& quotes,
                                               double spot,
                                               double rate,
                                               double dividend_yield,
                                               const calibration::BatesCalibOptions& options,
                                               pluss::codem::Config cfg) const {
  return calibration::calibrate_bates(quotes, spot, rate, dividend_yield, params_, options, cfg);
}

calibration::BatesCalibResult Bates::calibrate_inplace(const std::vector<calibration::OptionQuote>& quotes,
                                                       double spot,
                                                       double rate,
                                                       double dividend_yield,
                                                       const calibration::BatesCalibOptions& options,
                                                       pluss::codem::Config cfg) {
  auto out = calibration::calibrate_bates(quotes, spot, rate, dividend_yield, params_, options, cfg);
  params_ = out.params;
  return out;
}

Sv::Sv(volatility::SvParams params) : params_(params) {}

const volatility::SvParams& Sv::params() const noexcept {
  return params_;
}

void Sv::set_params(const volatility::SvParams& params) {
  params_ = params;
}

volatility::StochVolPaths Sv::paths(std::int64_t steps,
                                    std::int64_t num_paths,
                                    double spot0,
                                    double dt,
                                    const volatility::SimOptions& options,
                                    pluss::codem::Config cfg,
                                    std::uint64_t seed) const {
  return volatility::sv_paths(steps, num_paths, spot0, dt, params_, options, cfg, seed);
}

double Sv::price(pricing::OptionType type,
                 double spot,
                 double strike,
                 double rate,
                 double dividend_yield,
                 double maturity,
                 const pricing::MonteCarloOptions& options,
                 pluss::codem::Config cfg) const {
  return pricing::sv_price_mc(type, spot, strike, rate, dividend_yield, maturity, params_, options, cfg);
}

double Sv::price_mc(pricing::OptionType type,
                    double spot,
                    double strike,
                    double rate,
                    double dividend_yield,
                    double maturity,
                    const pricing::MonteCarloOptions& options,
                    pluss::codem::Config cfg) const {
  return pricing::sv_price_mc(type, spot, strike, rate, dividend_yield, maturity, params_, options, cfg);
}

double Sv::sse(const std::vector<calibration::OptionQuote>& quotes,
               double spot,
               double rate,
               double dividend_yield,
               const calibration::SvCalibOptions& options,
               pluss::codem::Config cfg) const {
  return calibration::sv_sse(quotes, spot, rate, dividend_yield, params_, options, cfg);
}

calibration::SvCalibResult Sv::calibrate(const std::vector<calibration::OptionQuote>& quotes,
                                         double spot,
                                         double rate,
                                         double dividend_yield,
                                         const calibration::SvCalibOptions& options,
                                         pluss::codem::Config cfg) const {
  return calibration::calibrate_sv(quotes, spot, rate, dividend_yield, params_, options, cfg);
}

calibration::SvCalibResult Sv::calibrate_inplace(const std::vector<calibration::OptionQuote>& quotes,
                                                 double spot,
                                                 double rate,
                                                 double dividend_yield,
                                                 const calibration::SvCalibOptions& options,
                                                 pluss::codem::Config cfg) {
  auto out = calibration::calibrate_sv(quotes, spot, rate, dividend_yield, params_, options, cfg);
  params_ = out.params;
  return out;
}

HestonNandi::HestonNandi(volatility::HestonNandiParams params) : params_(params) {}

const volatility::HestonNandiParams& HestonNandi::params() const noexcept {
  return params_;
}

void HestonNandi::set_params(const volatility::HestonNandiParams& params) {
  params_ = params;
}

volatility::StochVolPaths HestonNandi::paths(std::int64_t steps,
                                             std::int64_t num_paths,
                                             double spot0,
                                             double dt,
                                             const volatility::SimOptions& options,
                                             pluss::codem::Config cfg,
                                             std::uint64_t seed) const {
  return volatility::heston_nandi_paths(steps, num_paths, spot0, dt, params_, options, cfg, seed);
}

double HestonNandi::price_mc(pricing::OptionType type,
                             double spot,
                             double strike,
                             double rate,
                             double dividend_yield,
                             double maturity,
                             const pricing::MonteCarloOptions& options,
                             pluss::codem::Config cfg) const {
  return pricing::heston_nandi_price_mc(type, spot, strike, rate, dividend_yield, maturity, params_, options, cfg);
}

double HestonNandi::price(pricing::OptionType type,
                          double spot,
                          double strike,
                          double rate,
                          double dividend_yield,
                          double maturity,
                          const pricing::HestonNandiIntegrationOptions& options) const {
  return pricing::heston_nandi_price(type, spot, strike, rate, dividend_yield, maturity, params_, options);
}

double HestonNandi::returns_nll(const pluss::codem::Series& returns, pluss::codem::Config cfg) const {
  return calibration::heston_nandi_returns_nll(returns, params_, cfg);
}

double HestonNandi::sse(const std::vector<calibration::OptionQuote>& quotes,
                        double spot,
                        double rate,
                        double dividend_yield,
                        const calibration::HestonNandiCalibOptions& options,
                        pluss::codem::Config cfg) const {
  return calibration::heston_nandi_sse(quotes, spot, rate, dividend_yield, params_, options, cfg);
}

calibration::HestonNandiCalibResult HestonNandi::calibrate(const pluss::codem::Series& returns,
                                                           const std::vector<calibration::OptionQuote>& quotes,
                                                           double spot,
                                                           double rate,
                                                           double dividend_yield,
                                                           const calibration::HestonNandiCalibOptions& options,
                                                           pluss::codem::Config cfg) const {
  return calibration::calibrate_heston_nandi(returns, quotes, spot, rate, dividend_yield, params_, options, cfg);
}

calibration::HestonNandiCalibResult HestonNandi::calibrate_inplace(const pluss::codem::Series& returns,
                                                                   const std::vector<calibration::OptionQuote>& quotes,
                                                                   double spot,
                                                                   double rate,
                                                                   double dividend_yield,
                                                                   const calibration::HestonNandiCalibOptions& options,
                                                                   pluss::codem::Config cfg) {
  auto out = calibration::calibrate_heston_nandi(returns, quotes, spot, rate, dividend_yield, params_, options, cfg);
  params_ = out.params;
  return out;
}

Sabr::Sabr(volatility::SabrParams params) : params_(params) {}

const volatility::SabrParams& Sabr::params() const noexcept {
  return params_;
}

void Sabr::set_params(const volatility::SabrParams& params) {
  params_ = params;
}

volatility::StochVolPaths Sabr::paths(std::int64_t steps,
                                      std::int64_t num_paths,
                                      double spot0,
                                      double dt,
                                      const volatility::SimOptions& options,
                                      pluss::codem::Config cfg,
                                      std::uint64_t seed) const {
  return volatility::sabr_paths(steps, num_paths, spot0, dt, params_, options, cfg, seed);
}

double Sabr::price_mc(pricing::OptionType type,
                      double spot,
                      double strike,
                      double rate,
                      double dividend_yield,
                      double maturity,
                      const pricing::MonteCarloOptions& options,
                      pluss::codem::Config cfg) const {
  (void)dividend_yield;
  return pricing::sabr_price_mc(type, spot, strike, rate, maturity, params_, options, cfg);
}

Slv::Slv(volatility::SlvParams params) : params_(params) {}

const volatility::SlvParams& Slv::params() const noexcept {
  return params_;
}

void Slv::set_params(const volatility::SlvParams& params) {
  params_ = params;
}

volatility::StochVolPaths Slv::paths(std::int64_t steps,
                                     std::int64_t num_paths,
                                     double spot0,
                                     double dt,
                                     const volatility::SimOptions& options,
                                     pluss::codem::Config cfg,
                                     std::uint64_t seed) const {
  return volatility::slv_paths(steps, num_paths, spot0, dt, params_, options, cfg, seed);
}

double Slv::price_mc(pricing::OptionType type,
                     double spot,
                     double strike,
                     double rate,
                     double dividend_yield,
                     double maturity,
                     const pricing::MonteCarloOptions& options,
                     pluss::codem::Config cfg) const {
  return pricing::slv_price_mc(type, spot, strike, rate, dividend_yield, maturity, params_, options, cfg);
}

RoughHeston::RoughHeston(volatility::RoughHestonParams params) : params_(params) {}

const volatility::RoughHestonParams& RoughHeston::params() const noexcept {
  return params_;
}

void RoughHeston::set_params(const volatility::RoughHestonParams& params) {
  params_ = params;
}

volatility::StochVolPaths RoughHeston::paths(std::int64_t steps,
                                             std::int64_t num_paths,
                                             double spot0,
                                             double dt,
                                             const volatility::SimOptions& options,
                                             pluss::codem::Config cfg,
                                             std::uint64_t seed) const {
  return volatility::rough_heston_paths(steps, num_paths, spot0, dt, params_, options, cfg, seed);
}

double RoughHeston::price_mc(pricing::OptionType type,
                             double spot,
                             double strike,
                             double rate,
                             double dividend_yield,
                             double maturity,
                             const pricing::MonteCarloOptions& options,
                             pluss::codem::Config cfg) const {
  return pricing::rough_heston_price_mc(type, spot, strike, rate, dividend_yield, maturity, params_, options, cfg);
}

RoughBergomi::RoughBergomi(volatility::RoughBergomiParams params) : params_(params) {}

const volatility::RoughBergomiParams& RoughBergomi::params() const noexcept {
  return params_;
}

void RoughBergomi::set_params(const volatility::RoughBergomiParams& params) {
  params_ = params;
}

volatility::StochVolPaths RoughBergomi::paths(std::int64_t steps,
                                              std::int64_t num_paths,
                                              double spot0,
                                              double dt,
                                              const volatility::SimOptions& options,
                                              pluss::codem::Config cfg,
                                              std::uint64_t seed) const {
  return volatility::rough_bergomi_paths(steps, num_paths, spot0, dt, params_, options, cfg, seed);
}

double RoughBergomi::price_mc(pricing::OptionType type,
                              double spot,
                              double strike,
                              double rate,
                              double dividend_yield,
                              double maturity,
                              const pricing::MonteCarloOptions& options,
                              pluss::codem::Config cfg) const {
  return pricing::rough_bergomi_price_mc(type, spot, strike, rate, dividend_yield, maturity, params_, options, cfg);
}

VasicekRate::VasicekRate(rates::VasicekParams params) : params_(params) {}

const rates::VasicekParams& VasicekRate::params() const noexcept {
  return params_;
}

void VasicekRate::set_params(const rates::VasicekParams& params) {
  params_ = params;
}

rates::RatePaths VasicekRate::paths(std::int64_t steps,
                                    std::int64_t num_paths,
                                    double dt,
                                    const rates::SimOptions& options,
                                    pluss::codem::Config cfg,
                                    std::uint64_t seed) const {
  return rates::vasicek_paths(steps, num_paths, dt, params_, options, cfg, seed);
}

double VasicekRate::zcb(double maturity, double short_rate) const {
  return rates::vasicek_zcb(maturity, short_rate, params_);
}

double VasicekRate::sse(const std::vector<calibration::RateQuote>& quotes,
                        const calibration::VasicekCalibOptions& options) const {
  return calibration::vasicek_sse(quotes, params_, options);
}

calibration::VasicekCalibResult VasicekRate::calibrate(const std::vector<calibration::RateQuote>& quotes,
                                                       const calibration::VasicekCalibOptions& options) const {
  return calibration::calibrate_vasicek(quotes, params_, options);
}

calibration::VasicekCalibResult VasicekRate::calibrate_inplace(const std::vector<calibration::RateQuote>& quotes,
                                                               const calibration::VasicekCalibOptions& options) {
  auto out = calibration::calibrate_vasicek(quotes, params_, options);
  params_ = out.params;
  return out;
}

CirRate::CirRate(rates::CirParams params) : params_(params) {}

const rates::CirParams& CirRate::params() const noexcept {
  return params_;
}

void CirRate::set_params(const rates::CirParams& params) {
  params_ = params;
}

rates::RatePaths CirRate::paths(std::int64_t steps,
                                std::int64_t num_paths,
                                double dt,
                                const rates::SimOptions& options,
                                pluss::codem::Config cfg,
                                std::uint64_t seed) const {
  return rates::cir_paths(steps, num_paths, dt, params_, options, cfg, seed);
}

double CirRate::zcb(double maturity, double short_rate) const {
  return rates::cir_zcb(maturity, short_rate, params_);
}

double CirRate::sse(const std::vector<calibration::RateQuote>& quotes, const calibration::CirCalibOptions& options) const {
  return calibration::cir_sse(quotes, params_, options);
}

calibration::CirCalibResult CirRate::calibrate(const std::vector<calibration::RateQuote>& quotes,
                                               const calibration::CirCalibOptions& options) const {
  return calibration::calibrate_cir(quotes, params_, options);
}

calibration::CirCalibResult CirRate::calibrate_inplace(const std::vector<calibration::RateQuote>& quotes,
                                                       const calibration::CirCalibOptions& options) {
  auto out = calibration::calibrate_cir(quotes, params_, options);
  params_ = out.params;
  return out;
}

HullWhiteRate::HullWhiteRate(rates::HullWhiteParams params) : params_(params) {}

const rates::HullWhiteParams& HullWhiteRate::params() const noexcept {
  return params_;
}

void HullWhiteRate::set_params(const rates::HullWhiteParams& params) {
  params_ = params;
}

rates::RatePaths HullWhiteRate::paths(std::int64_t steps,
                                      std::int64_t num_paths,
                                      double dt,
                                      const rates::SimOptions& options,
                                      pluss::codem::Config cfg,
                                      std::uint64_t seed) const {
  return rates::hull_white_paths(steps, num_paths, dt, params_, options, cfg, seed);
}

double HullWhiteRate::zcb(double maturity, double short_rate) const {
  return rates::hull_white_zcb(maturity, short_rate, params_);
}

double HullWhiteRate::sse(const std::vector<calibration::RateQuote>& quotes,
                          const calibration::HullWhiteCalibOptions& options) const {
  return calibration::hull_white_sse(quotes, params_, options);
}

calibration::HullWhiteCalibResult HullWhiteRate::calibrate(const std::vector<calibration::RateQuote>& quotes,
                                                           const calibration::HullWhiteCalibOptions& options) const {
  return calibration::calibrate_hull_white(quotes, params_, options);
}

calibration::HullWhiteCalibResult HullWhiteRate::calibrate_inplace(const std::vector<calibration::RateQuote>& quotes,
                                                                   const calibration::HullWhiteCalibOptions& options) {
  auto out = calibration::calibrate_hull_white(quotes, params_, options);
  params_ = out.params;
  return out;
}

HjmRate::HjmRate(rates::HjmParams params) : params_(std::move(params)) {}

const rates::HjmParams& HjmRate::params() const noexcept {
  return params_;
}

void HjmRate::set_params(const rates::HjmParams& params) {
  params_ = params;
}

rates::HjmPaths HjmRate::paths(std::int64_t steps,
                               std::int64_t num_paths,
                               double dt,
                               const rates::SimOptions& options,
                               pluss::codem::Config cfg,
                               std::uint64_t seed) const {
  return rates::hjm_paths(steps, num_paths, dt, params_, options, cfg, seed);
}

double HjmRate::zcb_mc(double maturity,
                       std::int64_t paths,
                       std::int64_t steps,
                       rates::SimOptions options,
                       pluss::codem::Config cfg) const {
  return rates::hjm_zcb_mc(maturity, params_, paths, steps, options, cfg);
}

double HjmRate::sse(const std::vector<calibration::RateQuote>& quotes, const calibration::HjmCalibOptions& options) const {
  return calibration::hjm_sse(quotes, params_, options);
}

calibration::HjmCalibResult HjmRate::calibrate(const std::vector<calibration::RateQuote>& quotes,
                                               const calibration::HjmCalibOptions& options) const {
  return calibration::calibrate_hjm(quotes, params_, options);
}

calibration::HjmCalibResult HjmRate::calibrate_inplace(const std::vector<calibration::RateQuote>& quotes,
                                                       const calibration::HjmCalibOptions& options) {
  auto out = calibration::calibrate_hjm(quotes, params_, options);
  params_ = out.params;
  return out;
}

} // namespace pluss::subsp::math::finance::models
