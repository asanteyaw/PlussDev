#include <pluss/ma.h>

#include <iostream>
#include <vector>

int main() {
  const auto cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  pluss::subsp::math::finance::volatility::HestonParams hp;
  hp.kappa = 1.8;
  hp.theta = 0.05;
  hp.sigma = 0.28;
  hp.rho = -0.65;
  hp.v0 = 0.045;

  pluss::subsp::math::finance::models::Heston heston{hp};

  pluss::subsp::math::finance::pricing::HestonIntegrationOptions int_opt;
  int_opt.integration_steps = 300;
  int_opt.integration_u_max = 120.0;

  const double h_cf = heston.price(
      pluss::subsp::math::finance::pricing::OptionType::Call,
      100.0,
      100.0,
      0.02,
      0.0,
      1.0,
      int_opt);

  pluss::subsp::math::finance::pricing::MonteCarloOptions mc;
  mc.paths = 12000;
  mc.steps = 120;
  mc.scheme = pluss::subsp::math::finance::volatility::SimScheme::QuadExp;
  mc.rand_method = pluss::subsp::math::finance::volatility::RandMethod::Sobol;
  mc.randomized_quasi = true;
  mc.sobol_scrambling = 3;
  mc.offset = 256;
  mc.seed = 20260421ULL;

  const double h_mc = heston.price_mc(
      pluss::subsp::math::finance::pricing::OptionType::Call,
      100.0,
      100.0,
      0.02,
      0.0,
      1.0,
      mc,
      cfg);

  pluss::subsp::math::finance::volatility::SimOptions sim_opt;
  sim_opt.scheme = pluss::subsp::math::finance::volatility::SimScheme::Milstein;
  sim_opt.method = pluss::subsp::math::finance::volatility::RandMethod::Pseudo;
  sim_opt.seed = 20260422ULL;
  const auto h_paths = heston.paths(80, 6, 100.0, 1.0 / 252.0, sim_opt, cfg, sim_opt.seed);

  std::vector<pluss::subsp::math::finance::calibration::OptionQuote> quotes;
  for (double k : {90.0, 100.0, 110.0}) {
    pluss::subsp::math::finance::calibration::OptionQuote q;
    q.type = pluss::subsp::math::finance::pricing::OptionType::Call;
    q.strike = k;
    q.maturity = 1.0;
    q.market_price = heston.price(q.type, 100.0, q.strike, 0.02, 0.0, q.maturity, int_opt);
    q.weight = 1.0;
    quotes.push_back(q);
  }

  pluss::subsp::math::finance::calibration::HestonCalibOptions hcal;
  hcal.random_starts = 3;
  hcal.max_iterations = 20;
  hcal.integration = int_opt;
  hcal.seed = 20260423ULL;

  const auto h_fit = heston.calibrate_inplace(quotes, 100.0, 0.02, 0.0, hcal, cfg);

  const pluss::subsp::math::finance::models::StochVolBase& base = heston;
  const double h_base_mc = base.price_mc(
      pluss::subsp::math::finance::pricing::OptionType::Call,
      100.0,
      100.0,
      0.02,
      0.0,
      1.0,
      mc,
      cfg);

  pluss::subsp::math::finance::volatility::SvParams sp;
  sp.mu = 0.0;
  sp.logvar_mean = -3.2;
  sp.phi = 0.97;
  sp.vol_of_logvar = 0.24;
  sp.rho = -0.55;
  sp.logvar0 = -3.1;

  pluss::subsp::math::finance::models::Sv sv{sp};

  mc.rand_method = pluss::subsp::math::finance::volatility::RandMethod::Halton;
  mc.seed = 20260424ULL;
  const double sv_mc = sv.price(
      pluss::subsp::math::finance::pricing::OptionType::Call,
      100.0,
      100.0,
      0.02,
      0.0,
      1.0,
      mc,
      cfg);

  sim_opt.method = pluss::subsp::math::finance::volatility::RandMethod::Torus;
  sim_opt.seed = 20260425ULL;
  const auto sv_paths = sv.paths(80, 6, 100.0, 1.0 / 252.0, sim_opt, cfg, sim_opt.seed);

  std::cout << "Heston object\n";
  std::cout << "price(cf)=" << h_cf << "\n";
  std::cout << "price_mc=" << h_mc << "\n";
  std::cout << "base.price_mc=" << h_base_mc << "\n";
  std::cout << "paths.spot=" << h_paths.spot << "\n";
  std::cout << "paths.variance=" << h_paths.variance << "\n";
  std::cout << "calibrate.objective=" << h_fit.objective << " rmse=" << h_fit.rmse << "\n";
  std::cout << "params=" << heston.params().kappa << ", " << heston.params().theta << ", " << heston.params().sigma
            << ", " << heston.params().rho << ", " << heston.params().v0 << "\n";

  std::cout << "\nSV object\n";
  std::cout << "price_mc=" << sv_mc << "\n";
  std::cout << "paths.spot=" << sv_paths.spot << "\n";
  std::cout << "paths.variance=" << sv_paths.variance << "\n";

  pluss::subsp::math::finance::volatility::HestonNandiParams hnp;
  hnp.omega = 1.0e-6;
  hnp.alpha = 2.0e-5;
  hnp.beta = 0.92;
  hnp.gamma = 5.0;
  hnp.lambda = 0.0;
  hnp.r = 0.0;
  hnp.initial_variance = 0.00015;

  pluss::subsp::math::finance::models::HestonNandi hn{hnp};
  mc.rand_method = pluss::subsp::math::finance::volatility::RandMethod::Sobol;
  mc.randomized_quasi = true;
  mc.sobol_scrambling = 3;
  mc.seed = 20260431ULL;
  const double hn_mc = hn.price_mc(
      pluss::subsp::math::finance::pricing::OptionType::Call,
      100.0,
      100.0,
      0.02,
      0.0,
      1.0,
      mc,
      cfg);
  const double hn_cf = hn.price(
      pluss::subsp::math::finance::pricing::OptionType::Call,
      100.0,
      100.0,
      0.02,
      0.0,
      1.0);

  sim_opt.method = pluss::subsp::math::finance::volatility::RandMethod::Pseudo;
  sim_opt.seed = 20260432ULL;
  const auto hn_paths = hn.paths(80, 6, 100.0, 1.0 / 252.0, sim_opt, cfg, sim_opt.seed);

  pluss::subsp::math::finance::calibration::HestonNandiCalibOptions hn_cal;
  hn_cal.random_starts = 2;
  hn_cal.max_iterations = 10;
  hn_cal.seed = 20260433ULL;
  hn_cal.mc = mc;
  hn_cal.mc.paths = 2500;
  hn_cal.mc.seed = 20260434ULL;
  const auto hn_fit = hn.calibrate_inplace(
      pluss::randn({300}, cfg, 20260435ULL).mul(0.01),
      quotes,
      100.0,
      0.02,
      0.0,
      hn_cal,
      cfg);

  std::cout << "\nHeston-Nandi object\n";
  std::cout << "price(cf)=" << hn_cf << "\n";
  std::cout << "price_mc=" << hn_mc << "\n";
  std::cout << "paths.spot=" << hn_paths.spot << "\n";
  std::cout << "paths.variance=" << hn_paths.variance << "\n";
  std::cout << "calibrate.objective=" << hn_fit.objective << " rmse=" << hn_fit.options_rmse << "\n";
  std::cout << "params=" << hn.params().omega << ", " << hn.params().alpha << ", " << hn.params().beta << ", "
            << hn.params().gamma << ", " << hn.params().lambda << "\n";

  pluss::subsp::math::finance::volatility::SabrParams qsp;
  qsp.alpha0 = 0.01;
  qsp.beta = 0.3;
  qsp.nu = 0.03;
  qsp.rho = -0.2;
  qsp.mu = 0.0;
  pluss::subsp::math::finance::models::Sabr sabr{qsp};
  mc.rand_method = pluss::subsp::math::finance::volatility::RandMethod::Sobol;
  mc.seed = 20260426ULL;
  const double sabr_mc = sabr.price_mc(
      pluss::subsp::math::finance::pricing::OptionType::Call,
      100.0,
      100.0,
      0.02,
      0.0,
      1.0,
      mc,
      cfg);
  sim_opt.method = pluss::subsp::math::finance::volatility::RandMethod::Pseudo;
  sim_opt.paths_first = true;
  sim_opt.seed = 20260427ULL;
  const auto sabr_paths = sabr.paths(80, 6, 100.0, 1.0 / 252.0, sim_opt, cfg, sim_opt.seed);

  pluss::subsp::math::finance::volatility::SlvParams slvp;
  slvp.mu = 0.01;
  slvp.kappa = 1.6;
  slvp.theta = 0.04;
  slvp.sigma = 0.25;
  slvp.rho = -0.55;
  slvp.v0 = 0.04;
  slvp.local_vol_form = pluss::subsp::math::finance::volatility::LocalVolForm::Cev;
  slvp.local_vol_level = 0.95;
  slvp.local_vol_curvature = 0.20;
  pluss::subsp::math::finance::models::Slv slv{slvp};
  mc.rand_method = pluss::subsp::math::finance::volatility::RandMethod::Halton;
  mc.seed = 20260428ULL;
  const double slv_mc = slv.price_mc(
      pluss::subsp::math::finance::pricing::OptionType::Call,
      100.0,
      100.0,
      0.02,
      0.0,
      1.0,
      mc,
      cfg);
  sim_opt.seed = 20260429ULL;
  const auto slv_paths = slv.paths(80, 6, 100.0, 1.0 / 252.0, sim_opt, cfg, sim_opt.seed);

  auto lv_params = slvp;
  lv_params.sigma = 1e-8;
  lv_params.kappa = 50.0;
  lv_params.theta = 1.0;
  lv_params.v0 = 1.0;
  lv_params.rho = 0.0;
  lv_params.local_vol_form = pluss::subsp::math::finance::volatility::LocalVolForm::LogAffine;
  lv_params.local_vol_level = 0.20;
  lv_params.local_vol_slope = -0.02;
  pluss::subsp::math::finance::models::Slv local_vol_like{lv_params};
  mc.rand_method = pluss::subsp::math::finance::volatility::RandMethod::Torus;
  mc.seed = 20260430ULL;
  const double lv_mc = local_vol_like.price_mc(
      pluss::subsp::math::finance::pricing::OptionType::Call,
      100.0,
      100.0,
      0.02,
      0.0,
      1.0,
      mc,
      cfg);

  std::cout << "\nSABR object\n";
  std::cout << "price_mc=" << sabr_mc << "\n";
  std::cout << "paths.forward=" << sabr_paths.spot << "\n";
  std::cout << "paths.alpha2=" << sabr_paths.variance << "\n";

  std::cout << "\nSLV object\n";
  std::cout << "price_mc=" << slv_mc << "\n";
  std::cout << "paths.spot=" << slv_paths.spot << "\n";
  std::cout << "paths.variance=" << slv_paths.variance << "\n";

  std::cout << "\nLocal-vol special case via SLV\n";
  std::cout << "price_mc=" << lv_mc << "\n";

  return 0;
}
