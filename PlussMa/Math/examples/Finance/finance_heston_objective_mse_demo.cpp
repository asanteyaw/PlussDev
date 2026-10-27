#include <pluss/ma.h>

#include <iostream>

int main() {
  namespace fin = pluss::subsp::math::finance;

  const auto cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  const auto inst = fin::instrument::Vanilla{
      .type = fin::pricing::OptionType::Call,
      .strike = pluss::series::linspace(80.0, 120.0, 9, cfg),
      .maturity = pluss::series::full({9}, 1.0, cfg),
  };
  const auto mkt = fin::market::State{
      .spot = pluss::series::full({9}, 100.0, cfg),
      .rate = pluss::series::full({9}, 0.03, cfg),
      .dividend_yield = pluss::series::full({9}, 0.01, cfg),
  };

  const auto p0 = fin::model::Heston{
      .kappa = pluss::series::full({1}, 1.5, cfg),
      .theta = pluss::series::full({1}, 0.04, cfg),
      .sigma = pluss::series::full({1}, 0.30, cfg),
      .rho = pluss::series::full({1}, -0.70, cfg),
      .v0 = pluss::series::full({1}, 0.04, cfg),
  };

  const auto fft = fin::pricer::Fft{
      .num_fft = 4096,
      .du = 0.01,
      .alpha = 1.5,
  };

  const auto market_price = fft.price(inst, p0, mkt, fin::pricer::RiskSpec::PriceOnly, cfg).price.mul(1.015);
  const auto objective = [&](const fin::model::Heston& p) {
    const auto px = fft.price(inst, p, mkt, fin::pricer::RiskSpec::PriceOnly, cfg).price;
    return pluss::losses::mse(px, market_price);
  };

  std::cout << "market_price=" << market_price << "\n";
  std::cout << "objective_mse_at_p0=" << objective(p0) << "\n";
  return 0;
}
