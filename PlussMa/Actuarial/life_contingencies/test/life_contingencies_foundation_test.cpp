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
    bool threw = false;
    try {
      const Series bad = pluss::make_series(std::vector<double>{1.0}, {1}, cfg);
      (void)force_of_mortality_from_qx(bad);
    } catch (const std::exception&) {
      threw = true;
    }
    expect(threw, "force_of_mortality_from_qx should reject qx == 1", &failures);
  }

  return failures == 0 ? 0 : 1;
}
