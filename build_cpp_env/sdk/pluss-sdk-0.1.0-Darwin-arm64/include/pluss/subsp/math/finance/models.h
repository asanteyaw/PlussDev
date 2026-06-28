#pragma once

#include "pluss/subsp/math/finance/pricing.h"
#include "pluss/subsp/math/finance/rates.h"
#include "pluss/subsp/math/finance/volatility.h"

#include <cstdint>
#include <vector>

namespace pluss::subsp::math::finance::models {

class CODEM_API StochVolBase {
 public:
  virtual ~StochVolBase() = default;

  [[nodiscard]] virtual volatility::StochVolPaths paths(std::int64_t steps,
                                                        std::int64_t num_paths,
                                                        double spot0,
                                                        double dt,
                                                        const volatility::SimOptions& options = {},
                                                        pluss::codem::Config cfg = {},
                                                        std::uint64_t seed = 0) const = 0;

  [[nodiscard]] virtual double price_mc(pricing::OptionType type,
                                        double spot,
                                        double strike,
                                        double rate,
                                        double dividend_yield,
                                        double maturity,
                                        const pricing::MonteCarloOptions& options = {},
                                        pluss::codem::Config cfg = {}) const = 0;
};

class CODEM_API Heston final : public StochVolBase {
 public:
  explicit Heston(volatility::HestonParams params = {});

  [[nodiscard]] const volatility::HestonParams& params() const noexcept;
  void set_params(const volatility::HestonParams& params);

  [[nodiscard]] volatility::StochVolPaths paths(std::int64_t steps,
                                                std::int64_t num_paths,
                                                double spot0,
                                                double dt,
                                                const volatility::SimOptions& options = {},
                                                pluss::codem::Config cfg = {},
                                                std::uint64_t seed = 0) const override;

  [[nodiscard]] double price(pricing::OptionType type,
                             double spot,
                             double strike,
                             double rate,
                             double dividend_yield,
                             double maturity,
                             const pricing::HestonIntegrationOptions& options = {}) const;

  [[nodiscard]] pluss::codem::Series price(pricing::OptionType type,
                                           const pluss::codem::Series& spot,
                                           const pluss::codem::Series& strike,
                                           const pluss::codem::Series& rate,
                                           const pluss::codem::Series& dividend_yield,
                                           const pluss::codem::Series& maturity,
                                           const pricing::HestonFftOptions& options = {},
                                           pluss::codem::Config cfg = {}) const;

  [[nodiscard]] double price_mc(pricing::OptionType type,
                                double spot,
                                double strike,
                                double rate,
                                double dividend_yield,
                                double maturity,
                                const pricing::MonteCarloOptions& options = {},
                                pluss::codem::Config cfg = {}) const override;

  [[nodiscard]] pluss::codem::Series price_mc(pricing::OptionType type,
                                              const pluss::codem::Series& spot,
                                              const pluss::codem::Series& strike,
                                              const pluss::codem::Series& rate,
                                              const pluss::codem::Series& dividend_yield,
                                              const pluss::codem::Series& maturity,
                                              const pricing::MonteCarloOptions& options = {},
                                              pluss::codem::Config cfg = {}) const;

 private:
  volatility::HestonParams params_{};
};

class CODEM_API Bates final : public StochVolBase {
 public:
  explicit Bates(volatility::BatesParams params = {});

  [[nodiscard]] const volatility::BatesParams& params() const noexcept;
  void set_params(const volatility::BatesParams& params);

  [[nodiscard]] volatility::StochVolPaths paths(std::int64_t steps,
                                                std::int64_t num_paths,
                                                double spot0,
                                                double dt,
                                                const volatility::SimOptions& options = {},
                                                pluss::codem::Config cfg = {},
                                                std::uint64_t seed = 0) const override;

  [[nodiscard]] double price_mc(pricing::OptionType type,
                                double spot,
                                double strike,
                                double rate,
                                double dividend_yield,
                                double maturity,
                                const pricing::MonteCarloOptions& options = {},
                                pluss::codem::Config cfg = {}) const override;

  [[nodiscard]] double price_fft(pricing::OptionType type,
                                 double spot,
                                 double strike,
                                 double rate,
                                 double dividend_yield,
                                 double maturity,
                                 const pricing::HestonFftOptions& options = {},
                                 pluss::codem::Config cfg = {}) const;

  [[nodiscard]] pluss::codem::Series price_mc(pricing::OptionType type,
                                              const pluss::codem::Series& spot,
                                              const pluss::codem::Series& strike,
                                              const pluss::codem::Series& rate,
                                              const pluss::codem::Series& dividend_yield,
                                              const pluss::codem::Series& maturity,
                                              const pricing::MonteCarloOptions& options = {},
                                              pluss::codem::Config cfg = {}) const;

  [[nodiscard]] pluss::codem::Series price_fft(pricing::OptionType type,
                                               const pluss::codem::Series& spot,
                                               const pluss::codem::Series& strike,
                                               const pluss::codem::Series& rate,
                                               const pluss::codem::Series& dividend_yield,
                                               const pluss::codem::Series& maturity,
                                               const pricing::HestonFftOptions& options = {},
                                               pluss::codem::Config cfg = {}) const;

 private:
  volatility::BatesParams params_{};
};

class CODEM_API Sv final : public StochVolBase {
 public:
  explicit Sv(volatility::SvParams params = {});

  [[nodiscard]] const volatility::SvParams& params() const noexcept;
  void set_params(const volatility::SvParams& params);

  [[nodiscard]] volatility::StochVolPaths paths(std::int64_t steps,
                                                std::int64_t num_paths,
                                                double spot0,
                                                double dt,
                                                const volatility::SimOptions& options = {},
                                                pluss::codem::Config cfg = {},
                                                std::uint64_t seed = 0) const override;

  [[nodiscard]] double price(pricing::OptionType type,
                             double spot,
                             double strike,
                             double rate,
                             double dividend_yield,
                             double maturity,
                             const pricing::MonteCarloOptions& options = {},
                             pluss::codem::Config cfg = {}) const;

  [[nodiscard]] double price_mc(pricing::OptionType type,
                                double spot,
                                double strike,
                                double rate,
                                double dividend_yield,
                                double maturity,
                                const pricing::MonteCarloOptions& options = {},
                                pluss::codem::Config cfg = {}) const override;

 private:
 volatility::SvParams params_{};
};

class CODEM_API HestonNandi final : public StochVolBase {
 public:
  explicit HestonNandi(volatility::HestonNandiParams params = {});

  [[nodiscard]] const volatility::HestonNandiParams& params() const noexcept;
  void set_params(const volatility::HestonNandiParams& params);

  [[nodiscard]] volatility::StochVolPaths paths(std::int64_t steps,
                                                std::int64_t num_paths,
                                                double spot0,
                                                double dt,
                                                const volatility::SimOptions& options = {},
                                                pluss::codem::Config cfg = {},
                                                std::uint64_t seed = 0) const override;

  [[nodiscard]] double price_mc(pricing::OptionType type,
                                double spot,
                                double strike,
                                double rate,
                                double dividend_yield,
                                double maturity,
                                const pricing::MonteCarloOptions& options = {},
                                pluss::codem::Config cfg = {}) const override;

  [[nodiscard]] double price(pricing::OptionType type,
                             double spot,
                             double strike,
                             double rate,
                             double dividend_yield,
                             double maturity,
                             const pricing::HestonNandiIntegrationOptions& options = {}) const;

 private:
  volatility::HestonNandiParams params_{};
};

class CODEM_API Sabr final : public StochVolBase {
 public:
  explicit Sabr(volatility::SabrParams params = {});

  [[nodiscard]] const volatility::SabrParams& params() const noexcept;
  void set_params(const volatility::SabrParams& params);

  [[nodiscard]] volatility::StochVolPaths paths(std::int64_t steps,
                                                std::int64_t num_paths,
                                                double spot0,
                                                double dt,
                                                const volatility::SimOptions& options = {},
                                                pluss::codem::Config cfg = {},
                                                std::uint64_t seed = 0) const override;

  [[nodiscard]] double price_mc(pricing::OptionType type,
                                double spot,
                                double strike,
                                double rate,
                                double dividend_yield,
                                double maturity,
                                const pricing::MonteCarloOptions& options = {},
                                pluss::codem::Config cfg = {}) const override;

 private:
  volatility::SabrParams params_{};
};

class CODEM_API Slv final : public StochVolBase {
 public:
  explicit Slv(volatility::SlvParams params = {});

  [[nodiscard]] const volatility::SlvParams& params() const noexcept;
  void set_params(const volatility::SlvParams& params);

  [[nodiscard]] volatility::StochVolPaths paths(std::int64_t steps,
                                                std::int64_t num_paths,
                                                double spot0,
                                                double dt,
                                                const volatility::SimOptions& options = {},
                                                pluss::codem::Config cfg = {},
                                                std::uint64_t seed = 0) const override;

  [[nodiscard]] double price_mc(pricing::OptionType type,
                                double spot,
                                double strike,
                                double rate,
                                double dividend_yield,
                                double maturity,
                                const pricing::MonteCarloOptions& options = {},
                                pluss::codem::Config cfg = {}) const override;

 private:
  volatility::SlvParams params_{};
};

class CODEM_API RoughHeston final : public StochVolBase {
 public:
  explicit RoughHeston(volatility::RoughHestonParams params = {});

  [[nodiscard]] const volatility::RoughHestonParams& params() const noexcept;
  void set_params(const volatility::RoughHestonParams& params);

  [[nodiscard]] volatility::StochVolPaths paths(std::int64_t steps,
                                                std::int64_t num_paths,
                                                double spot0,
                                                double dt,
                                                const volatility::SimOptions& options = {},
                                                pluss::codem::Config cfg = {},
                                                std::uint64_t seed = 0) const override;

  [[nodiscard]] double price_mc(pricing::OptionType type,
                                double spot,
                                double strike,
                                double rate,
                                double dividend_yield,
                                double maturity,
                                const pricing::MonteCarloOptions& options = {},
                                pluss::codem::Config cfg = {}) const override;

 private:
  volatility::RoughHestonParams params_{};
};

class CODEM_API RoughBergomi final : public StochVolBase {
 public:
  explicit RoughBergomi(volatility::RoughBergomiParams params = {});

  [[nodiscard]] const volatility::RoughBergomiParams& params() const noexcept;
  void set_params(const volatility::RoughBergomiParams& params);

  [[nodiscard]] volatility::StochVolPaths paths(std::int64_t steps,
                                                std::int64_t num_paths,
                                                double spot0,
                                                double dt,
                                                const volatility::SimOptions& options = {},
                                                pluss::codem::Config cfg = {},
                                                std::uint64_t seed = 0) const override;

  [[nodiscard]] double price_mc(pricing::OptionType type,
                                double spot,
                                double strike,
                                double rate,
                                double dividend_yield,
                                double maturity,
                                const pricing::MonteCarloOptions& options = {},
                                pluss::codem::Config cfg = {}) const override;

 private:
  volatility::RoughBergomiParams params_{};
};

class CODEM_API ShortRateBase {
 public:
  virtual ~ShortRateBase() = default;

  [[nodiscard]] virtual rates::RatePaths paths(std::int64_t steps,
                                               std::int64_t num_paths,
                                               double dt,
                                               const rates::SimOptions& options = {},
                                               pluss::codem::Config cfg = {},
                                               std::uint64_t seed = 0) const = 0;

  [[nodiscard]] virtual double zcb(double maturity, double short_rate) const = 0;
};

class CODEM_API VasicekRate final : public ShortRateBase {
 public:
  explicit VasicekRate(rates::VasicekParams params = {});

  [[nodiscard]] const rates::VasicekParams& params() const noexcept;
  void set_params(const rates::VasicekParams& params);

  [[nodiscard]] rates::RatePaths paths(std::int64_t steps,
                                       std::int64_t num_paths,
                                       double dt,
                                       const rates::SimOptions& options = {},
                                       pluss::codem::Config cfg = {},
                                       std::uint64_t seed = 0) const override;

  [[nodiscard]] double zcb(double maturity, double short_rate) const override;

 private:
  rates::VasicekParams params_{};
};

class CODEM_API CirRate final : public ShortRateBase {
 public:
  explicit CirRate(rates::CirParams params = {});

  [[nodiscard]] const rates::CirParams& params() const noexcept;
  void set_params(const rates::CirParams& params);

  [[nodiscard]] rates::RatePaths paths(std::int64_t steps,
                                       std::int64_t num_paths,
                                       double dt,
                                       const rates::SimOptions& options = {},
                                       pluss::codem::Config cfg = {},
                                       std::uint64_t seed = 0) const override;

  [[nodiscard]] double zcb(double maturity, double short_rate) const override;

 private:
  rates::CirParams params_{};
};

class CODEM_API HullWhiteRate final : public ShortRateBase {
 public:
  explicit HullWhiteRate(rates::HullWhiteParams params = {});

  [[nodiscard]] const rates::HullWhiteParams& params() const noexcept;
  void set_params(const rates::HullWhiteParams& params);

  [[nodiscard]] rates::RatePaths paths(std::int64_t steps,
                                       std::int64_t num_paths,
                                       double dt,
                                       const rates::SimOptions& options = {},
                                       pluss::codem::Config cfg = {},
                                       std::uint64_t seed = 0) const override;

  [[nodiscard]] double zcb(double maturity, double short_rate) const override;

 private:
  rates::HullWhiteParams params_{};
};

class CODEM_API HjmRate {
 public:
  explicit HjmRate(rates::HjmParams params = {});

  [[nodiscard]] const rates::HjmParams& params() const noexcept;
  void set_params(const rates::HjmParams& params);

  [[nodiscard]] rates::HjmPaths paths(std::int64_t steps,
                                      std::int64_t num_paths,
                                      double dt,
                                      const rates::SimOptions& options = {},
                                      pluss::codem::Config cfg = {},
                                      std::uint64_t seed = 0) const;

  [[nodiscard]] double zcb_mc(double maturity,
                              std::int64_t paths = 20000,
                              std::int64_t steps = 252,
                              rates::SimOptions options = {},
                              pluss::codem::Config cfg = {}) const;

 private:
  rates::HjmParams params_{};
};

} // namespace pluss::subsp::math::finance::models
