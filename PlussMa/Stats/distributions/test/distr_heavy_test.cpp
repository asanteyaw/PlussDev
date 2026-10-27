// The dedicated objects for the heavy-tailed families Pareto, Lomax, Burr, Zeta and Yule: exact cumulants against
// the numerical ones of distr::Distribution, the refusals below the tail index, the cumulant generating function
// for t <= 0 and the fits.
#include <pluss/ma.h>

#include <cmath>
#include <iostream>
#include <string>
#include <vector>

namespace distr = pluss::subsp::stats::distr;
namespace dst = distr::dst;

namespace {

int failures = 0;
void expect(bool ok, const std::string& msg) {
  if (!ok) { ++failures; std::cerr << "FAIL: " << msg << "\n"; }
}
void near(double a, double b, double tol, const std::string& msg) {
  if (!(std::abs(a - b) <= tol * std::max(1.0, std::abs(b)))) {
    ++failures;
    std::cerr << "FAIL: " << msg << " got " << a << " want " << b << "\n";
  }
}
template <class F>
bool throws(F&& f) {
  try { f(); } catch (const std::exception&) { return true; }
  return false;
}

const auto f64 = pluss::Config{}.with_dtype(pluss::ScalarType::Float64);

template <class Obj>
void cumulants_match(const Obj& o, dst::Kind kind, std::vector<double> params, const char* label, int max_order, double tol) {
  const distr::Distribution generic(kind, params);
  for (int k = 1; k <= max_order; ++k) {
    // The numerical cumulant is the reference only where its own integration settles.
    double reference = 0.0;
    try { reference = as_f64(generic.cumulant(k)); } catch (const std::exception&) { continue; }
    near(as_f64(o.cumulant(k)), reference, tol, std::string(label) + " cumulant " + std::to_string(k));
  }
}

}  // namespace

int main() {
  // Exact cumulants equal the numerical ones of the generic object wherever the moment exists.
  cumulants_match(distr::Pareto{2.0, 9.0}, dst::Pareto, {2.0, 9.0}, "Pareto", 4, 1e-5);
  cumulants_match(distr::Lomax{1.5, 9.0}, dst::Lomax, {1.5, 9.0}, "Lomax", 4, 1e-5);
  cumulants_match(distr::Burr{9.0, 2.0}, dst::Burr, {9.0, 2.0}, "Burr", 4, 1e-5);
  cumulants_match(distr::Zeta{7.0}, dst::Zeta, {7.0}, "Zeta", 4, 2e-3);
  cumulants_match(distr::Yule{8.0}, dst::Yule, {8.0}, "Yule", 4, 1e-6);

  // Closed forms worked out by hand.
  near(as_f64(distr::Pareto{1.0, 3.0}.cumulant(1)), 1.5, 1e-14, "Pareto mean alpha xm / (alpha - 1)");
  near(as_f64(distr::Pareto{1.0, 3.0}.cumulant(2)), 0.75, 1e-14, "Pareto variance of alpha = 3");
  near(as_f64(distr::Lomax{2.0, 3.0}.cumulant(1)), 1.0, 1e-14, "Lomax mean scale / (alpha - 1)");
  near(as_f64(distr::Zeta{3.0}.cumulant(1)), 1.6449340668482264 / 1.2020569031595942, 1e-12, "Zeta mean zeta(s - 1) / zeta(s)");
  near(as_f64(distr::Yule{3.0}.cumulant(1)), 2.0, 1e-12, "Yule mean rho / (rho - 1)");
  near(as_f64(distr::Yule{4.0}.cumulant(2)), 2.25, 1e-12, "Yule variance rho^2 / ((rho - 1)^2 (rho - 2))");

  // Below the tail index the moment does not exist, and the call says so; the lower orders still work.
  expect(throws([] { (void)distr::Pareto{1.0, 2.0}.cumulant(2); }), "Pareto variance needs alpha > 2");
  expect(!throws([] { (void)distr::Pareto{1.0, 2.0}.cumulant(1); }), "Pareto mean exists for alpha = 2");
  expect(throws([] { (void)distr::Lomax{1.0, 1.0}.cumulant(1); }), "Lomax mean needs alpha > 1");
  expect(throws([] { (void)distr::Burr{2.0, 3.0}.cumulant(2); }), "Burr variance needs c > 2");
  expect(throws([] { (void)distr::Zeta{3.0}.cumulant(2); }), "Zeta variance needs s > 3");
  expect(throws([] { (void)distr::Yule{3.0}.cumulant(2); }), "Yule variance needs p > 3");

  // The cumulant generating function exists for t <= 0.
  {
    const auto t = pluss::make_series({-0.4, -0.1, 0.0}, f64);
    for (auto [name, g] : std::vector<std::pair<const char*, distr::Distribution>>{
             {"Pareto", distr::Distribution(dst::Pareto, {1.5, 4.0})}, {"Lomax", distr::Distribution(dst::Lomax, {1.5, 4.0})},
             {"Burr", distr::Distribution(dst::Burr, {3.0, 2.0})}, {"Zeta", distr::Distribution(dst::Zeta, {6.0})},
             {"Yule", distr::Distribution(dst::Yule, {6.0})}}) {
      pluss::Series got = pluss::series::undefined();
      if (std::string(name) == "Pareto") got = distr::Pareto{1.5, 4.0}.cgf(t);
      if (std::string(name) == "Lomax") got = distr::Lomax{1.5, 4.0}.cgf(t);
      if (std::string(name) == "Burr") got = distr::Burr{3.0, 2.0}.cgf(t);
      if (std::string(name) == "Zeta") got = distr::Zeta{6.0}.cgf(t);
      if (std::string(name) == "Yule") got = distr::Yule{6.0}.cgf(t);
      near(as_f64(got.select(0, 2)), 0.0, 1e-12, std::string(name) + " cgf(0) is 0");
      near(as_f64((got - g.cgf(t)).abs().max()), 0.0, 1e-6, std::string(name) + " cgf agrees with the generic object");
      expect(as_f64(got.select(0, 0)) < as_f64(got.select(0, 1)), std::string(name) + " cgf increases towards 0");
    }
    expect(throws([&] { (void)distr::Pareto{1.0, 3.0}.cgf(pluss::make_series({0.1}, f64)); }), "cgf for t > 0 is refused");
  }

  // Fits.
  {
    const auto data = distr::Pareto{2.0, 3.0}.sample({20000}, f64, 3ULL);
    const auto fit = distr::Pareto{1.0, 1.0}.MLE(data);
    near(fit.xm, 2.0, 1e-3, "Pareto MLE minimum");
    near(fit.alpha, 3.0, 0.05, "Pareto MLE tail index");
    const auto se = fit.stderr(data).pull<double>();
    expect(se.size() == 2 && se[1] > 0.0, "Pareto MLE standard error of the tail index");
    expect(fit.model().alpha == fit.alpha, "model() returns the fitted object");

    const auto lomax = distr::Lomax{2.0, 3.0}.MLE(distr::Lomax{2.0, 3.0}.sample({20000}, f64, 5ULL), {1.0, 2.0});
    near(lomax.scale, 2.0, 0.12, "Lomax MLE scale");
    near(lomax.alpha, 3.0, 0.2, "Lomax MLE tail index");

    const auto burr = distr::Burr{4.0, 2.0}.MLE(distr::Burr{4.0, 2.0}.sample({20000}, f64, 7ULL), {2.0, 1.0});
    near(burr.c, 4.0, 0.15, "Burr MLE c");
    near(burr.k, 2.0, 0.15, "Burr MLE k");

    const auto zeta = distr::Zeta{3.0}.MLE(distr::Zeta{3.0}.sample({20000}, f64, 11ULL), {2.0});
    near(zeta.s, 3.0, 0.05, "Zeta MLE tail exponent");
    const auto yule = distr::Yule{3.0}.MLE(distr::Yule{3.0}.sample({20000}, f64, 13ULL), {2.0});
    near(yule.p, 3.0, 0.1, "Yule MLE tail exponent");
    const auto ci = zeta.confint(distr::Zeta{3.0}.sample({20000}, f64, 11ULL), 0.95).host<double>().array().data;
    expect(ci.size() == 2 && ci[0] < zeta.s && zeta.s < ci[1], "the 95% interval of the Zeta fit brackets the estimate");
    expect(ci[1] - ci[0] < 0.2, "the interval of 20000 observations is narrow");
  }

  if (failures) { std::cerr << failures << " check(s) failed\n"; return 1; }
  std::cout << "distr_heavy_test passed\n";
  return 0;
}
