#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <pluss/ma.h>

#include <cmath>
#include <cstdint>
#include <limits>
#include <vector>

namespace {

using Catch::Approx;
using pluss::Config;
using pluss::ScalarType;
using pluss::Series;
using pluss::subsp::stats::ts::Ssm;
using pluss::subsp::stats::ts::SsmModel;
using pluss::subsp::stats::ts::SsmOptions;

constexpr double kPi = 3.14159265358979323846;

template <class T>
Series make_series_cfg(const std::vector<T>& values, std::initializer_list<std::int64_t> sizes, Config cfg) {
  return pluss::make_series(values, sizes,
                            pluss::cfg::dtype(cfg.dtype),
                            pluss::cfg::platform(cfg.platform));
}

SsmModel make_scalar_model(double a, double b, double c, double d, double mean0, double cov0, Config cfg) {
  SsmModel mdl;
  mdl.transition = make_series_cfg(std::vector<double>{a}, {1, 1}, cfg);
  mdl.state_loading = make_series_cfg(std::vector<double>{b}, {1, 1}, cfg);
  mdl.observation = make_series_cfg(std::vector<double>{c}, {1, 1}, cfg);
  mdl.innovation_loading = make_series_cfg(std::vector<double>{d}, {1, 1}, cfg);
  mdl.mean0 = make_series_cfg(std::vector<double>{mean0}, {1}, cfg);
  mdl.cov0 = make_series_cfg(std::vector<double>{cov0}, {1, 1}, cfg);
  return mdl;
}

struct ScalarKalmanRef {
  std::vector<double> x_filt{};
  std::vector<double> p_filt{};
  std::vector<double> x_pred{};
  std::vector<double> p_pred{};
  std::vector<double> ll{};
};

ScalarKalmanRef scalar_kalman_reference(const std::vector<double>& y,
                                        double a,
                                        double q,
                                        double c,
                                        double r,
                                        double x0,
                                        double p0) {
  const std::int64_t n = static_cast<std::int64_t>(y.size());
  ScalarKalmanRef out;
  out.x_filt.resize(static_cast<std::size_t>(n), 0.0);
  out.p_filt.resize(static_cast<std::size_t>(n), 0.0);
  out.x_pred.resize(static_cast<std::size_t>(n), 0.0);
  out.p_pred.resize(static_cast<std::size_t>(n), 0.0);
  out.ll.resize(static_cast<std::size_t>(n), 0.0);

  double x = x0;
  double p = p0;
  for (std::int64_t t = 0; t < n; ++t) {
    const double xp = a * x;
    const double pp = a * a * p + q;
    out.x_pred[static_cast<std::size_t>(t)] = xp;
    out.p_pred[static_cast<std::size_t>(t)] = pp;

    const double yt = y[static_cast<std::size_t>(t)];
    if (!std::isfinite(yt)) {
      x = xp;
      p = pp;
      out.x_filt[static_cast<std::size_t>(t)] = x;
      out.p_filt[static_cast<std::size_t>(t)] = p;
      out.ll[static_cast<std::size_t>(t)] = 0.0;
      continue;
    }

    const double innov = yt - c * xp;
    const double s = c * c * pp + r;
    const double k = pp * c / s;
    x = xp + k * innov;
    p = (1.0 - k * c) * pp;

    out.x_filt[static_cast<std::size_t>(t)] = x;
    out.p_filt[static_cast<std::size_t>(t)] = p;
    out.ll[static_cast<std::size_t>(t)] =
        -0.5 * (std::log(2.0 * kPi * s) + (innov * innov) / s);
  }
  return out;
}

std::vector<double> as_vec(const Series& x) {
  return x.to(pluss::pCPU, ScalarType::Float64).reshape({x.numel()}).to_vec<double>();
}

} // namespace

TEST_CASE("ts ssm: scalar filter matches closed-form Kalman recursion", "[ts][ssm][filter][parity]") {
  const auto cfg = Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);
  const double a = 0.92;
  const double b = 0.2;
  const double c = 1.15;
  const double d = 0.35;
  const double x0 = 0.1;
  const double p0 = 0.7;
  const auto mdl = make_scalar_model(a, b, c, d, x0, p0, cfg);

  const std::vector<double> yv{0.3, -0.2, 0.5, 0.1, -0.4, 0.2, 0.0};
  const Series y = make_series_cfg(yv, {static_cast<std::int64_t>(yv.size()), 1}, cfg);

  const auto out = Ssm::filter(y, mdl, {}, cfg);
  const auto ref = scalar_kalman_reference(yv, a, b * b, c, d * d, x0, p0);

  const auto xf = as_vec(out.filtered_states);
  const auto xp = as_vec(out.forecasted_states);
  const auto pf = as_vec(out.filtered_states_cov.reshape({static_cast<std::int64_t>(yv.size())}));
  const auto pp = as_vec(out.forecasted_states_cov.reshape({static_cast<std::int64_t>(yv.size())}));
  const auto ll = as_vec(out.loglik_per_obs);

  REQUIRE(xf.size() == yv.size());
  for (std::size_t i = 0; i < yv.size(); ++i) {
    REQUIRE(xf[i] == Approx(ref.x_filt[i]).margin(1e-9));
    REQUIRE(xp[i] == Approx(ref.x_pred[i]).margin(1e-9));
    REQUIRE(pf[i] == Approx(ref.p_filt[i]).margin(1e-9));
    REQUIRE(pp[i] == Approx(ref.p_pred[i]).margin(1e-9));
    REQUIRE(ll[i] == Approx(ref.ll[i]).margin(1e-7));
  }
}

TEST_CASE("ts ssm: missing observations are ignored and smoothing contracts variance", "[ts][ssm][missing][smooth]") {
  const auto cfg = Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);
  const auto mdl = make_scalar_model(0.9, 0.25, 1.0, 0.5, 0.0, 1.0, cfg);
  const double nan = std::numeric_limits<double>::quiet_NaN();
  const Series y = make_series_cfg(std::vector<double>{0.2, nan, -0.1, 0.4, nan, 0.0}, {6, 1}, cfg);

  const auto filt = Ssm::filter(y, mdl, {}, cfg);
  const auto smooth = Ssm::smooth(y, mdl, {}, cfg);

  const auto used = as_vec(filt.data_used.reshape({6}));
  const auto xf = as_vec(filt.filtered_states.reshape({6}));
  const auto xp = as_vec(filt.forecasted_states.reshape({6}));
  REQUIRE(used[1] == Approx(0.0));
  REQUIRE(used[4] == Approx(0.0));
  REQUIRE(xf[1] == Approx(xp[1]).margin(1e-10));
  REQUIRE(xf[4] == Approx(xp[4]).margin(1e-10));

  const auto vf = as_vec(filt.filtered_states_cov.reshape({6}));
  const auto vs = as_vec(smooth.smoothed_states_cov.reshape({6}));
  for (std::size_t i = 0; i < vf.size(); ++i) {
    REQUIRE(vs[i] <= vf[i] + 1e-8);
  }
}

TEST_CASE("ts ssm: forecast and simulation contracts are stable", "[ts][ssm][forecast][simulate]") {
  const auto cfg = Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);
  const auto mdl = make_scalar_model(0.85, 0.18, 1.0, 0.4, 0.3, 0.5, cfg);
  const Series yhist = make_series_cfg(std::vector<double>{0.1, 0.4, -0.2, 0.3}, {4, 1}, cfg);

  const auto f = Ssm::forecast(5, yhist, mdl, {}, cfg);
  REQUIRE(f.observations.sizes()[0] == 5);
  REQUIRE(f.states.sizes()[0] == 5);
  REQUIRE(f.observations_cov.isfinite().all().item<double>() == Approx(1.0));
  REQUIRE(f.states_cov.isfinite().all().item<double>() == Approx(1.0));

  const auto s1 = Ssm::simulate(16, mdl, 1, cfg, 20260319ULL);
  const auto s2 = Ssm::simulate(16, mdl, 1, cfg, 20260319ULL);
  REQUIRE(s1.observations.sizes().size() == 2);
  REQUIRE(s1.observations.sizes()[0] == 16);
  REQUIRE(s1.observations.sub(s2.observations).abs().max().item<double>() == Approx(0.0).margin(1e-12));

  const auto sm = Ssm::simulate(10, mdl, 3, cfg, 20260320ULL);
  REQUIRE(sm.observations.sizes().size() == 3);
  REQUIRE(sm.observations.sizes()[0] == 3);
  REQUIRE(sm.observations.sizes()[1] == 10);
}
