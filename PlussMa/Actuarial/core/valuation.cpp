#include "pluss/subsp/actuarial/valuation.h"

#include "pluss/config.h"
#include "pluss/subsp/actuarial/core.h"
#include "pluss/subsp/actuarial/life_contingencies.h"

#include <cmath>
#include <vector>

namespace pluss::subsp::actuarial {
namespace {

using pluss::codem::ScalarType;
using pluss::codem::Series;

Series as_f64_vector_(const Series& value, std::string_view name) {
  PLUSS_CHECK(value.defined(), std::string(name) + " must be defined");
  PLUSS_CHECK(value.dim() == 1, std::string(name) + " must be 1D");
  const Series out = value.to(ScalarType::Float64).contiguous();
  PLUSS_CHECK(out.isfinite().all().item<bool>(),
              std::string(name) + " must be finite");
  return out;
}

Series zeros_(std::int64_t size, pluss::codem::Platform platform) {
  return Series::zeros({size}, ScalarType::Float64).to(platform);
}

Series filled_(std::int64_t size, double value,
               pluss::codem::Platform platform) {
  return Series::ones({size}, ScalarType::Float64).to(platform).mul(value);
}

} // namespace

Mortality::Mortality(Series qx) : qx_(std::move(qx)) {}

bool Mortality::defined() const noexcept { return qx_.defined(); }

std::int64_t Mortality::horizon() const noexcept {
  return defined() ? qx_.numel() : 0;
}

const Series& Mortality::qx() const noexcept { return qx_; }

Series Mortality::px() const {
  PLUSS_CHECK(defined(), "Mortality is undefined");
  return life_contingencies::px_qx(qx_);
}

Series Mortality::force() const {
  PLUSS_CHECK(defined(), "Mortality is undefined");
  return life_contingencies::force_qx(qx_);
}

Series Mortality::survival() const {
  PLUSS_CHECK(defined(), "Mortality is undefined");
  return survival_curve(qx_);
}

Series Mortality::lifetime_pmf() const {
  PLUSS_CHECK(defined(), "Mortality is undefined");
  return life_contingencies::lifetime_pmf(qx_);
}

Series Mortality::life_table(double radix) const {
  PLUSS_CHECK(defined(), "Mortality is undefined");
  return life_contingencies::lx_qx(qx_, radix);
}

Mortality mortality_from_qx_(const Series& qx) {
  // survival_curve is the canonical qx validator and preserves the platform.
  (void)survival_curve(qx);
  return Mortality(qx.to(ScalarType::Float64).contiguous());
}

namespace mortality {

Mortality from_qx(const Series& qx) { return mortality_from_qx_(qx); }

Mortality from_px(const Series& px) {
  return from_qx(life_contingencies::qx_px(px));
}

Mortality from_force(const Series& force) {
  return from_qx(life_contingencies::qx_force(force));
}

Mortality from_lx(const Series& lx) {
  return from_qx(life_contingencies::qx_lx(lx));
}

Mortality select_ultimate(const Series& select_qx,
                          const Series& ultimate_qx,
                          std::int64_t projection_years) {
  return from_qx(life_contingencies::select_qx(
      select_qx, ultimate_qx, projection_years));
}

Mortality gompertz(const Series& attained_ages, double b, double c) {
  return from_qx(life_contingencies::gompertz_qx(attained_ages, b, c));
}

MortalityModel gompertz(double b, double c) {
  return MortalityModel("gompertz", [b, c](const Series& attained_ages) {
    return gompertz(attained_ages, b, c);
  });
}

Mortality makeham(const Series& attained_ages, double a, double b, double c) {
  return from_qx(
      life_contingencies::makeham_qx(attained_ages, a, b, c));
}

MortalityModel makeham(double a, double b, double c) {
  return MortalityModel("makeham", [a, b, c](const Series& attained_ages) {
    return makeham(attained_ages, a, b, c);
  });
}

} // namespace mortality

bool Discount::defined() const noexcept { return kind_ != Kind::Undefined; }

Series Discount::factors(std::int64_t horizon,
                         pluss::codem::Platform platform) const {
  PLUSS_CHECK(defined(), "Discount is undefined");
  PLUSS_CHECK(horizon >= 0, "Discount::factors: horizon must be >= 0");
  if (kind_ == Kind::Curve) {
    PLUSS_CHECK(curve_.numel() >= horizon + 1,
                "Discount::factors: curve must contain at least horizon + 1 values");
    return curve_.slice(0, 0, horizon + 1, 1)
        .to(platform, ScalarType::Float64)
        .contiguous();
  }
  const double log_discount = -std::log1p(rate_);
  return Series::arange(0, horizon + 1, 1, ScalarType::Int64)
      .to(platform, ScalarType::Float64)
      .mul(log_discount)
      .exp();
}

Discount discount_flat_(double rate) {
  PLUSS_CHECK(std::isfinite(rate), "discount::flat: rate must be finite");
  PLUSS_CHECK(rate > -1.0, "discount::flat: rate must be > -1");
  Discount out;
  out.kind_ = Discount::Kind::Flat;
  out.rate_ = rate;
  return out;
}

Discount discount_curve_(const Series& factors) {
  const Series clean = as_f64_vector_(factors, "discount::curve: factors");
  PLUSS_CHECK(clean.numel() >= 1,
              "discount::curve: factors must contain time zero");
  PLUSS_CHECK(clean.gt(0.0).all().item<bool>(),
              "discount::curve: factors must be > 0");
  Discount out;
  out.kind_ = Discount::Kind::Curve;
  out.curve_ = clean;
  return out;
}

namespace discount {
Discount flat(double annual_effective_rate) {
  return discount_flat_(annual_effective_rate);
}
Discount curve(const Series& factors) { return discount_curve_(factors); }
} // namespace discount

Cashflow::Cashflow(Series survival_payments, Series death_payments)
    : survival_payments_(std::move(survival_payments)),
      death_payments_(std::move(death_payments)) {}

bool Cashflow::defined() const noexcept {
  return survival_payments_.defined() && death_payments_.defined();
}

std::int64_t Cashflow::horizon() const noexcept {
  return defined() ? death_payments_.numel() : 0;
}

const Series& Cashflow::survival_payments() const noexcept {
  return survival_payments_;
}

const Series& Cashflow::death_payments() const noexcept {
  return death_payments_;
}

Cashflow cashflow_make_(const Series& survival_payments,
                        const Series& death_payments) {
  const Series alive = as_f64_vector_(
      survival_payments, "cashflow::make: survival_payments");
  const Series death = as_f64_vector_(
      death_payments, "cashflow::make: death_payments");
  PLUSS_CHECK(alive.numel() == death.numel() + 1,
              "cashflow::make: survival_payments must have one more value than death_payments");
  return Cashflow(alive, death.to(alive.platform()));
}

namespace cashflow {

Cashflow make(const Series& survival_payments,
              const Series& death_payments) {
  return cashflow_make_(survival_payments, death_payments);
}

Cashflow annuity(std::int64_t years, double amount, PaymentTiming timing) {
  PLUSS_CHECK(years >= 0, "cashflow::annuity: years must be >= 0");
  PLUSS_CHECK(std::isfinite(amount), "cashflow::annuity: amount must be finite");
  const auto platform = pluss::codem::Platform::CPU;
  Series alive = zeros_(years + 1, platform);
  if (years > 0) {
    const std::int64_t start = timing == PaymentTiming::Start ? 0 : 1;
    const std::int64_t end = timing == PaymentTiming::Start ? years : years + 1;
    Series payments = filled_(years, amount, platform);
    alive = alive.slice_scatter(payments, 0, start, end, 1);
  }
  return cashflow_make_(alive, zeros_(years, platform));
}

Cashflow death(std::int64_t years, double benefit) {
  PLUSS_CHECK(years >= 0, "cashflow::death: years must be >= 0");
  PLUSS_CHECK(std::isfinite(benefit), "cashflow::death: benefit must be finite");
  const auto platform = pluss::codem::Platform::CPU;
  return cashflow_make_(zeros_(years + 1, platform),
                        filled_(years, benefit, platform));
}

Cashflow survival(std::int64_t years, double benefit) {
  PLUSS_CHECK(years >= 0, "cashflow::survival: years must be >= 0");
  PLUSS_CHECK(std::isfinite(benefit), "cashflow::survival: benefit must be finite");
  const auto platform = pluss::codem::Platform::CPU;
  Series alive = zeros_(years + 1, platform);
  alive = alive.slice_scatter(filled_(1, benefit, platform), 0,
                              years, years + 1, 1);
  return cashflow_make_(alive, zeros_(years, platform));
}

Cashflow endowment(std::int64_t years, double death_benefit,
                   double survival_benefit) {
  PLUSS_CHECK(years >= 0, "cashflow::endowment: years must be >= 0");
  PLUSS_CHECK(std::isfinite(death_benefit),
              "cashflow::endowment: death_benefit must be finite");
  PLUSS_CHECK(std::isfinite(survival_benefit),
              "cashflow::endowment: survival_benefit must be finite");
  const auto platform = pluss::codem::Platform::CPU;
  Series alive = zeros_(years + 1, platform);
  alive = alive.slice_scatter(filled_(1, survival_benefit, platform), 0,
                              years, years + 1, 1);
  return cashflow_make_(alive, filled_(years, death_benefit, platform));
}

} // namespace cashflow

namespace operators {

Valuation value(Basis basis) {
  PLUSS_CHECK(basis.mortality.defined(),
              "operators::value: mortality must be defined");
  PLUSS_CHECK(basis.discount.defined(),
              "operators::value: discount must be defined");

  return Valuation("value", [basis = std::move(basis)](const Cashflow& flow) {
    PLUSS_CHECK(flow.defined(), "valuation: cashflow must be defined");
    PLUSS_CHECK(flow.horizon() <= basis.mortality.horizon(),
                "valuation: cashflow horizon exceeds mortality horizon");

    const std::int64_t n = flow.horizon();
    const auto platform = basis.mortality.qx().platform();
    const Series survival = basis.mortality.survival().slice(0, 0, n + 1, 1);
    const Series discount = basis.discount.factors(n, platform);
    const Series alive = flow.survival_payments().to(platform, ScalarType::Float64);
    Series value = alive.mul(survival).mul(discount).sum();

    if (n > 0) {
      const Series death_probability = survival.slice(0, 0, n, 1)
                                           .mul(basis.mortality.qx().slice(0, 0, n, 1));
      const Series death_discount = discount.slice(0, 1, n + 1, 1);
      const Series death = flow.death_payments().to(platform, ScalarType::Float64);
      value = value.add(death.mul(death_probability).mul(death_discount).sum());
    }
    return value;
  });
}

} // namespace operators

} // namespace pluss::subsp::actuarial
