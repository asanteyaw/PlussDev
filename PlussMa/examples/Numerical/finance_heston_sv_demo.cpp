#include <pluss/ma.h>

#include <cstdint>
#include <iostream>
#include <vector>

int main() {
  namespace pricing = pluss::subsp::math::finance::pricing;
  namespace vol = pluss::subsp::math::finance::volatility;
  namespace cal = pluss::subsp::math::finance::calibration;

  const auto cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  const double spot = 100.0;
  const double rate = 0.015;
  const double q = 0.0;

  vol::HestonParams hp;
  hp.kappa = 1.8;
  hp.theta = 0.05;
  hp.sigma = 0.28;
  hp.rho = -0.65;
  hp.v0 = 0.045;

  pricing::HestonIntegrationOptions int_opt;
  int_opt.integration_steps = 600;
  int_opt.integration_u_max = 120.0;

  const double h_call = pricing::heston_price(pricing::OptionType::Call, spot, 100.0, rate, q, 1.0, hp, int_opt);

  pricing::MonteCarloOptions mc;
  mc.paths = 12000;
  mc.steps = 120;
  mc.seed = 20260329ULL;

  const double h_mc = pricing::heston_price_mc(pricing::OptionType::Call, spot, 100.0, rate, q, 1.0, hp, mc, cfg);

  vol::SvParams sp;
  sp.mu = 0.0;
  sp.logvar_mean = -3.2;
  sp.phi = 0.97;
  sp.vol_of_logvar = 0.24;
  sp.rho = -0.55;
  sp.logvar0 = -3.1;
  const double sv_mc = pricing::sv_price_mc(pricing::OptionType::Call, spot, 100.0, rate, q, 1.0, sp, mc, cfg);

  std::cout << "Heston/SV pricing\n";
  std::cout << "heston_cf_call=" << h_call << "\n";
  std::cout << "heston_mc_call=" << h_mc << "\n";
  std::cout << "sv_mc_call=" << sv_mc << "\n";

  std::vector<cal::OptionQuote> quotes;
  for (double k : std::vector<double>{90.0, 100.0, 110.0}) {
    cal::OptionQuote qrow;
    qrow.type = pricing::OptionType::Call;
    qrow.strike = k;
    qrow.maturity = 1.0;
    qrow.market_price = pricing::heston_price(pricing::OptionType::Call, spot, k, rate, q, 1.0, hp, int_opt);
    qrow.weight = 1.0;
    quotes.push_back(qrow);
  }

  vol::HestonParams init = hp;
  init.kappa = 3.0;
  init.theta = 0.09;
  init.sigma = 0.45;
  init.rho = -0.2;
  init.v0 = 0.09;

  cal::HestonCalibOptions copt;
  copt.method = cal::HestonCalibMethod::CharFn;
  copt.random_starts = 3;
  copt.max_iterations = 40;
  copt.integration = int_opt;
  copt.seed = 20260330ULL;

  const auto fit = cal::calibrate_heston(quotes, spot, rate, q, init, copt, cfg);

  std::cout << "\nHeston calibration\n";
  std::cout << "objective=" << fit.objective << " rmse=" << fit.rmse << " evals=" << fit.evaluations
            << " converged=" << fit.converged << "\n";
  std::cout << "kappa=" << fit.params.kappa << " theta=" << fit.params.theta << " sigma=" << fit.params.sigma
            << " rho=" << fit.params.rho << " v0=" << fit.params.v0 << "\n";

  const auto hp_path = vol::heston_paths(60, 5, spot, 1.0 / 252.0, hp, cfg, 20260331ULL);
  std::cout << "\nHeston path samples\n";
  std::cout << "spot=" << hp_path.spot << "\n";
  std::cout << "variance=" << hp_path.variance << "\n";

  return 0;
}
