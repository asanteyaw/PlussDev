#include <pluss/ma.h>

#include <iostream>
#include <vector>

int main() {
  namespace cal = pluss::subsp::math::finance::calibration;
  namespace mdl = pluss::subsp::math::finance::models;
  namespace prc = pluss::subsp::math::finance::pricing;
  namespace vol = pluss::subsp::math::finance::volatility;

  const auto cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  vol::BatesParams bp;
  bp.kappa = 1.8;
  bp.theta = 0.05;
  bp.sigma = 0.28;
  bp.rho = -0.62;
  bp.v0 = 0.045;
  bp.jump_intensity = 0.2;
  bp.jump_mean = -0.08;
  bp.jump_vol = 0.20;

  prc::MonteCarloOptions mc;
  mc.paths = 1200;
  mc.steps = 32;
  mc.scheme = vol::SimScheme::QuadExp;
  mc.rand_method = vol::RandMethod::Sobol;
  mc.randomized_quasi = true;
  mc.sobol_scrambling = 3;
  mc.seed = 20260641ULL;

  const double f_px = prc::bates_price_mc(prc::OptionType::Call, 100.0, 100.0, 0.02, 0.0, 1.0, bp, mc, cfg);
  const auto f_paths = vol::bates_paths(80, 6, 100.0, 1.0 / 252.0, bp, cfg, 20260642ULL);

  mdl::Bates bates{bp};
  const double o_px = bates.price_mc(prc::OptionType::Call, 100.0, 100.0, 0.02, 0.0, 1.0, mc, cfg);

  std::vector<cal::OptionQuote> quotes;
  for (double k : {90.0, 100.0, 110.0}) {
    cal::OptionQuote q;
    q.type = prc::OptionType::Call;
    q.strike = k;
    q.maturity = 1.0;
    q.market_price = bates.price_mc(q.type, 100.0, q.strike, 0.02, 0.0, q.maturity, mc, cfg);
    q.weight = 1.0;
    quotes.push_back(q);
  }

  cal::BatesCalibOptions copt;
  copt.random_starts = 1;
  copt.max_iterations = 4;
  copt.seed = 20260643ULL;
  copt.mc = mc;
  const auto fit = bates.calibrate_inplace(quotes, 100.0, 0.02, 0.0, copt, cfg);

  std::cout << "Bates functional\n";
  std::cout << "price_mc=" << f_px << "\n";
  std::cout << "paths.spot=" << f_paths.spot << "\n";
  std::cout << "paths.variance=" << f_paths.variance << "\n";

  std::cout << "\nBates object\n";
  std::cout << "price_mc=" << o_px << "\n";
  std::cout << "calibrate.objective=" << fit.objective << " rmse=" << fit.rmse << "\n";
  std::cout << "params=" << bates.params().kappa << ", " << bates.params().theta << ", " << bates.params().sigma
            << ", " << bates.params().rho << ", " << bates.params().v0 << ", " << bates.params().jump_intensity
            << ", " << bates.params().jump_mean << ", " << bates.params().jump_vol << "\n";
  return 0;
}
