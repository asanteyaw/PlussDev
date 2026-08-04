#include <pluss/ma.h>

#include <cmath>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

using pluss::Config;
using pluss::ScalarType;
using pluss::Series;
using pluss::subsp::stats::ts::Ssm;
using pluss::subsp::stats::ts::SsmFilterResult;
using pluss::subsp::stats::ts::SsmForecastResult;
using pluss::subsp::stats::ts::SsmModel;
using pluss::subsp::stats::ts::SsmOptions;

template <class T>
Series make_series_cfg(const std::vector<T>& values, std::initializer_list<std::int64_t> sizes, Config cfg) {
  return pluss::make_series(values, sizes,
                            pluss::cfg::dtype(cfg.dtype),
                            pluss::cfg::platform(cfg.platform));
}

void expect(bool cond, const std::string& msg) {
  if (!cond) {
    throw std::runtime_error(msg);
  }
}

double scalar_at(const Series& x, std::int64_t i) {
  return x.index_select(0, pluss::series::full({1}, i, Config{}.with_dtype(ScalarType::Int64)))
      .reshape({1})
      .to(pluss::pCPU, ScalarType::Float64)
      .item<double>();
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

void test_filter_missing_and_shapes() {
  const auto cfg = Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);
  const SsmModel mdl = make_scalar_model(0.9, 0.2, 1.0, 0.5, 0.0, 1.0, cfg);
  const Series y = make_series_cfg(
      std::vector<double>{0.4, -0.2, 0.7, std::numeric_limits<double>::quiet_NaN(), 0.1, -0.1},
      {6, 1},
      cfg);
  SsmOptions opt;
  opt.tolerance = 1e-12;
  const SsmFilterResult out = Ssm::filter(y, mdl, opt, cfg);

  expect(out.filtered_states.sizes()[0] == 6 && out.filtered_states.sizes()[1] == 1, "filtered_states shape mismatch");
  expect(out.filtered_states_cov.sizes()[0] == 6, "filtered_states_cov shape mismatch");
  expect(out.forecasted_obs.sizes()[0] == 6 && out.forecasted_obs.sizes()[1] == 1, "forecasted_obs shape mismatch");
  expect(out.kalman_gain.sizes()[0] == 6 && out.kalman_gain.sizes()[1] == 1 && out.kalman_gain.sizes()[2] == 1,
         "kalman_gain shape mismatch");
  expect(std::isfinite(out.loglik), "loglik must be finite");

  const Series f3 = out.filtered_states.index_select(0, pluss::series::full({1}, 3, Config{}.with_dtype(ScalarType::Int64))).reshape({1});
  const Series p3 = out.forecasted_states.index_select(0, pluss::series::full({1}, 3, Config{}.with_dtype(ScalarType::Int64))).reshape({1});
  const Series used3 = out.data_used.index_select(0, pluss::series::full({1}, 3, Config{}.with_dtype(ScalarType::Int64))).reshape({1});
  const double f3v = f3.to(pluss::pCPU, ScalarType::Float64).item<double>();
  const double p3v = p3.to(pluss::pCPU, ScalarType::Float64).item<double>();
  const double u3v = used3.to(pluss::pCPU, ScalarType::Float64).item<double>();
  expect(std::abs(f3v - p3v) <= 1e-10, "missing-observation period should not update state mean");
  expect(std::abs(u3v) <= 1e-12, "missing-observation period should have DataUsed=0");
}

void test_smooth_reduces_uncertainty() {
  const auto cfg = Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);
  const SsmModel mdl = make_scalar_model(0.95, 0.15, 1.0, 0.4, 0.0, 1.0, cfg);
  const Series y = make_series_cfg(std::vector<double>{0.2, 0.1, -0.3, 0.5, -0.4, 0.0, 0.2}, {7, 1}, cfg);
  const auto out = Ssm::smooth(y, mdl, {}, cfg);

  const Series pf = out.filter.filtered_states_cov.to(pluss::pCPU, ScalarType::Float64).reshape({7, 1, 1});
  const Series ps = out.smoothed_states_cov.to(pluss::pCPU, ScalarType::Float64).reshape({7, 1, 1});
  for (std::int64_t t = 0; t < 7; ++t) {
    const double vf = pf.select(0, t).reshape({1}).item<double>();
    const double vs = ps.select(0, t).reshape({1}).item<double>();
    expect(vs <= vf + 1e-8, "smoothed variance should not exceed filtered variance in scalar case");
  }
}

void test_forecast_prior_closed_form() {
  const auto cfg = Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);
  const double a = 0.8;
  const double b = 0.3;
  const double c = 1.1;
  const double d = 0.5;
  const double mean0 = 0.7;
  const double cov0 = 0.4;
  const SsmModel mdl = make_scalar_model(a, b, c, d, mean0, cov0, cfg);

  const SsmForecastResult f = Ssm::forecast(4, mdl, cfg);
  expect(f.states.sizes()[0] == 4 && f.states.sizes()[1] == 1, "forecast states shape mismatch");
  expect(f.observations.sizes()[0] == 4 && f.observations.sizes()[1] == 1, "forecast observations shape mismatch");

  double x_mean = mean0;
  double x_var = cov0;
  const double q = b * b;
  const double r = d * d;
  for (std::int64_t h = 0; h < 4; ++h) {
    x_mean = a * x_mean;
    x_var = a * a * x_var + q;
    const double y_mean = c * x_mean;
    const double y_var = c * c * x_var + r;

    const double xh = scalar_at(f.states.reshape({4}), h);
    const double yh = scalar_at(f.observations.reshape({4}), h);
    const double xv = f.states_cov.select(0, h).reshape({1}).to(pluss::pCPU, ScalarType::Float64).item<double>();
    const double yv = f.observations_cov.select(0, h).reshape({1}).to(pluss::pCPU, ScalarType::Float64).item<double>();
    expect(std::abs(xh - x_mean) <= 1e-8, "forecast state mean mismatch");
    expect(std::abs(yh - y_mean) <= 1e-8, "forecast observation mean mismatch");
    expect(std::abs(xv - x_var) <= 1e-8, "forecast state variance mismatch");
    expect(std::abs(yv - y_var) <= 1e-8, "forecast observation variance mismatch");
  }
}

void test_simulation_shapes() {
  const auto cfg = Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);
  const SsmModel mdl = make_scalar_model(0.95, 0.2, 1.0, 0.3, 0.0, 1.0, cfg);

  const auto s1 = Ssm::simulate(20, mdl, 1, cfg, 20260317ULL);
  expect(s1.observations.sizes().size() == 2 && s1.observations.sizes()[0] == 20, "simulate (1 path) obs shape mismatch");
  expect(s1.states.sizes().size() == 2 && s1.states.sizes()[0] == 20, "simulate (1 path) state shape mismatch");

  const auto s4 = Ssm::simulate(12, mdl, 4, cfg, 20260318ULL);
  expect(s4.observations.sizes().size() == 3 &&
             s4.observations.sizes()[0] == 4 &&
             s4.observations.sizes()[1] == 12,
         "simulate (multi-path) obs shape mismatch");
  expect(s4.states.sizes().size() == 3 &&
             s4.states.sizes()[0] == 4 &&
             s4.states.sizes()[1] == 12,
         "simulate (multi-path) state shape mismatch");
}

} // namespace

int main() {
  try {
    test_filter_missing_and_shapes();
    test_smooth_reduces_uncertainty();
    test_forecast_prior_closed_form();
    test_simulation_shapes();
    std::cout << "ssm_test passed\n";
    return 0;
  } catch (const std::exception& e) {
    std::cerr << "ssm_test failed: " << e.what() << "\n";
    return 1;
  }
}
