#include <pluss/ma.h>

#include <iostream>

int main() {
  namespace obj = pluss::subsp::math::finance::object;
  namespace prc = pluss::subsp::math::finance::pricing;

  const auto cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  const auto inst = obj::Vanilla{
      .type = prc::OptionType::Put,
      .strike = pluss::series::full({5}, 105.0, cfg),
      .maturity = pluss::series::full({5}, 0.75, cfg),
  };
  const auto mkt = obj::MarketState{
      .spot = pluss::series::linspace(90.0, 120.0, 5, cfg),
      .rate = pluss::series::full({1}, 0.025, cfg),
      .dividend_yield = pluss::series::full({1}, 0.005, cfg),
  };
  const auto mdl = obj::model::BlackScholes{
      .volatility = pluss::series::full({5}, 0.20, cfg),
  };

  obj::pricer::MonteCarlo mc;
  mc.options.paths = 80000;
  mc.options.antithetic = true;
  mc.options.seed = 42;

  const auto out_mc = obj::price(inst, mdl, mkt, mc, cfg);
  const auto out_an = obj::price(inst, mdl, mkt, obj::pricer::Analytic{.with_greeks = false}, cfg);

  std::cout << "price_mc=" << out_mc.price << "\n";
  std::cout << "price_analytic=" << out_an.price << "\n";
  std::cout << "mc_minus_analytic=" << out_mc.price.sub(out_an.price) << "\n";
  return 0;
}
