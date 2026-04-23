#include "pluss/subsp/actuarial/life_contingencies.h"

#include "pluss/codem/series_factory.h"
#include "pluss/subsp/actuarial/core.h"

#include <cstdint>
#include <cmath>
#include <vector>

namespace pluss::subsp::actuarial::life_contingencies {
namespace {

using pluss::codem::ScalarType;
using pluss::codem::Series;

Series validated_qx_(const Series& qx) {
  // Reuse core actuarial validation + survival contract.
  (void)pluss::subsp::actuarial::survival_curve(qx);
  return qx.to(ScalarType::Float64).contiguous();
}

} // namespace

Series px_from_qx(const Series& qx) {
  const Series q = validated_qx_(qx);
  return q.mul(-1.0).add(1.0);
}

Series qx_from_px(const Series& px) {
  PLUSS_CHECK(px.defined(), "qx_from_px: px must be defined");
  PLUSS_CHECK(px.dim() == 1, "qx_from_px: px must be 1D");
  const Series p = px.to(ScalarType::Float64).contiguous();
  PLUSS_CHECK(p.isfinite().all().item<bool>(), "qx_from_px: px must be finite");
  PLUSS_CHECK(p.ge(0.0).logical_and(p.le(1.0)).all().item<bool>(), "qx_from_px: px must lie in [0,1]");
  return p.mul(-1.0).add(1.0);
}

Series force_of_mortality_from_qx(const Series& qx) {
  const Series q = validated_qx_(qx);
  PLUSS_CHECK(q.lt(1.0).all().item<bool>(), "force_of_mortality_from_qx: qx must lie in [0,1)");
  return q.mul(-1.0).add(1.0).log().mul(-1.0);
}

Series cumulative_hazard_from_qx(const Series& qx) {
  return force_of_mortality_from_qx(qx).cumsum(0);
}

Series survival_from_force(const Series& force) {
  PLUSS_CHECK(force.defined(), "survival_from_force: force must be defined");
  PLUSS_CHECK(force.dim() == 1, "survival_from_force: force must be 1D");
  Series mu = force.to(ScalarType::Float64).contiguous();
  PLUSS_CHECK(mu.isfinite().all().item<bool>(), "survival_from_force: force must be finite");
  PLUSS_CHECK(mu.ge(0.0).all().item<bool>(), "survival_from_force: force must be >= 0");

  const Series one = Series::ones(std::vector<std::int64_t>{1}, ScalarType::Float64).to(mu.platform());
  if (mu.numel() == 0) {
    return one;
  }
  const Series surv_tail = mu.cumsum(0).mul(-1.0).exp();
  return pluss::unite({one, surv_tail}, 0, true);
}

Series future_lifetime_pmf(const Series& qx) {
  const Series q = validated_qx_(qx);
  const Series surv = pluss::subsp::actuarial::survival_curve(q);
  const std::int64_t n = q.numel();
  if (n == 0) {
    return surv;
  }
  const Series head = surv.slice(/*dim=*/0, /*start=*/0, /*end=*/n, /*step=*/1).mul(q);
  const Series tail = surv.select(/*dim=*/0, /*index=*/n).reshape({1});
  return pluss::unite({head, tail}, 0, true);
}

Series curtate_expectation_of_life(const Series& qx) {
  const Series q = validated_qx_(qx);
  const Series surv = pluss::subsp::actuarial::survival_curve(q);
  if (q.numel() == 0) {
    return Series::zeros(std::vector<std::int64_t>{1}, ScalarType::Float64).to(surv.platform());
  }
  return surv.slice(/*dim=*/0, /*start=*/1, /*end=*/surv.numel(), /*step=*/1).sum();
}

Series curtate_expectation_curve(const Series& qx) {
  const Series q = validated_qx_(qx);
  const Series surv = pluss::subsp::actuarial::survival_curve(q);
  const std::int64_t n = q.numel();
  const Series zero1 = Series::zeros(std::vector<std::int64_t>{1}, ScalarType::Float64).to(surv.platform());
  if (n == 0) {
    return zero1;
  }

  const Series tail_sum = surv.flip(0).cumsum(0).flip(0);
  const Series lx = surv.slice(/*dim=*/0, /*start=*/0, /*end=*/n, /*step=*/1);
  const Series future_mass = tail_sum.slice(/*dim=*/0, /*start=*/1, /*end=*/n + 1, /*step=*/1);
  const Series mask = lx.gt(0.0).to(ScalarType::Float64);
  const Series e_head = future_mass.div(lx.maximum(1e-30)).mul(mask);
  return pluss::unite({e_head, zero1}, 0, true);
}

} // namespace pluss::subsp::actuarial::life_contingencies
