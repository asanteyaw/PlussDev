#include <pluss/ma.h>

#include <cmath>
#include <iostream>

int main() {
  namespace flt = pluss::subsp::math::finance::filtering;
  namespace vol = pluss::subsp::math::finance::volatility;

  const auto cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  flt::KalmanModel km;
  km.transition = pluss::full({1, 1}, 1.0, cfg);
  km.observation = pluss::full({1, 1}, 1.0, cfg);
  km.process_cov = pluss::full({1, 1}, 0.01, cfg);
  km.observation_cov = pluss::full({1, 1}, 0.04, cfg);
  km.initial_mean = pluss::full({1}, 0.0, cfg);
  km.initial_cov = pluss::full({1, 1}, 1.0, cfg);

  const auto y = pluss::full({40}, 1.2, cfg);
  const auto kf = flt::kalman_filter(y, km, cfg);

  std::cout << "Kalman filter\n";
  std::cout << "filtered_mean=" << kf.filtered_mean << "\n";
  std::cout << "innovation=" << kf.innovation << "\n";
  std::cout << "loglik=" << kf.loglik << "\n";

  flt::ParticleFilterModel pm;
  pm.initial_particles = pluss::randn({800, 1}, cfg, 20260711ULL);
  pm.options.ess_threshold = 0.6;
  pm.options.resample = true;
  pm.options.store_particles = true;

  pm.transition = [=](const pluss::Series& particles, std::int64_t t) {
    const auto eps = pluss::randn({particles.sizes()[0], particles.sizes()[1]}, cfg,
                                  0xBEEFULL + static_cast<std::uint64_t>(t + 1));
    return particles.mul(0.98).add(eps.mul(0.12));
  };

  pm.loglik = [](const pluss::Series& particles, const pluss::Series& obs, std::int64_t) {
    const std::int64_t n = particles.sizes()[0];
    const double var = 0.15 * 0.15;
    const double log2pi = 1.8378770664093453;
    const auto resid = obs.reshape({1, 1}).sub(particles);
    return resid.square().div(var).add(log2pi + std::log(var)).mul(-0.5).reshape({n});
  };

  const auto pf = flt::particle_filter(pluss::full({40}, 0.0, cfg), pm, cfg);

  std::cout << "\nParticle filter\n";
  std::cout << "filtered_mean=" << pf.filtered_mean << "\n";
  std::cout << "ess=" << pf.ess << "\n";
  std::cout << "loglik=" << pf.loglik << "\n";

  vol::SvParams sp;
  sp.mu = 0.0;
  sp.logvar_mean = -3.2;
  sp.phi = 0.97;
  sp.vol_of_logvar = 0.24;
  sp.rho = -0.55;
  sp.logvar0 = -3.1;

  const auto log_returns = pluss::randn({50}, cfg, 20260712ULL).mul(0.02);
  const auto svf = flt::sv_particle_filter(log_returns, sp, 1000, 1.0 / 252.0, 0.6, cfg, 20260713ULL);

  std::cout << "\nSV particle filter\n";
  std::cout << "filtered_mean=" << svf.filtered_mean << "\n";
  std::cout << "ess=" << svf.ess << "\n";
  std::cout << "loglik=" << svf.loglik << "\n";
  return 0;
}
