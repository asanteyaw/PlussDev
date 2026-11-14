#pragma once

#include "pluss/codem/codem.h"
#include "pluss/export.h"
#include "pluss/subsp/actuarial/operator.h"

#include <cstdint>

namespace pluss::subsp::actuarial {

class CODEM_API Mortality final {
public:
  Mortality() = default;

  [[nodiscard]] bool defined() const noexcept;
  [[nodiscard]] std::int64_t horizon() const noexcept;
  [[nodiscard]] const pluss::codem::Series& qx() const noexcept;
  [[nodiscard]] pluss::codem::Series px() const;
  [[nodiscard]] pluss::codem::Series force() const;
  [[nodiscard]] pluss::codem::Series survival() const;
  [[nodiscard]] pluss::codem::Series lifetime_pmf() const;
  [[nodiscard]] pluss::codem::Series life_table(double radix = 100000.0) const;

private:
  explicit Mortality(pluss::codem::Series qx);
  pluss::codem::Series qx_ = pluss::series::undefined();

  friend Mortality mortality_from_qx_(const pluss::codem::Series&);
};

using MortalityModel = Operator<Mortality(const pluss::codem::Series&)>;

namespace mortality {
[[nodiscard]] CODEM_API Mortality from_qx(const pluss::codem::Series& qx);
[[nodiscard]] CODEM_API Mortality from_px(const pluss::codem::Series& px);
[[nodiscard]] CODEM_API Mortality from_force(const pluss::codem::Series& force);
[[nodiscard]] CODEM_API Mortality from_lx(const pluss::codem::Series& lx);
[[nodiscard]] CODEM_API Mortality select_ultimate(
    const pluss::codem::Series& select_qx,
    const pluss::codem::Series& ultimate_qx,
    std::int64_t projection_years);
[[nodiscard]] CODEM_API Mortality gompertz(
    const pluss::codem::Series& attained_ages, double b, double c);
[[nodiscard]] CODEM_API MortalityModel gompertz(double b, double c);
[[nodiscard]] CODEM_API Mortality makeham(
    const pluss::codem::Series& attained_ages, double a, double b, double c);
[[nodiscard]] CODEM_API MortalityModel makeham(double a, double b, double c);
} // namespace mortality

class CODEM_API Discount final {
public:
  Discount() = default;

  [[nodiscard]] bool defined() const noexcept;
  [[nodiscard]] pluss::codem::Series factors(
      std::int64_t horizon,
      pluss::codem::Platform platform = pluss::codem::Platform::CPU) const;

private:
  enum class Kind : std::uint8_t { Undefined, Flat, Curve };

  Kind kind_ = Kind::Undefined;
  double rate_ = 0.0;
  pluss::codem::Series curve_ = pluss::series::undefined();

  friend Discount discount_flat_(double);
  friend Discount discount_curve_(const pluss::codem::Series&);
};

namespace discount {
[[nodiscard]] CODEM_API Discount flat(double annual_effective_rate);
[[nodiscard]] CODEM_API Discount curve(const pluss::codem::Series& factors);
} // namespace discount

struct CODEM_API Basis final {
  Mortality mortality{};
  Discount discount{};
};

class CODEM_API Cashflow final {
public:
  Cashflow() = default;

  [[nodiscard]] bool defined() const noexcept;
  [[nodiscard]] std::int64_t horizon() const noexcept;
  [[nodiscard]] const pluss::codem::Series& survival_payments() const noexcept;
  [[nodiscard]] const pluss::codem::Series& death_payments() const noexcept;

private:
  Cashflow(pluss::codem::Series survival_payments,
           pluss::codem::Series death_payments);

  pluss::codem::Series survival_payments_ = pluss::series::undefined();
  pluss::codem::Series death_payments_ = pluss::series::undefined();

  friend Cashflow cashflow_make_(const pluss::codem::Series&,
                                 const pluss::codem::Series&);
};

enum class PaymentTiming : std::uint8_t { Start, End };

namespace cashflow {
// survival_payments has length n+1 at times 0..n; death_payments has length n
// and is paid at the ends of years 1..n on death during the corresponding year.
[[nodiscard]] CODEM_API Cashflow make(
    const pluss::codem::Series& survival_payments,
    const pluss::codem::Series& death_payments);
[[nodiscard]] CODEM_API Cashflow annuity(std::int64_t years,
                                        double amount = 1.0,
                                        PaymentTiming timing = PaymentTiming::Start);
[[nodiscard]] CODEM_API Cashflow death(std::int64_t years,
                                      double benefit = 1.0);
[[nodiscard]] CODEM_API Cashflow survival(std::int64_t years,
                                         double benefit = 1.0);
[[nodiscard]] CODEM_API Cashflow endowment(std::int64_t years,
                                          double death_benefit = 1.0,
                                          double survival_benefit = 1.0);
} // namespace cashflow

using Valuation = Operator<pluss::codem::Series(const Cashflow&)>;

namespace operators {
[[nodiscard]] CODEM_API Valuation value(Basis basis);
} // namespace operators

} // namespace pluss::subsp::actuarial
