#include <pluss/ma.h>

#include <algorithm>
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

using pluss::Config;
using pluss::ScalarType;
using pluss::Series;

void expect(bool condition, const std::string& message) {
  if (!condition) {
    throw std::runtime_error(message);
  }
}

Config f64_cpu() {
  return Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);
}

Series row_index(std::int64_t row) {
  return pluss::make_series(std::vector<std::int64_t>{row}, {1},
                            Config{}.with_dtype(ScalarType::Int64).with_platform(pluss::pCPU));
}

Series terminal_row(const Series& x) {
  expect(x.sizes().size() >= 2, "terminal_row expects rank >= 2");
  return x.index_select(0, row_index(x.sizes()[0] - 1));
}

double mean_all(const Series& x) {
  return x.to(pluss::pCPU, ScalarType::Float64).mean().item<double>();
}

double var_all(const Series& x) {
  const Series y = x.to(pluss::pCPU, ScalarType::Float64);
  const Series m = y.mean();
  return y.sub(m).square().mean().item<double>();
}

double tol_mean(double var, std::int64_t n, double floor = 7e-4) {
  return std::max(floor, 6.0 * std::sqrt(std::max(var, 0.0) / static_cast<double>(n)));
}

double tol_var(double var, std::int64_t n, double floor = 2e-3) {
  return std::max(floor, 8.0 * var * std::sqrt(2.0 / static_cast<double>(n)));
}

struct CirMoments {
  double mean = 0.0;
  double var = 0.0;
};

CirMoments cir_transition_moments(double x0, double dt, double kappa, double theta, double sigma) {
  const double e = std::exp(-kappa * dt);
  const double one_minus = 1.0 - e;
  CirMoments out;
  out.mean = theta + (x0 - theta) * e;
  out.var = x0 * sigma * sigma * e * one_minus / kappa + theta * sigma * sigma * one_minus * one_minus / (2.0 * kappa);
  return out;
}

std::vector<double> hjm_alpha(const std::vector<double>& tenors, const std::vector<double>& vol) {
  const std::size_t n = tenors.size();
  std::vector<double> alpha(n, 0.0);
  for (std::size_t j = 0; j < n; ++j) {
    double tail = 0.0;
    const double base_t = (j == 0 ? 0.0 : tenors[j - 1]);
    for (std::size_t k = j; k < n; ++k) {
      const double prev_t = (k == 0 ? 0.0 : tenors[k - 1]);
      const double dtau = tenors[k] - std::max(prev_t, base_t);
      if (dtau > 0.0) {
        tail += vol[k] * dtau;
      }
    }
    alpha[j] = vol[j] * tail;
  }
  return alpha;
}

void test_cir_transition_matches_exact_moments() {
  namespace rate = pluss::subsp::math::finance::rates;
  const auto cfg = f64_cpu();

  const std::int64_t paths_hi = 120000;
  const double dt = 0.25;

  rate::CirParams hi;
  hi.kappa = 1.2;
  hi.theta = 0.06;
  hi.sigma = 0.2;
  hi.r0 = 0.04;
  hi.rate_floor = 1e-12;

  rate::SimOptions opt;
  opt.scheme = rate::Scheme::Transition;
  opt.seed = 2026031501ULL;
  const auto out_hi = rate::cir_paths(1, paths_hi, dt, hi, opt, cfg, opt.seed);
  const Series last_hi = terminal_row(out_hi.rate);
  const auto m_hi = cir_transition_moments(hi.r0, dt, hi.kappa, hi.theta, hi.sigma);
  const double mean_hi = mean_all(last_hi);
  const double var_hi = var_all(last_hi);
  expect(std::abs(mean_hi - m_hi.mean) <= tol_mean(m_hi.var, paths_hi),
         "CIR transition (degree>1) mean mismatch vs exact transition moments");
  expect(std::abs(var_hi - m_hi.var) <= tol_var(m_hi.var, paths_hi),
         "CIR transition (degree>1) variance mismatch vs exact transition moments");

  const std::int64_t paths_lo = 90000;
  rate::CirParams lo;
  lo.kappa = 0.7;
  lo.theta = 0.01;
  lo.sigma = 0.35;
  lo.r0 = 0.03;
  lo.rate_floor = 1e-12;

  opt.seed = 2026031502ULL;
  const auto out_lo = rate::cir_paths(1, paths_lo, dt, lo, opt, cfg, opt.seed);
  const Series last_lo = terminal_row(out_lo.rate);
  const auto m_lo = cir_transition_moments(lo.r0, dt, lo.kappa, lo.theta, lo.sigma);
  const double mean_lo = mean_all(last_lo);
  const double var_lo = var_all(last_lo);
  expect(std::abs(mean_lo - m_lo.mean) <= tol_mean(m_lo.var, paths_lo, 1.4e-3),
         "CIR transition (degree<=1) mean mismatch vs exact transition moments");
  expect(std::abs(var_lo - m_lo.var) <= tol_var(m_lo.var, paths_lo, 3.2e-3),
         "CIR transition (degree<=1) variance mismatch vs exact transition moments");
}

void test_vasicek_hullwhite_transition_moments() {
  namespace rate = pluss::subsp::math::finance::rates;
  const auto cfg = f64_cpu();
  const std::int64_t paths = 120000;
  const double dt = 0.1;

  rate::VasicekParams vp;
  vp.kappa = 0.85;
  vp.theta = 0.03;
  vp.sigma = 0.012;
  vp.r0 = 0.025;

  rate::SimOptions opt;
  opt.scheme = rate::Scheme::Transition;
  opt.seed = 2026031503ULL;
  const auto vout = rate::vasicek_paths(1, paths, dt, vp, opt, cfg, opt.seed);
  const Series vlast = terminal_row(vout.rate);
  const auto vm = cir_transition_moments(vp.r0, dt, vp.kappa, vp.theta, vp.sigma);
  expect(std::abs(mean_all(vlast) - vm.mean) <= tol_mean(vm.var, paths),
         "Vasicek transition mean mismatch");
  expect(std::abs(var_all(vlast) - vm.var) <= tol_var(vm.var, paths),
         "Vasicek transition variance mismatch");

  rate::HullWhiteParams hp;
  hp.kappa = vp.kappa;
  hp.sigma = vp.sigma;
  hp.r0 = vp.r0;
  hp.theta0 = vp.theta;
  hp.theta_slope = 0.0;
  opt.seed = 2026031504ULL;
  const auto hout = rate::hull_white_paths(1, paths, dt, hp, opt, cfg, opt.seed);
  const Series hlast = terminal_row(hout.rate);
  expect(std::abs(mean_all(hlast) - vm.mean) <= tol_mean(vm.var, paths),
         "Hull-White transition mean mismatch");
  expect(std::abs(var_all(hlast) - vm.var) <= tol_var(vm.var, paths),
         "Hull-White transition variance mismatch");
}

void test_heston_and_bates_transition_variance_moments() {
  namespace vol = pluss::subsp::math::finance::volatility;
  const auto cfg = f64_cpu();
  const std::int64_t paths = 110000;
  const double dt = 0.25;

  vol::HestonParams hp;
  hp.kappa = 1.4;
  hp.theta = 0.05;
  hp.sigma = 0.35;
  hp.rho = -0.6;
  hp.v0 = 0.04;
  hp.mu = 0.01;

  vol::SimOptions opt;
  opt.scheme = vol::SimScheme::Transition;
  opt.seed = 2026031505ULL;
  const auto hout = vol::detail::heston_paths(1, paths, 100.0, dt, hp, opt, cfg, opt.seed);
  const Series hlast = terminal_row(hout.variance);
  const auto hm = cir_transition_moments(hp.v0, dt, hp.kappa, hp.theta, hp.sigma);
  expect(std::abs(mean_all(hlast) - hm.mean) <= tol_mean(hm.var, paths),
         "Heston transition variance mean mismatch");
  expect(std::abs(var_all(hlast) - hm.var) <= tol_var(hm.var, paths),
         "Heston transition variance variance mismatch");

  vol::BatesParams bp;
  bp.kappa = hp.kappa;
  bp.theta = hp.theta;
  bp.sigma = hp.sigma;
  bp.rho = hp.rho;
  bp.v0 = hp.v0;
  bp.mu = hp.mu;
  bp.jump_intensity = 0.2;
  bp.jump_mean = -0.08;
  bp.jump_vol = 0.2;

  opt.seed = 2026031506ULL;
  const auto bout = vol::detail::bates_paths(1, paths, 100.0, dt, bp, opt, cfg, opt.seed);
  const Series blast = terminal_row(bout.variance);
  expect(std::abs(mean_all(blast) - hm.mean) <= tol_mean(hm.var, paths),
         "Bates transition variance mean mismatch");
  expect(std::abs(var_all(blast) - hm.var) <= tol_var(hm.var, paths),
         "Bates transition variance variance mismatch");

  opt.scheme = vol::SimScheme::QuadExp;
  opt.seed = 2026031507ULL;
  const auto hqe = vol::detail::heston_paths(1, paths, 100.0, dt, hp, opt, cfg, opt.seed);
  const Series hqelast = terminal_row(hqe.variance);
  expect(std::abs(mean_all(hqelast) - hm.mean) <= tol_mean(hm.var, paths, 1.2e-3),
         "Heston QE variance mean mismatch");
  expect(std::abs(var_all(hqelast) - hm.var) <= tol_var(hm.var, paths, 3.0e-3),
         "Heston QE variance variance mismatch");
}

void test_hjm_drift_matches_matlab_formulation() {
  namespace rate = pluss::subsp::math::finance::rates;
  const auto cfg = f64_cpu();
  const std::int64_t paths = 130000;
  const double dt = 1.0 / 252.0;

  rate::HjmParams p;
  p.r0 = 0.03;
  p.tenors = {0.25, 0.5, 1.0, 2.0, 3.0};
  p.vol = {0.012, 0.011, 0.010, 0.009, 0.008};

  rate::SimOptions opt;
  opt.seed = 2026031508ULL;
  const auto out = rate::hjm_paths(1, paths, dt, p, opt, cfg, opt.seed);

  const std::int64_t k = static_cast<std::int64_t>(p.tenors.size());
  const Series f1 = out.forward.index_select(0, row_index(1)).reshape({paths, k});
  const std::vector<double> mean_f = f1.mean(0, false).to(pluss::pCPU, ScalarType::Float64).host<double>();
  const std::vector<double> alpha = hjm_alpha(p.tenors, p.vol);

  for (std::int64_t j = 0; j < k; ++j) {
    const double target = p.r0 + alpha[static_cast<std::size_t>(j)] * dt;
    const double vol_j = p.vol[static_cast<std::size_t>(j)];
    const double var = vol_j * vol_j * dt;
    expect(std::abs(mean_f[static_cast<std::size_t>(j)] - target) <= tol_mean(var, paths, 9e-4),
           "HJM forward drift mismatch vs alpha(t,T) formulation");
  }
}

} // namespace

int main() {
  try {
    test_cir_transition_matches_exact_moments();
    test_vasicek_hullwhite_transition_moments();
    test_heston_and_bates_transition_variance_moments();
    test_hjm_drift_matches_matlab_formulation();
    std::cout << "simulator_matlab_parity_test passed\n";
    return 0;
  } catch (const std::exception& e) {
    std::cerr << "simulator_matlab_parity_test failed: " << e.what() << '\n';
    return 1;
  }
}
