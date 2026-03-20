#include <pluss/ma.h>

#include <iostream>

int main() {
  namespace obj = pluss::subsp::math::finance::object;
  namespace prc = pluss::subsp::math::finance::pricing;

  const auto cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  const auto inst = obj::Vanilla{
      .type = prc::OptionType::Call,
      .strike = pluss::full({1}, 100.0, cfg),
      .maturity = pluss::full({1}, 1.0, cfg),
  };
  const auto mkt = obj::MarketState{
      .spot = pluss::full({1}, 100.0, cfg),
      .rate = pluss::full({1}, 0.03, cfg),
      .dividend_yield = pluss::full({1}, 0.01, cfg),
  };
  const auto mdl = obj::model::Heston{
      .kappa = pluss::full({1}, 1.4, cfg),
      .theta = pluss::full({1}, 0.04, cfg),
      .sigma = pluss::full({1}, 0.35, cfg),
      .rho = pluss::full({1}, -0.65, cfg),
      .v0 = pluss::full({1}, 0.04, cfg),
  };

  obj::pricer::Fft fft;
  fft.return_grid = false;
  fft.options.num_fft = 4096;
  fft.options.characteristic_fcn_step = 0.02;

  obj::pricer::MonteCarlo mc;
  mc.options.paths = 120000;
  mc.options.steps = 252;
  mc.options.antithetic = true;
  mc.options.seed = 7;

  const auto out_fft = obj::price(inst, mdl, mkt, fft, cfg);
  const auto out_mc = obj::price(inst, mdl, mkt, mc, cfg);

  std::cout << "heston_fft_price=" << out_fft.price << "\n";
  std::cout << "heston_mc_price=" << out_mc.price << "\n";
  return 0;
}
