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
using namespace pluss::subsp::actuarial;

double as_f64_scalar(const Series& x) {
  if (!x.defined() || x.numel() != 1) {
    throw std::runtime_error("expected scalar Series");
  }
  return x.to(pluss::pCPU, ScalarType::Float64).item<double>();
}

std::vector<double> as_f64_vec(const Series& x) {
  return x.to(pluss::pCPU, ScalarType::Float64).reshape({x.numel()}).host<double>();
}

void expect(bool cond, const std::string& msg, int* failures) {
  if (!cond) {
    ++(*failures);
    std::cerr << "[FAIL] " << msg << '\n';
  }
}

bool approx(double a, double b, double tol = 1e-10) {
  return std::abs(a - b) <= tol;
}

} // namespace

int main() {
  int failures = 0;
  const Config cfg = Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

  {
    const Series q = pluss::make_series(std::vector<double>{0.0, 0.0, 0.0}, {3}, cfg);
    const auto s = as_f64_vec(survival_curve(q));
    expect(s.size() == 4, "survival_curve size for n=3", &failures);
    expect(approx(s[0], 1.0) && approx(s[1], 1.0) && approx(s[2], 1.0) && approx(s[3], 1.0),
           "survival_curve all ones when qx=0",
           &failures);

    const double i = 0.05;
    const double v = 1.0 / (1.0 + i);
    expect(approx(as_f64_scalar(annuity_due(q, i)), 1.0 + v + v * v, 1e-12), "annuity_due (qx=0)", &failures);
    expect(approx(as_f64_scalar(annuity_immediate(q, i)), v + v * v + v * v * v, 1e-12),
           "annuity_immediate (qx=0)",
           &failures);
    expect(approx(as_f64_scalar(term_insurance(q, i)), 0.0, 1e-12), "term_insurance (qx=0)", &failures);
    expect(approx(as_f64_scalar(pure_endowment(q, i)), v * v * v, 1e-12), "pure_endowment (qx=0)", &failures);
    expect(approx(as_f64_scalar(endowment_insurance(q, i)), v * v * v, 1e-12), "endowment_insurance (qx=0)", &failures);
  }

  {
    const Series q = pluss::make_series(std::vector<double>{1.0, 1.0, 1.0}, {3}, cfg);
    const double i = 0.05;
    const double v = 1.0 / (1.0 + i);
    expect(approx(as_f64_scalar(annuity_due(q, i)), 1.0, 1e-12), "annuity_due (qx=1)", &failures);
    expect(approx(as_f64_scalar(annuity_immediate(q, i)), 0.0, 1e-12), "annuity_immediate (qx=1)", &failures);
    expect(approx(as_f64_scalar(term_insurance(q, i)), v, 1e-12), "term_insurance (qx=1)", &failures);
    expect(approx(as_f64_scalar(pure_endowment(q, i)), 0.0, 1e-12), "pure_endowment (qx=1)", &failures);
  }

  {
    const Series q = pluss::make_series(std::vector<double>{0.2, 0.1}, {2}, cfg);
    expect(approx(as_f64_scalar(annuity_due(q, 0.0)), 1.8, 1e-12), "annuity_due (i=0)", &failures);
    expect(approx(as_f64_scalar(annuity_immediate(q, 0.0)), 1.52, 1e-12), "annuity_immediate (i=0)", &failures);
    expect(approx(as_f64_scalar(term_insurance(q, 0.0)), 0.28, 1e-12), "term_insurance (i=0)", &failures);
    expect(approx(as_f64_scalar(pure_endowment(q, 0.0)), 0.72, 1e-12), "pure_endowment (i=0)", &failures);
    expect(approx(as_f64_scalar(endowment_insurance(q, 0.0)), 1.0, 1e-12), "endowment_insurance (i=0)", &failures);

    const auto lx = as_f64_vec(lx_from_qx(q, 100000.0));
    expect(lx.size() == 3, "lx_from_qx output size", &failures);
    expect(approx(lx[0], 100000.0, 1e-9) && approx(lx[1], 80000.0, 1e-9) && approx(lx[2], 72000.0, 1e-9),
           "lx_from_qx values",
           &failures);

    const auto ct = commutation_table(q, 0.0, 100000.0);
    const auto d = as_f64_vec(ct.dx);
    const auto D = as_f64_vec(ct.D);
    const auto N = as_f64_vec(ct.N);
    const auto C = as_f64_vec(ct.C);
    const auto M = as_f64_vec(ct.M);
    expect(d.size() == 2 && approx(d[0], 20000.0, 1e-9) && approx(d[1], 8000.0, 1e-9),
           "commutation dx",
           &failures);
    expect(D.size() == 3 && approx(D[0], 100000.0, 1e-9) && approx(D[1], 80000.0, 1e-9) &&
               approx(D[2], 72000.0, 1e-9),
           "commutation D",
           &failures);
    expect(N.size() == 3 && approx(N[0], 180000.0, 1e-9) && approx(N[1], 80000.0, 1e-9) &&
               approx(N[2], 0.0, 1e-9),
           "commutation N",
           &failures);
    expect(C.size() == 2 && approx(C[0], 20000.0, 1e-9) && approx(C[1], 8000.0, 1e-9),
           "commutation C",
           &failures);
    expect(M.size() == 3 && approx(M[0], 28000.0, 1e-9) && approx(M[1], 8000.0, 1e-9) &&
               approx(M[2], 0.0, 1e-9),
           "commutation M",
           &failures);

    const double p = as_f64_scalar(net_level_premium_endowment(q, 0.0));
    expect(approx(p, 1.0 / 1.8, 1e-12), "net_level_premium_endowment", &failures);
    const auto rv = as_f64_vec(reserve_curve_endowment(q, 0.0));
    expect(rv.size() == 3, "reserve_curve_endowment size", &failures);
    expect(approx(rv[0], 0.0, 1e-12) && approx(rv[1], 1.0 - p, 1e-12) && approx(rv[2], 1.0, 1e-12),
           "reserve_curve_endowment values",
           &failures);
  }

  {
    bool threw = false;
    try {
      const Series bad = pluss::make_series(std::vector<double>{-0.1, 0.2}, {2}, cfg);
      (void)survival_curve(bad);
    } catch (const std::exception&) {
      threw = true;
    }
    expect(threw, "qx below 0 should throw", &failures);
  }

  {
    bool threw = false;
    try {
      const Series bad = pluss::make_series(std::vector<double>{1.2, 0.2}, {2}, cfg);
      (void)annuity_due(bad, 0.05);
    } catch (const std::exception&) {
      threw = true;
    }
    expect(threw, "qx above 1 should throw", &failures);
  }

  return failures == 0 ? 0 : 1;
}
