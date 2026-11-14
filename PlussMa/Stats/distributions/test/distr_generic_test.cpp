// distr::Distribution: the one object for every dst::Kind. Evaluation and sampling must equal the kind-based
// functions and the hand-written objects; cumulants, cgf and the fit are numerical and are checked against the
// hand-written objects where those exist.
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

const auto f64 = pluss::Config{}.with_dtype(pluss::ScalarType::Float64);

}  // namespace

int main() {
  const auto x = pluss::make_series({-1.0, 0.0, 0.5, 2.0, 4.0}, f64);
  const auto xp = pluss::make_series({0.2, 0.5, 1.0, 2.5, 6.0}, f64);

  // Evaluation and sampling are the kind-based functions and the hand-written objects, exactly.
  {
    const distr::Distribution g(dst::Normal, {0.5, 1.5});
    const distr::Normal o{0.5, 1.5};
    near(as_f64((g.pdf(x) - o.pdf(x)).abs().max()), 0.0, 1e-15, "Normal pdf equals the object");
    near(as_f64((g.logcdf(x) - o.logcdf(x)).abs().max()), 0.0, 1e-15, "Normal logcdf equals the object");
    near(as_f64((g.sample({6}, f64, 11ULL) - o.sample({6}, f64, 11ULL)).abs().max()), 0.0, 0.0, "Normal draws equal the object's");
    near(as_f64((g.pdf(x) - distr::pdf(dst::Normal, x, {0.5, 1.5})).abs().max()), 0.0, 0.0, "pdf equals the kind-based function");
    const auto probabilities = pluss::make_series({0.025, 0.5, 0.975}, f64);
    const auto quantiles = g.icdf(probabilities);
    near(as_f64((g.cdf(quantiles) - probabilities).abs().max()), 0.0, 2e-10,
         "generic icdf inverts cdf");
    near(as_f64((distr::icdf(dst::Normal, probabilities, {0.5, 1.5}) - quantiles).abs().max()),
         0.0, 0.0, "kind-based icdf equals the object");
    const auto like = pluss::make_series({0.0, 0.0, 0.0}, f64).reshape({3, 1});
    expect(g.sample_like(like, 3ULL).sizes()[0] == 3, "sample_like takes the shape of its argument");
  }

  // Cumulants and cgf against the hand-written objects, continuous and discrete.
  const auto cumulants = [&](const distr::Distribution& g, auto&& o, const char* label, double tol) {
    for (int k = 1; k <= 4; ++k)
      near(as_f64(g.cumulant(k)), as_f64(o.cumulant(k)), tol, std::string(label) + " cumulant " + std::to_string(k));
  };
  cumulants(distr::Distribution(dst::Normal, {0.5, 1.5}), distr::Normal{0.5, 1.5}, "Normal", 1e-8);
  cumulants(distr::Distribution(dst::Gamma, {0.0, 1.5, 2.5}), distr::Gamma{2.5, 1.5, 0.0}, "Gamma", 1e-8);
  cumulants(distr::Distribution(dst::Beta, {0.0, 1.0, 2.0, 5.0}), distr::Beta{0.0, 1.0, 2.0, 5.0}, "Beta", 1e-8);
  cumulants(distr::Distribution(dst::Poisson, {3.5}), distr::Poisson{3.5}, "Poisson", 1e-8);
  cumulants(distr::Distribution(dst::Binomial, {12.0, 0.35}), distr::Binomial{12, 0.35}, "Binomial", 1e-8);
  {
    const auto t = pluss::make_series({-0.3, 0.2, 0.7}, f64);
    near(as_f64((distr::Distribution(dst::Normal, {0.5, 1.5}).cgf(t) - distr::Normal{0.5, 1.5}.cgf(t)).abs().max()), 0.0, 1e-8, "Normal cgf");
    near(as_f64((distr::Distribution(dst::Poisson, {3.5}).cgf(t) - distr::Poisson{3.5}.cgf(t)).abs().max()), 0.0, 1e-8, "Poisson cgf");
    expect(t.sizes() == distr::Distribution(dst::Normal, {0.0, 1.0}).cgf(t).sizes(), "cgf keeps the shape of t");
  }

  // A family with no hand-written object: Gompertz mean from the density, checked against draws.
  {
    const distr::Distribution g(dst::Gompertz, {0.8, 2.0});
    const double mean = as_f64(g.cumulant(1));
    const double sample_mean = as_f64(g.sample({200000}, f64, 5ULL).mean());
    near(sample_mean, mean, 5e-3, "Gompertz mean agrees with the sample mean");
    near(as_f64(g.cdf(pluss::make_series({1.0}, f64)).reshape(std::vector<std::int64_t>{})), as_f64(distr::cdf(dst::Gompertz, pluss::make_series({1.0}, f64), {0.8, 2.0}).reshape(std::vector<std::int64_t>{})), 1e-15, "cdf is the kind-based cdf");
  }

  // Heavy tails: the moment does not exist, so the call refuses instead of returning a truncated number.
  {
    bool refused = false;
    try { (void)distr::Distribution(dst::Cauchy, {0.0, 1.0}).cumulant(1); } catch (const std::exception&) { refused = true; }
    expect(refused, "Cauchy cumulant is refused");
    refused = false;
    try { (void)distr::Distribution(dst::Normal, {0.0, 1.0}).cumulant(7); } catch (const std::exception&) { refused = true; }
    expect(refused, "cumulant order above 6 is refused");
  }

  // Parameters as Series: a different distribution at every point; only the evaluation methods exist.
  {
    const auto mu = pluss::make_series({0.0, 1.0, 2.0}, f64), sd = pluss::make_series({1.0, 2.0, 0.5}, f64);
    const auto pts = pluss::make_series({0.3, 0.3, 0.3}, f64);
    const distr::Distribution g(dst::Normal, std::vector<pluss::Series>{mu, sd});
    const auto got = g.pdf(pts).host<double>().array().data;
    for (std::size_t i = 0; i < 3; ++i) {
      const double m = i == 0 ? 0.0 : (i == 1 ? 1.0 : 2.0), s = i == 0 ? 1.0 : (i == 1 ? 2.0 : 0.5);
      near(got[i], as_f64(distr::Normal{m, s}.pdf(pluss::make_series({0.3}, f64))), 1e-14, "Series parameters give per-point densities");
    }
    bool refused = false;
    try { (void)g.cumulant(1); } catch (const std::exception&) { refused = true; }
    expect(refused, "cumulant is refused for Series parameters");
  }

  // Maximum likelihood against the hand-written fits, and recovery of a family with no hand-written fit.
  {
    const auto data = distr::Normal{1.5, 2.0}.sample({1000}, f64, 3ULL);
    const auto fit = distr::Distribution(dst::Normal, {0.0, 1.0}).MLE(data);
    const auto ref = distr::Normal{0.0, 1.0}.MLE(data);
    expect(fit.converged, "Normal MLE converged");
    near(fit.params[0], ref.mean, 1e-6, "Normal MLE mean");
    near(fit.params[1], ref.stddev, 1e-6, "Normal MLE stddev");
    const auto se = fit.std_err();
    const auto ref_se = ref.std_err(data).host<double>().array().data;
    near(se[0], ref_se[0], 1e-4, "Normal MLE stderr of the mean");
    near(se[1], ref_se[1], 1e-4, "Normal MLE stderr of the stddev");
    const auto ci = fit.confint(0.95);
    near(ci[0].first, fit.params[0] - 1.959964 * se[0], 1e-5, "95% interval lower bound");
    expect(fit.model().kind == dst::Normal, "model() returns the fitted object");

    const auto weib = distr::Distribution(dst::Gompertz, {0.8, 2.0}).sample({4000}, f64, 9ULL);
    const auto gfit = distr::Distribution(dst::Gompertz, {0.5, 1.5}).MLE(weib);
    near(gfit.params[0], 0.8, 0.08, "Gompertz MLE recovers b");
    near(gfit.params[1], 2.0, 0.1, "Gompertz MLE recovers c");
  }

  if (failures) { std::cerr << failures << " check(s) failed\n"; return 1; }
  std::cout << "distr_generic_test passed\n";
  return 0;
}
