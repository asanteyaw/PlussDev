#include <pluss/ma.h>

#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

using pluss::Config;
using pluss::ScalarType;
using pluss::Series;
using namespace pluss::subsp::actuarial::life_contingencies;

std::vector<double> as_f64_vec(const Series& x) {
  return x.to(pluss::pCPU, ScalarType::Float64).reshape({x.numel()}).to_vec<double>();
}

double as_f64_scalar(const Series& x) {
  if (!x.defined() || x.numel() != 1) {
    throw std::runtime_error("expected scalar Series");
  }
  return x.to(pluss::pCPU, ScalarType::Float64).item<double>();
}

bool approx(double a, double b, double tol = 1e-10) {
  return std::abs(a - b) <= tol;
}

void expect(bool cond, const std::string& msg, int* failures) {
  if (!cond) {
    ++(*failures);
    std::cerr << "[FAIL] " << msg << '\n';
  }
}

} // namespace

int main() {
  int failures = 0;
  const Config cfg = Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

  const Series qx = pluss::make_series(std::vector<double>{0.1, 0.2}, {2}, cfg);

  {
    const auto px = as_f64_vec(px_from_qx(qx));
    expect(px.size() == 2, "px_from_qx size", &failures);
    expect(approx(px[0], 0.9, 1e-12) && approx(px[1], 0.8, 1e-12), "px_from_qx values", &failures);

    const auto qx_rt = as_f64_vec(qx_from_px(px_from_qx(qx)));
    expect(approx(qx_rt[0], 0.1, 1e-12) && approx(qx_rt[1], 0.2, 1e-12), "qx_from_px roundtrip", &failures);
  }

  {
    const Series ages = pluss::make_series(std::vector<double>{40.0, 41.0, 42.0}, {3}, cfg);
    const double b = 1.0e-4;
    const double c = 1.1;
    const double a = 2.0e-4;
    const auto gom_mu = as_f64_vec(gompertz_force_surface(ages, b, c));
    const auto mak_mu = as_f64_vec(makeham_force_surface(ages, a, b, c));
    expect(gom_mu.size() == 3 && mak_mu.size() == 3, "law surface sizes", &failures);
    for (std::size_t i = 0; i < 3; ++i) {
      const double g_ref = b * std::pow(c, 40.0 + static_cast<double>(i));
      expect(approx(gom_mu[i], g_ref, 1e-12), "gompertz_force_surface values", &failures);
      expect(approx(mak_mu[i], a + g_ref, 1e-12), "makeham_force_surface values", &failures);
    }

    const auto q_from_mu = as_f64_vec(qx_from_force(makeham_force_surface(ages, a, b, c)));
    const auto q_makeham = as_f64_vec(makeham_qx_surface(ages, a, b, c));
    for (std::size_t i = 0; i < q_makeham.size(); ++i) {
      expect(approx(q_makeham[i], q_from_mu[i], 1e-12), "makeham_qx_surface matches qx_from_force", &failures);
    }
  }

  {
    const auto mu = as_f64_vec(force_of_mortality_from_qx(qx));
    const auto H = as_f64_vec(cumulative_hazard_from_qx(qx));
    expect(mu.size() == 2 && H.size() == 2, "hazard vector sizes", &failures);
    expect(approx(mu[0], -std::log(0.9), 1e-12), "mu[0]", &failures);
    expect(approx(mu[1], -std::log(0.8), 1e-12), "mu[1]", &failures);
    expect(approx(H[0], mu[0], 1e-12) && approx(H[1], mu[0] + mu[1], 1e-12), "cumulative hazard", &failures);
  }

  {
    const auto surv = as_f64_vec(survival_from_force(force_of_mortality_from_qx(qx)));
    expect(surv.size() == 3, "survival_from_force size", &failures);
    expect(approx(surv[0], 1.0, 1e-12), "survival[0]", &failures);
    expect(approx(surv[1], 0.9, 1e-12), "survival[1]", &failures);
    expect(approx(surv[2], 0.72, 1e-12), "survival[2]", &failures);

    const double ex = as_f64_scalar(curtate_expectation_of_life(qx));
    expect(approx(ex, 1.62, 1e-12), "curtate_expectation_of_life", &failures);

    const auto pmf = as_f64_vec(future_lifetime_pmf(qx));
    expect(pmf.size() == 3, "future_lifetime_pmf size", &failures);
    expect(approx(pmf[0], 0.1, 1e-12) && approx(pmf[1], 0.18, 1e-12) && approx(pmf[2], 0.72, 1e-12),
           "future_lifetime_pmf values",
           &failures);
    expect(approx(pmf[0] + pmf[1] + pmf[2], 1.0, 1e-12), "future_lifetime_pmf sums to one", &failures);

    const auto ex_curve = as_f64_vec(curtate_expectation_curve(qx));
    expect(ex_curve.size() == 3, "curtate_expectation_curve size", &failures);
    expect(approx(ex_curve[0], 1.62, 1e-12) && approx(ex_curve[1], 0.8, 1e-12) && approx(ex_curve[2], 0.0, 1e-12),
           "curtate_expectation_curve values",
           &failures);
  }

  {
    const Series qx_tab = pluss::make_series(std::vector<double>{0.1, 0.2, 0.3}, {3}, cfg);
    const auto lx = as_f64_vec(lx_from_qx(qx_tab, 1000.0));
    expect(lx.size() == 4, "lx_from_qx size", &failures);
    expect(approx(lx[0], 1000.0, 1e-12) && approx(lx[1], 900.0, 1e-12) && approx(lx[2], 720.0, 1e-12) &&
               approx(lx[3], 504.0, 1e-12),
           "lx_from_qx values",
           &failures);

    const Series lx_s = pluss::make_series(lx, {4}, cfg);
    const auto dx = as_f64_vec(dx_from_lx(lx_s));
    expect(dx.size() == 3, "dx_from_lx size", &failures);
    expect(approx(dx[0], 100.0, 1e-12) && approx(dx[1], 180.0, 1e-12) && approx(dx[2], 216.0, 1e-12),
           "dx_from_lx values",
           &failures);

    const auto px_l = as_f64_vec(px_from_lx(lx_s));
    const auto qx_l = as_f64_vec(qx_from_lx(lx_s));
    expect(approx(px_l[0], 0.9, 1e-12) && approx(px_l[1], 0.8, 1e-12) && approx(px_l[2], 0.7, 1e-12),
           "px_from_lx values",
           &failures);
    expect(approx(qx_l[0], 0.1, 1e-12) && approx(qx_l[1], 0.2, 1e-12) && approx(qx_l[2], 0.3, 1e-12),
           "qx_from_lx values",
           &failures);

    const auto lx_from_px_v = as_f64_vec(lx_from_px(px_from_qx(qx_tab), 1000.0));
    expect(lx_from_px_v.size() == 4, "lx_from_px size", &failures);
    for (std::size_t i = 0; i < lx.size(); ++i) {
      expect(approx(lx_from_px_v[i], lx[i], 1e-12), "lx_from_px matches lx_from_qx", &failures);
    }
  }

  {
    const Series select_q = pluss::make_series(std::vector<double>{0.01, 0.02}, {2}, cfg);
    const Series ultimate_scalar = pluss::make_series(std::vector<double>{0.03}, {1}, cfg);
    const auto q_proj_scalar = as_f64_vec(compose_select_ultimate_qx(select_q, ultimate_scalar, 5));
    expect(q_proj_scalar.size() == 5, "compose_select_ultimate_qx scalar ultimate size", &failures);
    expect(approx(q_proj_scalar[0], 0.01, 1e-12) && approx(q_proj_scalar[1], 0.02, 1e-12) &&
               approx(q_proj_scalar[2], 0.03, 1e-12) && approx(q_proj_scalar[3], 0.03, 1e-12) &&
               approx(q_proj_scalar[4], 0.03, 1e-12),
           "compose_select_ultimate_qx scalar ultimate values",
           &failures);

    const Series ultimate_vec = pluss::make_series(std::vector<double>{0.03, 0.04, 0.05}, {3}, cfg);
    const auto q_proj_vec = as_f64_vec(compose_select_ultimate_qx(select_q, ultimate_vec, 5));
    expect(q_proj_vec.size() == 5, "compose_select_ultimate_qx vector ultimate size", &failures);
    expect(approx(q_proj_vec[0], 0.01, 1e-12) && approx(q_proj_vec[1], 0.02, 1e-12) &&
               approx(q_proj_vec[2], 0.03, 1e-12) && approx(q_proj_vec[3], 0.04, 1e-12) &&
               approx(q_proj_vec[4], 0.05, 1e-12),
           "compose_select_ultimate_qx vector ultimate values",
           &failures);
  }

  {
    bool threw = false;
    try {
      const Series bad = pluss::make_series(std::vector<double>{1.0}, {1}, cfg);
      (void)force_of_mortality_from_qx(bad);
    } catch (const std::exception&) {
      threw = true;
    }
    expect(threw, "force_of_mortality_from_qx should reject qx == 1", &failures);
  }

  {
    bool threw = false;
    try {
      const Series ages_bad = pluss::make_series(std::vector<double>{-1.0, 10.0}, {2}, cfg);
      (void)gompertz_force_surface(ages_bad, 1e-4, 1.08);
    } catch (const std::exception&) {
      threw = true;
    }
    expect(threw, "gompertz_force_surface should reject negative ages", &failures);
  }

  {
    bool threw = false;
    try {
      const Series select_q = pluss::make_series(std::vector<double>{0.01}, {1}, cfg);
      const Series empty = pluss::make_series(std::vector<double>{}, {0}, cfg);
      (void)compose_select_ultimate_qx(select_q, empty, 3);
    } catch (const std::exception&) {
      threw = true;
    }
    expect(threw, "compose_select_ultimate_qx should reject missing required ultimate tail", &failures);
  }

  {
    bool threw = false;
    try {
      const Series bad_lx = pluss::make_series(std::vector<double>{100.0, 105.0, 90.0}, {3}, cfg);
      (void)qx_from_lx(bad_lx);
    } catch (const std::exception&) {
      threw = true;
    }
    expect(threw, "qx_from_lx should reject increasing lx", &failures);
  }

  {
    bool threw = false;
    try {
      const Series bad_lx = pluss::make_series(std::vector<double>{100.0, 0.0, 0.0}, {3}, cfg);
      (void)px_from_lx(bad_lx);
    } catch (const std::exception&) {
      threw = true;
    }
    expect(threw, "px_from_lx should reject zero denominator rows", &failures);
  }

  return failures == 0 ? 0 : 1;
}
