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
namespace actuarial = pluss::ma::actuarial;
using namespace pluss::subsp::actuarial::actuarial_models;

std::vector<double> as_f64_vec(const Series& x) {
  return x.to(pluss::pCPU, ScalarType::Float64).reshape({x.numel()}).host<double>();
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

  {
    const Series exposure = pluss::make_series(std::vector<double>{100.0, 200.0}, {2}, cfg);
    const Series freq = pluss::make_series(std::vector<double>{0.1, 0.2}, {2}, cfg);
    const Series sev = pluss::make_series(std::vector<double>{500.0, 400.0}, {2}, cfg);
    const auto out = as_f64_vec(claim_cost(exposure, freq, sev));
    expect(out.size() == 2, "claim_cost size", &failures);
    expect(approx(out[0], 5000.0, 1e-12) && approx(out[1], 16000.0, 1e-12),
           "claim_cost values",
           &failures);
  }

  {
    const Series losses = pluss::make_series(std::vector<double>{1000.0, 2000.0}, {2}, cfg);
    const Series exposure = pluss::make_series(std::vector<double>{100.0, 100.0}, {2}, cfg);
    const double pp = as_f64_scalar(pure_premium(losses, exposure));
    expect(approx(pp, 15.0, 1e-12), "pure_premium", &failures);
  }

  {
    const Series losses = pluss::make_series(std::vector<double>{300.0, 200.0}, {2}, cfg);
    const Series premium = pluss::make_series(std::vector<double>{1000.0, 1000.0}, {2}, cfg);
    const double lr = as_f64_scalar(loss_ratio(losses, premium));
    expect(approx(lr, 0.25, 1e-12), "loss_ratio", &failures);
  }

  {
    const Series losses = pluss::make_series(std::vector<double>{100.0, 0.0, 60.0}, {3}, cfg);
    const Series exposure = pluss::make_series(std::vector<double>{10.0, 0.0, 20.0}, {3}, cfg);
    const Series premium = pluss::make_series(std::vector<double>{200.0, 0.0, 120.0}, {3}, cfg);
    const auto pp_curve = as_f64_vec(premium_curve(losses, exposure));
    const auto lr_curve = as_f64_vec(ratio_curve(losses, premium));
    expect(pp_curve.size() == 3 && lr_curve.size() == 3, "curve ratios size", &failures);
    expect(approx(pp_curve[0], 10.0, 1e-12) && approx(pp_curve[1], 0.0, 1e-12) && approx(pp_curve[2], 3.0, 1e-12),
           "premium_curve values",
           &failures);
    expect(approx(lr_curve[0], 0.5, 1e-12) && approx(lr_curve[1], 0.0, 1e-12) && approx(lr_curve[2], 0.5, 1e-12),
           "ratio_curve values",
           &failures);
  }

  {
    const Series exposure = pluss::make_series(std::vector<double>{0.0, 50.0, 200.0}, {3}, cfg);
    const Series exp_pp = pluss::make_series(std::vector<double>{12.0, 15.0, 20.0}, {3}, cfg);
    const Series manual_pp = pluss::make_series(std::vector<double>{10.0, 10.0, 10.0}, {3}, cfg);
    const auto z = as_f64_vec(credibility_weight(exposure, 100.0));
    expect(approx(z[0], 0.0, 1e-12) && approx(z[1], 1.0 / 3.0, 1e-12) && approx(z[2], 2.0 / 3.0, 1e-12),
           "credibility_weight values",
           &failures);
    const auto blend = as_f64_vec(credibility_blend(exp_pp, manual_pp, exposure, 100.0));
    expect(approx(blend[0], 10.0, 1e-12) && approx(blend[1], 35.0 / 3.0, 1e-12) && approx(blend[2], 50.0 / 3.0, 1e-12),
           "credibility_blend values",
           &failures);
  }

  {
    const Series pure = pluss::make_series(15.0, cfg);
    const double ind = as_f64_scalar(indicated_premium(pure, 2.0, 0.2, 0.1));
    expect(approx(ind, 17.0 / 0.7, 1e-12), "indicated_premium", &failures);
  }

  {
    const Series current = pluss::make_series(std::vector<double>{100.0, 80.0}, {2}, cfg);
    const Series indicated = pluss::make_series(std::vector<double>{110.0, 76.0}, {2}, cfg);
    const auto chg = as_f64_vec(rate_change(current, indicated));
    expect(approx(chg[0], 0.10, 1e-12) && approx(chg[1], -0.05, 1e-12),
           "rate_change values",
           &failures);
  }

  {
    const Series losses = pluss::make_series(std::vector<double>{100.0, 0.0, 50.0}, {3}, cfg);
    const Series exposure = pluss::make_series(std::vector<double>{10.0, 0.0, 25.0}, {3}, cfg);
    const Series current = pluss::make_series(std::vector<double>{12.0, 8.0, 3.0}, {3}, cfg);
    const auto chg = as_f64_vec(experience_change(losses, exposure, current, 1.0, 0.2, 0.1));
    expect(approx(chg[0], (11.0 / 0.7) / 12.0 - 1.0, 1e-12) &&
               approx(chg[1], (1.0 / 0.7) / 8.0 - 1.0, 1e-12) &&
               approx(chg[2], (3.0 / 0.7) / 3.0 - 1.0, 1e-12),
           "experience_change values",
           &failures);

    const actuarial::Pricing pricing = actuarial::operators::price({
        .fixed_expense = 1.0,
        .variable_expense_ratio = 0.2,
        .profit_ratio = 0.1,
    });
    const actuarial::PricingResult result = pricing({
        .losses = losses,
        .exposure = exposure,
        .current_premium = current,
    });
    expect(pricing.key() == "price", "pricing operator key", &failures);
    expect(result.rate_change.allclose(
               experience_change(
                   losses, exposure, current, 1.0, 0.2, 0.1)),
           "pricing operator parity", &failures);
  }

  {
    const Series reported = pluss::make_series(std::vector<double>{600.0, 300.0}, {2}, cfg);
    const Series expected = pluss::make_series(std::vector<double>{1000.0, 800.0}, {2}, cfg);
    const Series pct = pluss::make_series(std::vector<double>{0.6, 0.3}, {2}, cfg);
    const auto ult = as_f64_vec(bf_ultimate(reported, expected, pct));
    const auto res = as_f64_vec(bf_reserve(expected, pct));
    expect(approx(ult[0], 1000.0, 1e-12) && approx(ult[1], 860.0, 1e-12),
           "bf_ultimate values",
           &failures);
    expect(approx(res[0], 400.0, 1e-12) && approx(res[1], 560.0, 1e-12),
           "bf_reserve values",
           &failures);
  }

  {
    const Series triangle = pluss::make_series(std::vector<double>{
                                                   100.0, 150.0, 180.0,
                                                   120.0, 180.0, 0.0,
                                                   130.0, 0.0, 0.0},
                                               {3, 3},
                                               cfg);
    const auto age_to_age = as_f64_vec(age_factors(triangle));
    expect(age_to_age.size() == 2 &&
               approx(age_to_age[0], 1.5, 1e-12) &&
               approx(age_to_age[1], 1.2, 1e-12),
           "age_factors values",
           &failures);
    const auto cdf = as_f64_vec(ladder_cdf(age_factors(triangle)));
    expect(cdf.size() == 3 &&
               approx(cdf[0], 1.8, 1e-12) &&
               approx(cdf[1], 1.2, 1e-12) &&
               approx(cdf[2], 1.0, 1e-12),
           "ladder_cdf values",
           &failures);
    const Series cdf_s = pluss::make_series(std::vector<double>{1.8, 1.2, 1.0}, {3}, cfg);
    const auto ult = as_f64_vec(ladder_ultimate(triangle, cdf_s));
    const auto res = as_f64_vec(ladder_reserve(triangle, cdf_s));
    expect(ult.size() == 3 &&
               approx(ult[0], 180.0, 1e-12) &&
               approx(ult[1], 216.0, 1e-12) &&
               approx(ult[2], 234.0, 1e-12),
           "ladder_ultimate values",
           &failures);
    expect(res.size() == 3 &&
               approx(res[0], 0.0, 1e-12) &&
               approx(res[1], 36.0, 1e-12) &&
               approx(res[2], 104.0, 1e-12),
           "ladder_reserve values",
           &failures);

    const actuarial::Triangle labeled = actuarial::triangle::cumulative(
        triangle,
        pluss::make_series({{true, true, true},
                            {true, true, false},
                            {true, false, false}}),
        {"2023", "2024", "2025"},
        {"12", "24", "36"});
    const actuarial::Reserving estimator = actuarial::operators::chain_ladder();
    const actuarial::ReserveResult result = estimator(labeled);
    expect(estimator.key() == "chain_ladder", "chain-ladder operator key", &failures);
    expect(result.ultimate.allclose(
               pluss::make_series({180.0, 216.0, 234.0})),
           "chain-ladder operator ultimate", &failures);
    expect(labeled.origins()[1] == "2024" && labeled.development()[2] == "36",
           "triangle labels", &failures);

    const actuarial::Triangle observed_zero = actuarial::triangle::cumulative(
        pluss::make_series({{0.0, 10.0}, {5.0, 0.0}}),
        pluss::make_series({{true, true}, {true, false}}));
    expect(observed_zero.observed().select(0, 0).select(0, 0).item<bool>(),
           "explicit mask preserves an observed zero", &failures);

    // A supplied pattern is reported as the pattern that was used: the
    // age-to-age factors are the ratios of adjacent cumulative factors, not the
    // factors estimated from the triangle.
    const actuarial::ReserveResult selected =
        actuarial::operators::chain_ladder(pluss::make_series({1.8, 1.2, 1.0}))(labeled);
    expect(selected.age_to_age.allclose(pluss::make_series({1.8 / 1.2, 1.2})),
           "supplied cdf implies the reported age-to-age factors", &failures);
    expect(selected.cdf.allclose(pluss::make_series({1.8, 1.2, 1.0})),
           "supplied cdf is reported unchanged", &failures);

    const actuarial::Reserving bf = actuarial::operators::bornhuetter_ferguson(
        pluss::make_series({200.0, 220.0, 250.0}),
        pluss::make_series({0.9, 0.7, 0.5}));
    const actuarial::ReserveResult bf_result = bf(labeled);
    expect(bf_result.reserve.allclose(pluss::make_series({20.0, 66.0, 125.0})),
           "Bornhuetter-Ferguson operator reserve", &failures);
  }

  {
    bool threw = false;
    try {
      const Series pure = pluss::make_series(10.0, cfg);
      (void)indicated_premium(pure, 1.0, 0.6, 0.4);
    } catch (const std::exception&) {
      threw = true;
    }
    expect(threw, "invalid loading ratios should throw", &failures);
  }

  {
    bool threw = false;
    try {
      const Series triangle = pluss::make_series(std::vector<double>{
                                                     100.0, 150.0, 180.0,
                                                     120.0, 180.0, 0.0,
                                                     130.0, 0.0, 0.0},
                                                 {3, 3},
                                                 cfg);
      const Series bad_cdf = pluss::make_series(std::vector<double>{1.8, 1.2, 1.1}, {3}, cfg);
      (void)ladder_ultimate(triangle, bad_cdf);
    } catch (const std::exception&) {
      threw = true;
    }
    expect(threw, "invalid chain_ladder cdf should throw", &failures);
  }

  return failures == 0 ? 0 : 1;
}
