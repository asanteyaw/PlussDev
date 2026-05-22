#include "pluss/subsp/actuarial/life_contingencies.h"

#include "pluss/codem/series_factory.h"
#include "pluss/subsp/actuarial/core.h"

#include <algorithm>
#include <cstdint>
#include <cmath>
#include <string_view>
#include <vector>

namespace pluss::subsp::actuarial::life_contingencies {
namespace {

using pluss::codem::ScalarType;
using pluss::codem::Series;

Series sanitize_prob_vector_(const Series& probs, std::string_view op, std::string_view arg_name) {
  PLUSS_CHECK(probs.defined(), std::string(op) + ": " + std::string(arg_name) + " must be defined");
  PLUSS_CHECK(probs.dim() == 1, std::string(op) + ": " + std::string(arg_name) + " must be 1D");
  const Series p = probs.to(ScalarType::Float64).contiguous();
  PLUSS_CHECK(p.isfinite().all().item<bool>(),
              std::string(op) + ": " + std::string(arg_name) + " must be finite");
  PLUSS_CHECK(p.ge(0.0).logical_and(p.le(1.0)).all().item<bool>(),
              std::string(op) + ": " + std::string(arg_name) + " values must lie in [0,1]");
  return p;
}

Series sanitize_qx_(const Series& qx, std::string_view op, std::string_view arg_name = "qx") {
  // Reuse core actuarial validation + survival contract for consistent qx semantics.
  (void)pluss::subsp::actuarial::survival_curve(qx);
  return sanitize_prob_vector_(qx, op, arg_name);
}

Series sanitize_force_(const Series& force, std::string_view op, std::string_view arg_name = "force") {
  PLUSS_CHECK(force.defined(), std::string(op) + ": " + std::string(arg_name) + " must be defined");
  PLUSS_CHECK(force.dim() == 1, std::string(op) + ": " + std::string(arg_name) + " must be 1D");
  const Series mu = force.to(ScalarType::Float64).contiguous();
  PLUSS_CHECK(mu.isfinite().all().item<bool>(),
              std::string(op) + ": " + std::string(arg_name) + " must be finite");
  PLUSS_CHECK(mu.ge(0.0).all().item<bool>(),
              std::string(op) + ": " + std::string(arg_name) + " must be >= 0");
  return mu;
}

Series sanitize_ages_(const Series& attained_ages, std::string_view op) {
  PLUSS_CHECK(attained_ages.defined(), std::string(op) + ": attained_ages must be defined");
  PLUSS_CHECK(attained_ages.dim() == 1, std::string(op) + ": attained_ages must be 1D");
  const Series ages = attained_ages.to(ScalarType::Float64).contiguous();
  PLUSS_CHECK(ages.isfinite().all().item<bool>(), std::string(op) + ": attained_ages must be finite");
  PLUSS_CHECK(ages.ge(0.0).all().item<bool>(), std::string(op) + ": attained_ages must be >= 0");
  return ages;
}

void validate_radix_(double radix, std::string_view op) {
  PLUSS_CHECK(std::isfinite(radix), std::string(op) + ": radix must be finite");
  PLUSS_CHECK(radix > 0.0, std::string(op) + ": radix must be > 0");
}

void validate_gompertz_params_(double b, double c, std::string_view op) {
  PLUSS_CHECK(std::isfinite(b), std::string(op) + ": b must be finite");
  PLUSS_CHECK(std::isfinite(c), std::string(op) + ": c must be finite");
  PLUSS_CHECK(b >= 0.0, std::string(op) + ": b must be >= 0");
  PLUSS_CHECK(c > 1.0, std::string(op) + ": c must be > 1 for Gompertz-style age acceleration");
}

void validate_makeham_params_(double a, double b, double c, std::string_view op) {
  PLUSS_CHECK(std::isfinite(a), std::string(op) + ": a must be finite");
  PLUSS_CHECK(a >= 0.0, std::string(op) + ": a must be >= 0");
  validate_gompertz_params_(b, c, op);
}

Series sanitize_lx_(const Series& lx, std::string_view op) {
  PLUSS_CHECK(lx.defined(), std::string(op) + ": lx must be defined");
  PLUSS_CHECK(lx.dim() == 1, std::string(op) + ": lx must be 1D");
  const Series l = lx.to(ScalarType::Float64).contiguous();
  PLUSS_CHECK(l.numel() >= 1, std::string(op) + ": lx must contain at least one entry");
  PLUSS_CHECK(l.isfinite().all().item<bool>(), std::string(op) + ": lx must be finite");
  PLUSS_CHECK(l.ge(0.0).all().item<bool>(), std::string(op) + ": lx must be >= 0");
  PLUSS_CHECK(l.select(/*dim=*/0, /*index=*/0).item<double>() > 0.0,
              std::string(op) + ": lx[0] must be > 0");
  if (l.numel() > 1) {
    const Series head = l.slice(/*dim=*/0, /*start=*/0, /*end=*/l.numel() - 1, /*step=*/1);
    const Series tail = l.slice(/*dim=*/0, /*start=*/1, /*end=*/l.numel(), /*step=*/1);
    PLUSS_CHECK(head.ge(tail).all().item<bool>(), std::string(op) + ": lx must be non-increasing");
  }
  return l;
}

Series deterministic_px_from_lx_(const Series& lx, std::string_view op) {
  const Series l = sanitize_lx_(lx, op);
  if (l.numel() <= 1) {
    return Series::empty(std::vector<std::int64_t>{0}, ScalarType::Float64, l.platform());
  }
  const Series head = l.slice(/*dim=*/0, /*start=*/0, /*end=*/l.numel() - 1, /*step=*/1);
  const Series tail = l.slice(/*dim=*/0, /*start=*/1, /*end=*/l.numel(), /*step=*/1);
  PLUSS_CHECK(head.gt(0.0).all().item<bool>(),
              std::string(op) + ": lx[:-1] must be > 0 to derive deterministic ratios");
  const Series px = tail.div(head);
  PLUSS_CHECK(px.ge(0.0).logical_and(px.le(1.0)).all().item<bool>(),
              std::string(op) + ": derived px must lie in [0,1]");
  return px;
}

} // namespace

Series gompertz_force_surface(const Series& attained_ages, double b, double c) {
  validate_gompertz_params_(b, c, "gompertz_force_surface");
  const Series ages = sanitize_ages_(attained_ages, "gompertz_force_surface");
  if (ages.numel() == 0 || b == 0.0) {
    return Series::zeros(ages.sizes(), ScalarType::Float64).to(ages.platform());
  }
  const Series mu = ages.mul(std::log(c)).exp().mul(b);
  PLUSS_CHECK(mu.isfinite().all().item<bool>(),
              "gompertz_force_surface: non-finite hazard generated; check age range and parameters");
  return mu;
}

Series gompertz_qx_surface(const Series& attained_ages, double b, double c) {
  return qx_from_force(gompertz_force_surface(attained_ages, b, c));
}

Series makeham_force_surface(const Series& attained_ages, double a, double b, double c) {
  validate_makeham_params_(a, b, c, "makeham_force_surface");
  const Series gompertz = gompertz_force_surface(attained_ages, b, c);
  const Series mu = gompertz.add(a);
  PLUSS_CHECK(mu.isfinite().all().item<bool>(),
              "makeham_force_surface: non-finite hazard generated; check age range and parameters");
  return mu;
}

Series makeham_qx_surface(const Series& attained_ages, double a, double b, double c) {
  return qx_from_force(makeham_force_surface(attained_ages, a, b, c));
}

Series px_from_qx(const Series& qx) {
  const Series q = sanitize_qx_(qx, "px_from_qx");
  return q.mul(-1.0).add(1.0);
}

Series qx_from_px(const Series& px) {
  const Series p = sanitize_prob_vector_(px, "qx_from_px", "px");
  return p.mul(-1.0).add(1.0);
}

Series qx_from_force(const Series& force) {
  const Series mu = sanitize_force_(force, "qx_from_force");
  return mu.mul(-1.0).exp().mul(-1.0).add(1.0);
}

Series force_of_mortality_from_qx(const Series& qx) {
  const Series q = sanitize_qx_(qx, "force_of_mortality_from_qx");
  PLUSS_CHECK(q.lt(1.0).all().item<bool>(), "force_of_mortality_from_qx: qx must lie in [0,1)");
  return q.mul(-1.0).add(1.0).log().mul(-1.0);
}

Series cumulative_hazard_from_qx(const Series& qx) {
  return force_of_mortality_from_qx(qx).cumsum(0);
}

Series survival_from_force(const Series& force) {
  Series mu = sanitize_force_(force, "survival_from_force");

  const Series one = Series::ones(std::vector<std::int64_t>{1}, ScalarType::Float64).to(mu.platform());
  if (mu.numel() == 0) {
    return one;
  }
  const Series surv_tail = mu.cumsum(0).mul(-1.0).exp();
  return pluss::unite({one, surv_tail}, 0, true);
}

Series future_lifetime_pmf(const Series& qx) {
  const Series q = sanitize_qx_(qx, "future_lifetime_pmf");
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
  const Series q = sanitize_qx_(qx, "curtate_expectation_of_life");
  const Series surv = pluss::subsp::actuarial::survival_curve(q);
  if (q.numel() == 0) {
    return Series::zeros(std::vector<std::int64_t>{1}, ScalarType::Float64).to(surv.platform());
  }
  return surv.slice(/*dim=*/0, /*start=*/1, /*end=*/surv.numel(), /*step=*/1).sum();
}

Series curtate_expectation_curve(const Series& qx) {
  const Series q = sanitize_qx_(qx, "curtate_expectation_curve");
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

Series lx_from_qx(const Series& qx, double radix) {
  validate_radix_(radix, "lx_from_qx");
  return pluss::subsp::actuarial::lx_from_qx(sanitize_qx_(qx, "lx_from_qx"), radix);
}

Series lx_from_px(const Series& px, double radix) {
  validate_radix_(radix, "lx_from_px");
  const Series p = sanitize_prob_vector_(px, "lx_from_px", "px");
  return lx_from_qx(p.mul(-1.0).add(1.0), radix);
}

Series dx_from_lx(const Series& lx) {
  const Series l = sanitize_lx_(lx, "dx_from_lx");
  if (l.numel() <= 1) {
    return Series::empty(std::vector<std::int64_t>{0}, ScalarType::Float64, l.platform());
  }
  const Series head = l.slice(/*dim=*/0, /*start=*/0, /*end=*/l.numel() - 1, /*step=*/1);
  const Series tail = l.slice(/*dim=*/0, /*start=*/1, /*end=*/l.numel(), /*step=*/1);
  return head.sub(tail);
}

Series px_from_lx(const Series& lx) {
  return deterministic_px_from_lx_(lx, "px_from_lx");
}

Series qx_from_lx(const Series& lx) {
  return deterministic_px_from_lx_(lx, "qx_from_lx").mul(-1.0).add(1.0);
}

Series compose_select_ultimate_qx(const Series& select_qx,
                                  const Series& ultimate_qx,
                                  std::int64_t projection_years) {
  PLUSS_CHECK(projection_years >= 0,
              "compose_select_ultimate_qx: projection_years must be >= 0");
  const Series q_select = sanitize_qx_(select_qx, "compose_select_ultimate_qx", "select_qx");
  if (projection_years == 0) {
    return Series::empty(std::vector<std::int64_t>{0}, ScalarType::Float64, q_select.platform());
  }

  const std::int64_t select_take = std::min<std::int64_t>(projection_years, q_select.numel());
  const std::int64_t tail_need = projection_years - select_take;
  const Series select_part = q_select.slice(/*dim=*/0, /*start=*/0, /*end=*/select_take, /*step=*/1);
  if (tail_need == 0) {
    return select_part;
  }

  const Series q_ultimate = sanitize_qx_(ultimate_qx, "compose_select_ultimate_qx", "ultimate_qx");
  PLUSS_CHECK(q_ultimate.numel() > 0,
              "compose_select_ultimate_qx: ultimate_qx must be non-empty when projection exceeds select horizon");

  Series tail_part = Series::undefined();
  if (q_ultimate.numel() == 1) {
    const double tail_q = q_ultimate.select(/*dim=*/0, /*index=*/0).item<double>();
    tail_part = Series::ones(std::vector<std::int64_t>{tail_need}, ScalarType::Float64).to(q_ultimate.platform()).mul(
        tail_q);
  } else {
    PLUSS_CHECK(q_ultimate.numel() >= tail_need,
                "compose_select_ultimate_qx: ultimate_qx length must be >= required tail horizon or equal to 1");
    tail_part = q_ultimate.slice(/*dim=*/0, /*start=*/0, /*end=*/tail_need, /*step=*/1);
  }

  if (select_take == 0) {
    return tail_part;
  }
  return pluss::unite({select_part, tail_part.to(select_part.platform())}, /*dim=*/0, /*strict_dtype=*/true);
}

} // namespace pluss::subsp::actuarial::life_contingencies
