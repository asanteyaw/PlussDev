#include <pluss/ma.h>

#include <cmath>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

int main() {
  using pluss::Series;
  using pluss::subsp::math::finance::filtering::ParticleFilterModel;
  using pluss::subsp::math::finance::filtering::particle_filter;
  using namespace pluss::subsp::math::optimization;

  std::ifstream in("/Users/yawasante/software/cxx/PlussDev/PlussMic/data/sp500_data.csv");
  if (!in) {
    std::cerr << "failed to open sp500_data.csv\n";
    return 1;
  }

  std::string line;
  std::getline(in, line);
  std::getline(in, line);
  std::getline(in, line);

  std::vector<double> close;
  close.reserve(12000);
  while (std::getline(in, line)) {
    if (line.empty()) continue;
    std::stringstream ss(line);
    std::string field;
    std::int64_t col = 0;
    double c = std::numeric_limits<double>::quiet_NaN();
    while (std::getline(ss, field, ',')) {
      if (col == 2) {
        try {
          c = std::stod(field);
        } catch (...) {
          c = std::numeric_limits<double>::quiet_NaN();
        }
        break;
      }
      ++col;
    }
    if (std::isfinite(c) && c > 0.0) close.push_back(c);
  }
  if (close.size() < 3) {
    std::cerr << "not enough close data\n";
    return 1;
  }

  std::vector<double> log_ret_v;
  log_ret_v.reserve(close.size() - 1);
  for (std::size_t i = 1; i < close.size(); ++i) {
    log_ret_v.push_back(std::log(close[i] / close[i - 1]));
  }
  if (log_ret_v.size() > 3000) {
    log_ret_v = std::vector<double>(log_ret_v.end() - 3000, log_ret_v.end());
  }

  const auto cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);
  const Series y = pluss::make_series(log_ret_v, {static_cast<std::int64_t>(log_ret_v.size())}, cfg)
                       .reshape({static_cast<std::int64_t>(log_ret_v.size()), 1});

  constexpr std::int64_t n_particles = 512;
  constexpr double dt = 1.0 / 252.0;
  constexpr std::uint64_t base_seed = 2026031501ULL;

  const Series x0 = pluss::make_series({std::log(2.0), std::log(0.04), std::log(0.50), 0.05, std::log(0.04)}, {5}, cfg);
  const Series lb = pluss::make_series({-5.0, -12.0, -7.0, -0.4, -12.0}, {5}, cfg);
  const Series ub = pluss::make_series({3.0, -1.0, 1.2, 0.4, -1.0}, {5}, cfg);

  DifferentialEvolutionPrmmConfig de_cfg;
  de_cfg.population_size = 40;
  de_cfg.elite_count = 10;
  de_cfg.max_generations = 25;
  de_cfg.check_every = 5;
  de_cfg.relative_improvement_tolerance = 1e-8;
  de_cfg.lower_bounds = lb;
  de_cfg.upper_bounds = ub;
  de_cfg.display = PopulationSolverDisplay::None;
  de_cfg.rng_seed = base_seed + 31ULL;

  const auto objective = [&](const Series& u) -> double {
    const double kappa = std::exp(u.select(0, 0).item<double>());
    const double theta = std::exp(u.select(0, 1).item<double>());
    const double sigma = std::exp(u.select(0, 2).item<double>());
    const double mu = u.select(0, 3).item<double>();
    const double v0 = std::exp(u.select(0, 4).item<double>());

    ParticleFilterModel m;
    m.initial_particles = pluss::series::randn({n_particles, 1}, cfg, base_seed).mul(std::sqrt(v0) * 0.20).add(v0).clamp_min(1e-12);
    m.options.ess_threshold = 0.5;
    m.options.resample = true;
    m.options.store_particles = false;

    m.transition = [=](const Series& particles, std::int64_t t) {
      const Series eps =
          pluss::series::randn({n_particles, 1}, cfg, base_seed + 7919ULL * static_cast<std::uint64_t>(t + 1));
      const Series v = particles.clamp_min(1e-12);
      return v.add((kappa * dt) * (theta - v))
          .add((sigma * std::sqrt(dt)) * v.sqrt().mul(eps))
          .clamp_min(1e-12);
    };

    m.loglik = [=](const Series& particles, const Series& obs, std::int64_t) {
      const Series v = particles.clamp_min(1e-12);
      const Series var = v.mul(dt).clamp_min(1e-12);
      const Series mean = (mu * dt) - v.mul(0.5 * dt);
      const Series resid = obs.reshape({1, 1}).sub(mean);
      return var.log().add(std::log(2.0 * 3.14159265358979323846)).add(resid.square().div(var)).mul(-0.5).reshape({n_particles});
    };

    const auto pf = particle_filter(y, m, cfg);
    if (!std::isfinite(pf.loglik)) return 1e16;
    const double feller_penalty = std::max(0.0, sigma * sigma - 2.0 * kappa * theta) * 1e6;
    return -pf.loglik + feller_penalty;
  };

  const auto fit = differential_evolution_prmm_minimizer(objective, x0, de_cfg);
  const Series uhat = fit.minimizer.reshape({5});
  const double kappa = std::exp(uhat.select(0, 0).item<double>());
  const double theta = std::exp(uhat.select(0, 1).item<double>());
  const double sigma = std::exp(uhat.select(0, 2).item<double>());
  const double mu = uhat.select(0, 3).item<double>();
  const double v0 = std::exp(uhat.select(0, 4).item<double>());

  ParticleFilterModel final_model;
  final_model.initial_particles =
      pluss::series::randn({n_particles, 1}, cfg, base_seed + 97ULL).mul(std::sqrt(v0) * 0.20).add(v0).clamp_min(1e-12);
  final_model.options.ess_threshold = 0.5;
  final_model.options.resample = true;
  final_model.options.store_particles = true;
  final_model.transition = [=](const Series& particles, std::int64_t t) {
    const Series eps =
        pluss::series::randn({n_particles, 1}, cfg, base_seed + 104729ULL + 7919ULL * static_cast<std::uint64_t>(t + 1));
    const Series v = particles.clamp_min(1e-12);
    return v.add((kappa * dt) * (theta - v))
        .add((sigma * std::sqrt(dt)) * v.sqrt().mul(eps))
        .clamp_min(1e-12);
  };
  final_model.loglik = [=](const Series& particles, const Series& obs, std::int64_t) {
    const Series v = particles.clamp_min(1e-12);
    const Series var = v.mul(dt).clamp_min(1e-12);
    const Series mean = (mu * dt) - v.mul(0.5 * dt);
    const Series resid = obs.reshape({1, 1}).sub(mean);
    return var.log().add(std::log(2.0 * 3.14159265358979323846)).add(resid.square().div(var)).mul(-0.5).reshape({n_particles});
  };
  const auto pf = particle_filter(y, final_model, cfg);

  std::cout << "rows=" << y.sizes()[0] << "\n";
  std::cout << "solver_status="
            << (fit.status == PopulationSolverStatus::Converged
                    ? "Converged"
                    : (fit.status == PopulationSolverStatus::ReachedIterationBudget ? "ReachedIterationBudget"
                                                                                      : "ReachedEvaluationBudget"))
            << "\n";
  std::cout << "objective=" << fit.minimum << " evals=" << fit.diagnostics.evaluations
            << " iters=" << fit.diagnostics.iterations << "\n";
  std::cout << "particle_loglik=" << pf.loglik << "\n";
  std::cout << "kappa=" << kappa << "\n";
  std::cout << "theta=" << theta << "\n";
  std::cout << "sigma=" << sigma << "\n";
  std::cout << "mu=" << mu << "\n";
  std::cout << "v0=" << v0 << "\n";
  std::cout << "filtered_variance_mean_path=" << pf.filtered_mean.reshape({pf.filtered_mean.sizes()[0]}) << "\n";
  return 0;
}
