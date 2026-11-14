#include <pluss/ma.h>

#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>

namespace {

using pluss::Config;
using pluss::ScalarType;
using pluss::Series;
using pluss::subsp::math::finance::filtering::EkfModel;
using pluss::subsp::math::finance::filtering::KalmanModel;
using pluss::subsp::math::finance::filtering::ParticleFilterModel;
using pluss::subsp::math::finance::filtering::ParticleResampleMethod;
using pluss::subsp::math::finance::filtering::ParticleResampleTrigger;
using pluss::subsp::math::finance::filtering::UkfModel;
using pluss::subsp::math::finance::filtering::ekf_filter;
using pluss::subsp::math::finance::filtering::kalman_filter;
using pluss::subsp::math::finance::filtering::particle_filter;
using pluss::subsp::math::finance::filtering::sv_particle_filter;
using pluss::subsp::math::finance::filtering::ukf_filter;
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
  model.transition = pluss::series::full({1, 1}, 1.0, cfg);
  model.observation = pluss::series::full({1, 1}, 1.0, cfg);
  model.process_cov = pluss::series::full({1, 1}, 0.01, cfg);
  model.observation_cov = pluss::series::full({1, 1}, 0.04, cfg);
  model.initial_mean = pluss::series::full({1}, 0.0, cfg);
  model.initial_cov = pluss::series::full({1, 1}, 1.0, cfg);

  const Series y = pluss::series::full({t_obs}, 1.25, cfg);
  const auto out = kalman_filter(y, model, cfg);

  expect(out.filtered_mean.sizes().size() == 2, "kalman filtered_mean rank mismatch");
  expect(out.filtered_mean.sizes()[0] == t_obs && out.filtered_mean.sizes()[1] == 1,
         "kalman filtered_mean shape mismatch");
  expect(out.filtered_cov.sizes()[0] == t_obs && out.filtered_cov.sizes()[1] == 1 && out.filtered_cov.sizes()[2] == 1,
         "kalman filtered_cov shape mismatch");
  expect(out.data_used.sizes()[0] == t_obs && out.data_used.sizes()[1] == 1,
         "kalman data_used shape mismatch");
  expect(std::isfinite(out.loglik), "kalman loglik must be finite");

  const double tail_mean = out.filtered_mean.select(0, t_obs - 1).reshape({1}).host<double>().front();
  expect(std::abs(tail_mean - 1.25) < 0.1, "kalman filtered mean should track constant signal");
}

void test_linear_kalman_missing_and_tol() {
  const Config cfg = f64_cpu();
  const std::int64_t t_obs = 48;

  KalmanModel model;
  model.transition = pluss::series::full({1, 1}, 1.0, cfg);
  model.observation = pluss::series::full({1, 1}, 1.0, cfg);
  model.process_cov = pluss::series::full({1, 1}, 0.01, cfg);
  model.observation_cov = pluss::series::full({1, 1}, 0.04, cfg);
  model.initial_mean = pluss::series::full({1}, 0.0, cfg);
  model.initial_cov = pluss::series::full({1, 1}, 1.0, cfg);

  Series y = pluss::series::full({t_obs}, 1.1, cfg);
  const Series idx_nan = pluss::series::full({1}, 20, Config{}.with_dtype(ScalarType::Int64).with_platform(cfg.platform));
  y = pluss::index_put(y,
                       std::vector<Series>{idx_nan},
                       pluss::series::full({1}, std::numeric_limits<double>::quiet_NaN(), cfg),
                       false);

  const auto out = kalman_filter(y, model, cfg);
  expect(out.filtered_mean.isfinite().all().item<double>() == 1.0, "kalman missing-data: filtered mean must stay finite");
  expect(std::isfinite(out.loglik), "kalman missing-data: loglik must be finite");

  const double f_missing = out.filtered_mean.select(0, 20).reshape({1}).host<double>().front();
  const double p_missing = out.predicted_mean.select(0, 20).reshape({1}).host<double>().front();
  expect(std::abs(f_missing - p_missing) < 1e-10,
         "kalman missing-data: when observation is NaN, filtered state should equal predicted state");
  const double used_missing = out.data_used.select(0, 20).reshape({1}).host<double>().front();
  expect(used_missing == 0.0, "kalman missing-data: data_used must be 0 for NaN channel");

  KalmanModel tol_model = model;
  tol_model.info_tol = 1e9;
  const auto out_tol = kalman_filter(pluss::series::full({t_obs}, 3.0, cfg), tol_model, cfg);
  const double f0 = out_tol.filtered_mean.select(0, 0).reshape({1}).host<double>().front();
  const double p0 = out_tol.predicted_mean.select(0, 0).reshape({1}).host<double>().front();
  expect(std::abs(f0 - p0) < 1e-10, "kalman info_tol: first-step filtered should equal predicted when all info is ignored");
  const double used0 = out_tol.data_used.select(0, 0).reshape({1}).host<double>().front();
  expect(used0 == 0.0, "kalman info_tol: data_used must be 0 when info_tol blocks observation");
}

void test_ekf() {
  const Config cfg = f64_cpu();
  const std::int64_t t_obs = 32;

  EkfModel model;
  model.transition = [](const Series& x, std::int64_t) { return x; };
  model.observation = [](const Series& x, std::int64_t) { return x.square(); };
  model.transition_jacobian = [](const Series&, std::int64_t) { return pluss::series::full({1, 1}, 1.0, f64_cpu()); };
  model.observation_jacobian = [](const Series& x, std::int64_t) { return x.mul(2.0).reshape({1, 1}); };
  model.process_cov = pluss::series::full({1, 1}, 0.01, cfg);
  model.observation_cov = pluss::series::full({1, 1}, 0.05, cfg);
  model.initial_mean = pluss::series::full({1}, 1.0, cfg);
  model.initial_cov = pluss::series::full({1, 1}, 0.5, cfg);

  const Series y = pluss::series::full({t_obs}, 1.0, cfg);
  const auto out = ekf_filter(y, model, cfg);

  expect(out.filtered_mean.sizes()[0] == t_obs, "ekf filtered_mean length mismatch");
  expect(out.data_used.sizes()[0] == t_obs && out.data_used.sizes()[1] == 1, "ekf data_used shape mismatch");
  expect(out.filtered_mean.isfinite().all().item<double>() == 1.0, "ekf filtered_mean has non-finite values");
  expect(std::isfinite(out.loglik), "ekf loglik must be finite");
}

void test_ukf() {
  const Config cfg = f64_cpu();
  const std::int64_t t_obs = 32;

  UkfModel model;
  model.transition = [](const Series& x, std::int64_t) { return x; };
  model.observation = [](const Series& x, std::int64_t) { return x.square(); };
  model.process_cov = pluss::series::full({1, 1}, 0.01, cfg);
  model.observation_cov = pluss::series::full({1, 1}, 0.05, cfg);
  model.initial_mean = pluss::series::full({1}, 1.0, cfg);
  model.initial_cov = pluss::series::full({1, 1}, 0.5, cfg);

  const Series y = pluss::series::full({t_obs}, 1.0, cfg);
  const auto out = ukf_filter(y, model, cfg);
  expect(out.filtered_mean.sizes()[0] == t_obs, "ukf filtered_mean length mismatch");
  expect(out.data_used.sizes()[0] == t_obs && out.data_used.sizes()[1] == 1, "ukf data_used shape mismatch");
  expect(out.filtered_mean.isfinite().all().item<double>() == 1.0, "ukf filtered_mean has non-finite values");
  expect(std::isfinite(out.loglik), "ukf loglik must be finite");
}

void test_particle_filter() {
  const Config cfg = f64_cpu();
  const std::int64_t t_obs = 48;
  const std::int64_t n_particles = 1200;

  ParticleFilterModel model;
  model.initial_particles = pluss::series::randn({n_particles, 1}, cfg, 20260701ULL);
  model.options.resample = true;
  model.options.ess_threshold = 0.6;
  model.options.store_particles = true;

  model.transition = [=](const Series& particles, std::int64_t t) {
    const Series eps = pluss::series::randn({particles.sizes()[0], particles.sizes()[1]}, cfg,
                                    0xC0FFEEULL + static_cast<std::uint64_t>(t + 1));
    return particles.mul(0.98).add(eps.mul(0.12));
  };

  model.loglik = [](const Series& particles, const Series& obs, std::int64_t) {
    const std::int64_t n = particles.sizes()[0];
    const double var = 0.15 * 0.15;
    const Series resid = obs.reshape({1, 1}).sub(particles);
    return resid.square().div(var).add(kLog2Pi + std::log(var)).mul(-0.5).reshape({n});
  };

  const Series y = pluss::series::full({t_obs}, 0.0, cfg);
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

void test_particle_filter_resampling_modes() {
  const Config cfg = f64_cpu();
  const std::int64_t t_obs = 24;
  const std::int64_t n_particles = 1000;

  ParticleFilterModel model;
  model.initial_particles = pluss::series::randn({n_particles, 1}, cfg, 12345ULL);
  model.options.resample = true;
  model.options.resample_trigger = ParticleResampleTrigger::Interval;
  model.options.resample_interval = 2;
  model.options.resample_method = ParticleResampleMethod::Residual;
  model.options.store_particles = true;
  model.options.resample_seed = 67890ULL;

  model.transition = [=](const Series& particles, std::int64_t t) {
    const Series eps = pluss::series::randn({particles.sizes()[0], particles.sizes()[1]}, cfg,
                                    3333ULL + static_cast<std::uint64_t>(t + 1));
    return particles.add(eps.mul(0.05));
  };

  model.loglik = [](const Series& particles, const Series& obs, std::int64_t) {
    const std::int64_t n = particles.sizes()[0];
    const double var = 0.1 * 0.1;
    const Series resid = obs.reshape({1, 1}).sub(particles);
    return resid.square().div(var).add(kLog2Pi + std::log(var)).mul(-0.5).reshape({n});
  };

  const Series y = pluss::series::full({t_obs}, 0.0, cfg);
  const auto out = particle_filter(y, model, cfg);
  expect(out.filtered_mean.sizes()[0] == t_obs && out.filtered_mean.sizes()[1] == 1,
         "particle resample modes: filtered_mean shape mismatch");
  expect(out.particles.sizes()[0] == t_obs && out.particles.sizes()[1] == n_particles,
         "particle resample modes: particles shape mismatch");
  expect(out.particle_weights.sizes()[0] == t_obs && out.particle_weights.sizes()[1] == n_particles,
         "particle resample modes: particle_weights shape mismatch");
  expect(out.particle_weights.isfinite().all().item<double>() == 1.0,
         "particle resample modes: particle_weights has non-finite");
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

  const Series r = pluss::series::randn({56}, cfg, 20260702ULL).mul(0.02);
  const auto out = sv_particle_filter(r, p, 1400, 1.0 / 252.0, 0.6, cfg, 20260703ULL);

  expect(out.filtered_mean.sizes()[0] == 56 && out.filtered_mean.sizes()[1] == 1,
         "sv_particle filtered_mean shape mismatch");
  expect(out.particle_weights.sizes()[0] == 56 && out.particle_weights.sizes()[1] == 1400,
         "sv_particle weights shape mismatch");
  expect(out.ess.numel() == 56, "sv_particle ess shape mismatch");
  expect(out.filtered_mean.isfinite().all().item<double>() == 1.0, "sv_particle filtered mean has non-finite values");
  expect(std::isfinite(out.loglik), "sv_particle loglik must be finite");
}


// The leverage effect: data simulated with a strongly negative correlation between the return and the next log-variance shock is better
// explained by the filter that knows the correlation than by one that sets it to zero. Before the filter used rho, both gave the same
// likelihood, whatever rho was.
void test_sv_particle_filter_uses_leverage() {
  const Config cfg = f64_cpu();
  SvParams truth;
  truth.mu = 0.0;
  truth.logvar_mean = -3.0;
  truth.phi = 0.9;
  truth.vol_of_logvar = 6.0;  // per sqrt(year): about 0.38 per day-step of 1/252
  truth.rho = -0.9;
  truth.logvar0 = -3.0;
  const double dt = 1.0 / 252.0;
  const auto sim = pluss::subsp::math::finance::volatility::detail::sv_paths(1500, 1, 100.0, dt, truth, cfg, 11ULL);
  const Series path = sim.spot.reshape({1501});
  const Series returns = path.slice(0, 1, 1501, 1).div(path.slice(0, 0, 1500, 1)).log();

  SvParams blind = truth;
  blind.rho = 0.0;
  double wins = 0.0;
  for (std::uint64_t seed : {1ULL, 2ULL, 3ULL}) {
    const double with = sv_particle_filter(returns, truth, 600, dt, 0.5, cfg, seed).loglik;
    const double without = sv_particle_filter(returns, blind, 600, dt, 0.5, cfg, seed).loglik;
    if (with > without) wins += 1.0;
  }
  expect(wins >= 2.0, "the SV particle filter must use the leverage correlation rho");
}

// On a linear Gaussian model the unscented filter is exact, so it must reproduce the Kalman filter: means, covariances and the log-likelihood,
// for any sigma-point spread, including a state of small variance such as an interest rate (the jitter must not swamp alpha^2 P).
void test_ukf_matches_kalman_on_linear_models() {
  const Config cfg = f64_cpu();
  const std::int64_t t_obs = 60;
  for (const double scale : {1.0, 1e-4}) {  // 1e-4: the variance of a short rate is of order 1e-4
    KalmanModel kalman;
    kalman.transition = pluss::series::full({1, 1}, 0.95, cfg);
    kalman.observation = pluss::series::full({1, 1}, 1.0, cfg);
    kalman.process_cov = pluss::series::full({1, 1}, 0.01 * scale, cfg);
    kalman.observation_cov = pluss::series::full({1, 1}, 0.04 * scale, cfg);
    kalman.initial_mean = pluss::series::full({1}, 0.0, cfg);
    kalman.initial_cov = pluss::series::full({1, 1}, 0.5 * scale, cfg);
    const Series y = pluss::series::randn({t_obs}, cfg, 77ULL).mul(std::sqrt(0.2 * scale));
    const auto exact = kalman_filter(y, kalman, cfg);
    for (const double alpha : {1e-3, 0.5}) {
      UkfModel ukf;
      ukf.transition = [](const Series& x, std::int64_t) { return x.mul(0.95); };
      ukf.observation = [](const Series& x, std::int64_t) { return x; };
      ukf.process_cov = kalman.process_cov;
      ukf.observation_cov = kalman.observation_cov;
      ukf.initial_mean = kalman.initial_mean;
      ukf.initial_cov = kalman.initial_cov;
      ukf.options.alpha = alpha;
      const auto got = ukf_filter(y, ukf, cfg);
      expect(std::abs(got.loglik - exact.loglik) < 1e-2, "ukf log-likelihood must equal the Kalman one on a linear model");  // the jitter of 1e-10 is what remains
      expect(got.filtered_mean.sub(exact.filtered_mean).abs().max().item<double>() < 1e-6, "ukf filtered mean must equal the Kalman one");
      expect(got.filtered_cov.sub(exact.filtered_cov).abs().max().item<double>() < 1e-9, "ukf filtered covariance must equal the Kalman one");
    }
  }
}

} // namespace

int main() {
  try {
    test_linear_kalman();
    test_linear_kalman_missing_and_tol();
    test_ekf();
    test_ukf();
    test_ukf_matches_kalman_on_linear_models();
    test_particle_filter();
    test_particle_filter_resampling_modes();
    test_sv_particle_filter();
    test_sv_particle_filter_uses_leverage();
    std::cout << "filtering_test passed\n";
    return 0;
  } catch (const std::exception& e) {
    std::cerr << "filtering_test failed: " << e.what() << '\n';
    return 1;
  }
}
