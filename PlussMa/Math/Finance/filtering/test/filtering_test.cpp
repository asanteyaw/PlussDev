#include <pluss/ma.h>

#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {

using pluss::Config;
using pluss::ScalarType;
using pluss::Series;
using pluss::subsp::math::finance::filtering::EkfModel;
using pluss::subsp::math::finance::filtering::KalmanModel;
using pluss::subsp::math::finance::filtering::ParticleFilterModel;
using pluss::subsp::math::finance::filtering::ekf_filter;
using pluss::subsp::math::finance::filtering::kalman_filter;
using pluss::subsp::math::finance::filtering::particle_filter;
using pluss::subsp::math::finance::filtering::sv_particle_filter;
using pluss::subsp::math::finance::volatility::SvParams;

constexpr double kLog2Pi = 1.8378770664093453;

void expect(bool condition, const std::string& message) {
  if (!condition) {
    throw std::runtime_error(message);
  }
}

Config f64_cpu() {
  return Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);
}

void test_linear_kalman() {
  const Config cfg = f64_cpu();
  const std::int64_t t_obs = 64;

  KalmanModel model;
  model.transition = pluss::full({1, 1}, 1.0, cfg);
  model.observation = pluss::full({1, 1}, 1.0, cfg);
  model.process_cov = pluss::full({1, 1}, 0.01, cfg);
  model.observation_cov = pluss::full({1, 1}, 0.04, cfg);
  model.initial_mean = pluss::full({1}, 0.0, cfg);
  model.initial_cov = pluss::full({1, 1}, 1.0, cfg);

  const Series y = pluss::full({t_obs}, 1.25, cfg);
  const auto out = kalman_filter(y, model, cfg);

  expect(out.filtered_mean.sizes().size() == 2, "kalman filtered_mean rank mismatch");
  expect(out.filtered_mean.sizes()[0] == t_obs && out.filtered_mean.sizes()[1] == 1,
         "kalman filtered_mean shape mismatch");
  expect(out.filtered_cov.sizes()[0] == t_obs && out.filtered_cov.sizes()[1] == 1 && out.filtered_cov.sizes()[2] == 1,
         "kalman filtered_cov shape mismatch");
  expect(std::isfinite(out.loglik), "kalman loglik must be finite");

  const double tail_mean = out.filtered_mean.select(0, t_obs - 1).reshape({1}).item<double>();
  expect(std::abs(tail_mean - 1.25) < 0.1, "kalman filtered mean should track constant signal");
}

void test_ekf() {
  const Config cfg = f64_cpu();
  const std::int64_t t_obs = 32;

  EkfModel model;
  model.transition = [](const Series& x, std::int64_t) { return x; };
  model.observation = [](const Series& x, std::int64_t) { return x.square(); };
  model.transition_jacobian = [](const Series&, std::int64_t) { return pluss::full({1, 1}, 1.0, f64_cpu()); };
  model.observation_jacobian = [](const Series& x, std::int64_t) { return x.mul(2.0).reshape({1, 1}); };
  model.process_cov = pluss::full({1, 1}, 0.01, cfg);
  model.observation_cov = pluss::full({1, 1}, 0.05, cfg);
  model.initial_mean = pluss::full({1}, 1.0, cfg);
  model.initial_cov = pluss::full({1, 1}, 0.5, cfg);

  const Series y = pluss::full({t_obs}, 1.0, cfg);
  const auto out = ekf_filter(y, model, cfg);

  expect(out.filtered_mean.sizes()[0] == t_obs, "ekf filtered_mean length mismatch");
  expect(out.filtered_mean.isfinite().all().item<double>() == 1.0, "ekf filtered_mean has non-finite values");
  expect(std::isfinite(out.loglik), "ekf loglik must be finite");
}

void test_particle_filter() {
  const Config cfg = f64_cpu();
  const std::int64_t t_obs = 48;
  const std::int64_t n_particles = 1200;

  ParticleFilterModel model;
  model.initial_particles = pluss::randn({n_particles, 1}, cfg, 20260701ULL);
  model.options.resample = true;
  model.options.ess_threshold = 0.6;
  model.options.store_particles = true;

  model.transition = [=](const Series& particles, std::int64_t t) {
    const Series eps = pluss::randn({particles.sizes()[0], particles.sizes()[1]}, cfg,
                                    0xC0FFEEULL + static_cast<std::uint64_t>(t + 1));
    return particles.mul(0.98).add(eps.mul(0.12));
  };

  model.loglik = [](const Series& particles, const Series& obs, std::int64_t) {
    const std::int64_t n = particles.sizes()[0];
    const double var = 0.15 * 0.15;
    const Series resid = obs.reshape({1, 1}).sub(particles);
    return resid.square().div(var).add(kLog2Pi + std::log(var)).mul(-0.5).reshape({n});
  };

  const Series y = pluss::full({t_obs}, 0.0, cfg);
  const auto out = particle_filter(y, model, cfg);

  expect(out.filtered_mean.sizes().size() == 2, "particle filtered_mean rank mismatch");
  expect(out.filtered_mean.sizes()[0] == t_obs && out.filtered_mean.sizes()[1] == 1,
         "particle filtered_mean shape mismatch");
  expect(out.particle_weights.sizes()[0] == t_obs && out.particle_weights.sizes()[1] == n_particles,
         "particle weights shape mismatch");
  expect(out.ess.numel() == t_obs, "particle ess shape mismatch");
  expect(out.filtered_mean.isfinite().all().item<double>() == 1.0, "particle filtered mean has non-finite values");
  expect(std::isfinite(out.loglik), "particle loglik must be finite");
}

void test_sv_particle_filter() {
  const Config cfg = f64_cpu();
  SvParams p;
  p.mu = 0.0;
  p.logvar_mean = -3.3;
  p.phi = 0.97;
  p.vol_of_logvar = 0.22;
  p.rho = -0.55;
  p.logvar0 = -3.2;

  const Series r = pluss::randn({56}, cfg, 20260702ULL).mul(0.02);
  const auto out = sv_particle_filter(r, p, 1400, 1.0 / 252.0, 0.6, cfg, 20260703ULL);

  expect(out.filtered_mean.sizes()[0] == 56 && out.filtered_mean.sizes()[1] == 1,
         "sv_particle filtered_mean shape mismatch");
  expect(out.particle_weights.sizes()[0] == 56 && out.particle_weights.sizes()[1] == 1400,
         "sv_particle weights shape mismatch");
  expect(out.ess.numel() == 56, "sv_particle ess shape mismatch");
  expect(out.filtered_mean.isfinite().all().item<double>() == 1.0, "sv_particle filtered mean has non-finite values");
  expect(std::isfinite(out.loglik), "sv_particle loglik must be finite");
}

} // namespace

int main() {
  try {
    test_linear_kalman();
    test_ekf();
    test_particle_filter();
    test_sv_particle_filter();
    std::cout << "filtering_test passed\n";
    return 0;
  } catch (const std::exception& e) {
    std::cerr << "filtering_test failed: " << e.what() << '\n';
    return 1;
  }
}
