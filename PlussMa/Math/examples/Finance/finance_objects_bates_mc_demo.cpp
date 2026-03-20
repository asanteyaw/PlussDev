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
  const auto mdl = obj::model::Bates{
      .kappa = pluss::full({1}, 1.5, cfg),
      .theta = pluss::full({1}, 0.04, cfg),
      .sigma = pluss::full({1}, 0.32, cfg),
      .rho = pluss::full({1}, -0.60, cfg),
      .v0 = pluss::full({1}, 0.04, cfg),
      .jump_intensity = pluss::full({1}, 0.12, cfg),
      .jump_mean = pluss::full({1}, -0.08, cfg),
      .jump_vol = pluss::full({1}, 0.20, cfg),
  };

  obj::pricer::MonteCarlo mc;
  mc.options.paths = 100000;
  mc.options.steps = 252;
  mc.options.antithetic = true;
  mc.options.seed = 77;

  const auto out = obj::price(inst, mdl, mkt, mc, cfg);
  std::cout << "bates_mc_price=" << out.price << "\n";
  return 0;
}
