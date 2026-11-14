#include "pluss/subsp/actuarial/core.h"
#include "pluss/subsp/actuarial/valuation.h"

#include "pluss/codem/series_factory.h"
#include "pluss/config.h"

#include <cmath>
#include <string_view>
#include <vector>

namespace pluss::subsp::actuarial {
namespace {

using pluss::codem::ScalarType;
using pluss::codem::Series;

void validate_interest_(double i, std::string_view op) {
  PLUSS_CHECK(std::isfinite(i),
              std::string(op) + ": interest_rate must be finite");
  PLUSS_CHECK(i > -1.0, std::string(op) + ": interest_rate must be > -1");
}

Series sanitize_qx_(const Series &qx, std::string_view op) {
  PLUSS_CHECK(qx.defined(), std::string(op) + ": qx must be defined");
  PLUSS_CHECK(qx.dim() == 1, std::string(op) + ": qx must be 1D");
  Series q = qx.to(ScalarType::Float64).contiguous();
  PLUSS_CHECK(q.isfinite().all().item<bool>(),
              std::string(op) + ": qx must be finite");
  const bool in_range = q.ge(0.0).logical_and(q.le(1.0)).all().item<bool>();
  PLUSS_CHECK(in_range, std::string(op) + ": qx values must lie in [0, 1]");
  return q;
}

Series discount_powers_(std::int64_t start, std::int64_t end,
                        double interest_rate, const Series &like) {
  if (end <= start) {
    return Series::empty(std::vector<std::int64_t>{0}, ScalarType::Float64,
                         like.platform());
  }
  const double logv = -std::log1p(interest_rate);
  return Series::arange(start, end, 1, ScalarType::Int64)
      .to(like.platform(), ScalarType::Float64)
      .mul(logv)
      .exp();
}

Series survival_prefix_(const Series &qx_clean) {
  const std::int64_t n = qx_clean.numel();
  const Series one =
      Series::ones(std::vector<std::int64_t>{1}, ScalarType::Float64)
          .to(qx_clean.platform());
  if (n == 0) {
    return one;
  }
  const Series px = qx_clean.mul(-1.0).add(1.0);
  return pluss::series::unite({one, px.cumprod(0)}, /*dim=*/0,
                              /*strict_dtype=*/true);
}

CommutationTable commutation_from_sanitized_(const Series &qx_clean,
                                             double interest_rate,
                                             double radix) {
  const std::int64_t n = qx_clean.numel();
  const auto platform = qx_clean.platform();
  const Series zero1 =
      Series::zeros(std::vector<std::int64_t>{1}, ScalarType::Float64)
          .to(platform);

  CommutationTable out{};
  out.lx = survival_prefix_(qx_clean).mul(radix);
  out.D = out.lx.mul(
      discount_powers_(/*start=*/0, /*end=*/n + 1, interest_rate, qx_clean));

  if (n == 0) {
    out.dx = Series::empty(std::vector<std::int64_t>{0}, ScalarType::Float64,
                           platform);
    out.N = zero1;
    out.C = Series::empty(std::vector<std::int64_t>{0}, ScalarType::Float64,
                          platform);
    out.M = zero1;
    return out;
  }

  const Series lx0 =
      out.lx.slice(/*dim=*/0, /*start=*/0, /*end=*/n, /*step=*/1);
  const Series lx1 =
      out.lx.slice(/*dim=*/0, /*start=*/1, /*end=*/n + 1, /*step=*/1);
  out.dx = lx0.sub(lx1);
  out.C = out.dx.mul(
      discount_powers_(/*start=*/1, /*end=*/n + 1, interest_rate, qx_clean));

  const Series N_head =
      out.D.slice(/*dim=*/0, /*start=*/0, /*end=*/n, /*step=*/1)
          .flip(0)
          .cumsum(0)
          .flip(0);
  out.N =
      pluss::series::unite({N_head, zero1}, /*dim=*/0, /*strict_dtype=*/true);
  const Series M_head = out.C.flip(0).cumsum(0).flip(0);
  out.M =
      pluss::series::unite({M_head, zero1}, /*dim=*/0, /*strict_dtype=*/true);
  return out;
}

} // namespace

Series survival_curve(const Series &qx) {
  return survival_prefix_(sanitize_qx_(qx, "survival_curve"));
}

Series annuity_due(const Series &qx, double interest_rate) {
  const Mortality mortality_basis = mortality::from_qx(qx);
  return operators::value({mortality_basis, discount::flat(interest_rate)})(
      cashflow::annuity(mortality_basis.horizon(), 1.0, PaymentTiming::Start));
}

Series annuity_immediate(const Series &qx, double interest_rate) {
  const Mortality mortality_basis = mortality::from_qx(qx);
  return operators::value({mortality_basis, discount::flat(interest_rate)})(
      cashflow::annuity(mortality_basis.horizon(), 1.0, PaymentTiming::End));
}

Series term_insurance(const Series &qx, double interest_rate) {
  const Mortality mortality_basis = mortality::from_qx(qx);
  return operators::value({mortality_basis, discount::flat(interest_rate)})(
      cashflow::death(mortality_basis.horizon()));
}

Series pure_endowment(const Series &qx, double interest_rate) {
  const Mortality mortality_basis = mortality::from_qx(qx);
  return operators::value({mortality_basis, discount::flat(interest_rate)})(
      cashflow::survival(mortality_basis.horizon()));
}

Series endowment_insurance(const Series &qx, double interest_rate) {
  const Mortality mortality_basis = mortality::from_qx(qx);
  return operators::value({mortality_basis, discount::flat(interest_rate)})(
      cashflow::endowment(mortality_basis.horizon()));
}

Series lx_qx(const Series &qx, double radix) {
  PLUSS_CHECK(std::isfinite(radix), "lx_qx: radix must be finite");
  PLUSS_CHECK(radix > 0.0, "lx_qx: radix must be > 0");
  return survival_prefix_(sanitize_qx_(qx, "lx_qx")).mul(radix);
}

CommutationTable commutation_table(const Series &qx, double interest_rate,
                                   double radix) {
  validate_interest_(interest_rate, "commutation_table");
  PLUSS_CHECK(std::isfinite(radix), "commutation_table: radix must be finite");
  PLUSS_CHECK(radix > 0.0, "commutation_table: radix must be > 0");
  return commutation_from_sanitized_(sanitize_qx_(qx, "commutation_table"),
                                     interest_rate, radix);
}

Series endowment_premium(const Series &qx, double interest_rate) {
  const CommutationTable ct =
      commutation_table(qx, interest_rate, /*radix=*/1.0);
  const Series Dn = ct.D.select(0, ct.D.numel() - 1);
  const Series A = ct.M.add(Dn).div(ct.D);
  const Series a = ct.N.div(ct.D);
  const Series a0 = a.select(0, 0);
  PLUSS_CHECK(a0.item<double>() > 0.0,
              "endowment_premium: denominator must be > 0");
  return A.select(0, 0).div(a0);
}

Series endowment_reserve(const Series &qx, double interest_rate) {
  const CommutationTable ct =
      commutation_table(qx, interest_rate, /*radix=*/1.0);
  const Series Dn = ct.D.select(0, ct.D.numel() - 1);
  const Series A = ct.M.add(Dn).div(ct.D);
  const Series a = ct.N.div(ct.D);
  const Series a0 = a.select(0, 0);
  PLUSS_CHECK(a0.item<double>() > 0.0,
              "endowment_reserve: denominator must be > 0");
  const Series premium = A.select(0, 0).div(a0);
  return A.sub(a.mul(premium));
}

} // namespace pluss::subsp::actuarial
