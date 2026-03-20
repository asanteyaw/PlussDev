#include <pluss/ma.h>

#include <iostream>

int main() {
  namespace obj = pluss::subsp::math::finance::object;
  namespace prc = pluss::subsp::math::finance::pricing;

  const auto cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  const auto inst = obj::Vanilla{
      .type = prc::OptionType::Call,
      .strike = pluss::full({7}, 100.0, cfg),
      .maturity = pluss::full({7}, 1.0, cfg),
  };
  const auto mkt = obj::MarketState{
      .spot = pluss::linspace(70.0, 130.0, 7, cfg),
      .rate = pluss::full({1}, 0.03, cfg),
      .dividend_yield = pluss::full({1}, 0.01, cfg),
  };
  const auto mdl = obj::model::BlackScholes{
      .volatility = pluss::full({7}, 0.22, cfg),
  };

  const auto out = obj::price(inst, mdl, mkt, obj::pricer::Analytic{.with_greeks = true}, cfg);

  std::cout << "spot=" << mkt.spot << "\n";
  std::cout << "price=" << out.price << "\n";
  std::cout << "delta=" << out.delta << "\n";
  std::cout << "gamma=" << out.gamma << "\n";
  std::cout << "vega=" << out.vega << "\n";
  std::cout << "theta=" << out.theta << "\n";
  std::cout << "rho=" << out.rho << "\n";
  return 0;
}
