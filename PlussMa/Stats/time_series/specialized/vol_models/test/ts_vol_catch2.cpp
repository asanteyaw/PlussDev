#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <pluss/ma.h>

#include <cmath>
#include <cstdint>

namespace {

using Catch::Approx;
using pluss::Config;
using pluss::ScalarType;
using pluss::Series;
using pluss::subsp::stats::ts::Garch11;
using pluss::subsp::stats::ts::Garch11Fit;
using pluss::subsp::stats::ts::Garch11Options;
using pluss::subsp::stats::ts::Garch;
using pluss::subsp::stats::ts::GarchOptions;
using pluss::subsp::stats::ts::Egarch;
using pluss::subsp::stats::ts::EgarchFit;
using pluss::subsp::stats::ts::EgarchOptions;
using pluss::subsp::stats::ts::Gjr;
using pluss::subsp::stats::ts::GjrFit;
using pluss::subsp::stats::ts::GjrOptions;

Series simulate_truth(std::int64_t n, std::uint64_t seed) {
  const auto cfg = Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);
  const Garch11Fit truth{
      .mean = 0.0,
      .omega = 0.02,
      .alpha = 0.08,
      .beta = 0.90,
      .variance_floor = 1e-12,
      .stationarity_margin = 1e-6,
      .finite_diff_eps = 1e-5,
  };
  return truth.simulate(n, cfg, seed);
}

Series simulate_egarch_truth(std::int64_t n, std::uint64_t seed) {
  const auto cfg = Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);
  const EgarchFit truth{
      .mean = 0.0,
      .omega = -0.15,
      .arch = {0.12},
      .asym = {-0.06},
      .garch = {0.90},
      .variance_floor = 1e-12,
      .stationarity_margin = 1e-6,
      .finite_diff_eps = 1e-5,
  };
  return truth.simulate(n, cfg, seed);
}

Series simulate_gjr_truth(std::int64_t n, std::uint64_t seed) {
  const auto cfg = Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);
  const GjrFit truth{
      .mean = 0.0,
      .omega = 0.02,
      .arch = {0.06},
      .asym = {0.08},
      .garch = {0.86},
      .variance_floor = 1e-12,
      .stationarity_margin = 1e-6,
      .finite_diff_eps = 1e-5,
  };
  return truth.simulate(n, cfg, seed);
}

} // namespace

TEST_CASE("ts vol garch11: parameter recovery and score behavior", "[ts][vol][garch11][accuracy]") {
  const auto cfg = Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);
  const Series y = simulate_truth(6000, 20260323ULL);

  Garch11Options opt;
  opt.max_iterations = 1000;
  opt.max_evaluations = 10000;
  const auto fit = Garch11::fit_mle(y, opt, cfg);
  const Series sc = fit.score(y, cfg);
  const double snorm = sc.square().sum().sqrt().item<double>();

  REQUIRE(fit.alpha >= 0.0);
  REQUIRE(fit.beta >= 0.0);
  REQUIRE(fit.alpha + fit.beta < 1.0);
  REQUIRE(std::isfinite(snorm));
  REQUIRE(snorm < 100.0);
}

TEST_CASE("ts vol garch11: numerics and finite outputs", "[ts][vol][garch11][numerics]") {
  const auto cfg = Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);
  const Series y = simulate_truth(2500, 20260324ULL);
  const auto fit = Garch11::fit_mle(y, {}, cfg);

  const Series inf = fit.infer(y, cfg);
  const Series llobs = fit.loglik_per_obs(y, cfg);
  const Series info = fit.observed_inf(y, cfg);
  const Series f = fit.forecast(20, y, cfg);

  REQUIRE(inf.isfinite().all().item<double>() == Approx(1.0));
  REQUIRE(llobs.isfinite().all().item<double>() == Approx(1.0));
  REQUIRE(info.isfinite().all().item<double>() == Approx(1.0));
  REQUIRE(f.isfinite().all().item<double>() == Approx(1.0));
  REQUIRE(inf.min().item<double>() > 0.0);
  REQUIRE(f.min().item<double>() > 0.0);
}

TEST_CASE("ts vol garch11: stress and crash checks", "[ts][vol][garch11][stress][crash]") {
  const auto cfg = Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);
  const Series y = simulate_truth(30000, 20260325ULL);

  Garch11Options opt;
  opt.max_iterations = 1200;
  opt.max_evaluations = 12000;
  const auto fit = Garch11::fit_mle(y, opt, cfg);

  const Series f = fit.forecast(30, y, cfg);
  REQUIRE(f.sizes()[0] == 30);
  REQUIRE(f.isfinite().all().item<double>() == Approx(1.0));

  const Series ys = fit.simulate(5000, cfg, 20260326ULL);
  REQUIRE(ys.sizes()[0] == 5000);
  REQUIRE(ys.isfinite().all().item<double>() == Approx(1.0));

  Garch11Options bad;
  bad.stationarity_margin = 1.0;
  REQUIRE_THROWS(Garch11::fit_mle(y, bad, cfg));
  REQUIRE_THROWS(fit.forecast(0, y, cfg));
}

TEST_CASE("ts vol garch(p,q): generic fit and diagnostics", "[ts][vol][garchpq][accuracy]") {
  const auto cfg = Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);
  const Garch11Fit seed{
      .mean = 0.0,
      .omega = 0.015,
      .alpha = 0.09,
      .beta = 0.86,
      .variance_floor = 1e-12,
      .stationarity_margin = 1e-6,
      .finite_diff_eps = 1e-5,
  };
  const Series y = seed.simulate(7000, cfg, 20260330ULL);

  GarchOptions opt;
  opt.arch_order = 2;
  opt.garch_order = 1;
  opt.arch0 = {0.06, 0.03};
  opt.garch0 = {0.85};
  opt.max_iterations = 1200;
  opt.max_evaluations = 12000;
  const auto fit = Garch::fit_mle(y, opt, cfg);

  REQUIRE(fit.arch.size() == 2);
  REQUIRE(fit.garch.size() == 1);
  REQUIRE(fit.persistence() < 1.0);
  REQUIRE(fit.infer(y, cfg).isfinite().all().item<double>() == Approx(1.0));
  REQUIRE(fit.observed_inf(y, cfg).isfinite().all().item<double>() == Approx(1.0));
  REQUIRE(fit.forecast(25, y, cfg).isfinite().all().item<double>() == Approx(1.0));
}

TEST_CASE("ts vol egarch(p,q): fit and diagnostics", "[ts][vol][egarch][accuracy]") {
  const auto cfg = Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);
  const Series y = simulate_egarch_truth(5000, 20260427ULL);

  EgarchOptions opt;
  opt.arch_order = 1;
  opt.garch_order = 1;
  opt.max_iterations = 1200;
  opt.max_evaluations = 12000;
  const auto fit = Egarch::fit_mle(y, opt, cfg);

  REQUIRE(fit.arch.size() == 1);
  REQUIRE(fit.asym.size() == 1);
  REQUIRE(fit.garch.size() == 1);
  REQUIRE(fit.persistence() < 1.0);
  REQUIRE(fit.loglik_per_obs(y, cfg).isfinite().all().item<double>() == Approx(1.0));
  REQUIRE(fit.infer(y, cfg).isfinite().all().item<double>() == Approx(1.0));
  REQUIRE(fit.observed_inf(y, cfg).isfinite().all().item<double>() == Approx(1.0));
  REQUIRE(fit.forecast(15, y, cfg).min().item<double>() > 0.0);
}

TEST_CASE("ts vol gjr(p,q): fit and diagnostics", "[ts][vol][gjr][accuracy]") {
  const auto cfg = Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);
  const Series y = simulate_gjr_truth(5000, 20260428ULL);

  GjrOptions opt;
  opt.arch_order = 1;
  opt.garch_order = 1;
  opt.max_iterations = 1200;
  opt.max_evaluations = 12000;
  const auto fit = Gjr::fit_mle(y, opt, cfg);

  REQUIRE(fit.arch.size() == 1);
  REQUIRE(fit.asym.size() == 1);
  REQUIRE(fit.garch.size() == 1);
  REQUIRE(fit.persistence() < 1.0);
  REQUIRE(fit.loglik_per_obs(y, cfg).isfinite().all().item<double>() == Approx(1.0));
  REQUIRE(fit.infer(y, cfg).min().item<double>() > 0.0);
  REQUIRE(fit.observed_inf(y, cfg).isfinite().all().item<double>() == Approx(1.0));
  REQUIRE(fit.forecast(15, y, cfg).min().item<double>() > 0.0);
}

TEST_CASE("ts vol egarch/gjr: crash checks", "[ts][vol][egarch][gjr][crash]") {
  const auto cfg = Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);
  const Series y = pluss::series::randn({600}, cfg, 20260429ULL);

  EgarchOptions bad_e;
  bad_e.stationarity_margin = 1.0;
  REQUIRE_THROWS(Egarch::fit_mle(y, bad_e, cfg));

  GjrOptions bad_g;
  bad_g.stationarity_margin = 1.0;
  REQUIRE_THROWS(Gjr::fit_mle(y, bad_g, cfg));

  const auto efit = Egarch::fit_mle(y, {}, cfg);
  const auto gfit = Gjr::fit_mle(y, {}, cfg);
  REQUIRE_THROWS(efit.forecast(0, y, cfg));
  REQUIRE_THROWS(gfit.forecast(0, y, cfg));
}
