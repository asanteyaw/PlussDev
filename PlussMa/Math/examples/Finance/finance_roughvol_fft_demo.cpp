#include <pluss/ma.h>

#include <iostream>

int main() {
  namespace fin = pluss::subsp::math::finance;

  const auto cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);
  const auto inst = fin::instrument::Vanilla{
      .type = fin::pricing::OptionType::Call,
      .strike = pluss::series::full({1}, 100.0, cfg),
      .maturity = pluss::series::full({1}, 1.0, cfg),
  };
  const auto mkt = fin::market::State{
      .spot = pluss::series::full({1}, 100.0, cfg),
      .rate = pluss::series::full({1}, 0.03, cfg),
      .dividend_yield = pluss::series::full({1}, 0.01, cfg),
  };

  const auto bates = fin::model::Bates{
      .kappa = pluss::series::full({1}, 1.5, cfg),
      .theta = pluss::series::full({1}, 0.04, cfg),
      .sigma = pluss::series::full({1}, 0.3, cfg),
      .rho = pluss::series::full({1}, -0.7, cfg),
      .v0 = pluss::series::full({1}, 0.04, cfg),
      .jump_intensity = pluss::series::full({1}, 0.15, cfg),
      .jump_mean = pluss::series::full({1}, -0.08, cfg),
      .jump_vol = pluss::series::full({1}, 0.20, cfg),
  };
  const auto fft = fin::pricer::Fft{
      .num_fft = 4096,
      .du = 0.01,
      .alpha = 1.5,
  };
  std::cout << "bates_fft_price=" << fft.price(inst, bates, mkt, fin::pricer::RiskSpec::PriceOnly, cfg).price << "\n";

  const auto rh = fin::model::RoughHeston{
      .kappa = pluss::series::full({1}, 1.5, cfg),
      .theta = pluss::series::full({1}, 0.04, cfg),
      .sigma = pluss::series::full({1}, 0.3, cfg),
      .alpha = pluss::series::full({1}, -0.2, cfg),
      .rho = pluss::series::full({1}, -0.7, cfg),
      .v0 = pluss::series::full({1}, 0.04, cfg),
  };
  const auto rb = fin::model::RoughBergomi{
      .xi = pluss::series::full({1}, 0.04, cfg),
      .eta = pluss::series::full({1}, 1.6, cfg),
      .alpha = pluss::series::full({1}, -0.4, cfg),
      .rho = pluss::series::full({1}, -0.7, cfg),
  };
  const auto mc = fin::pricer::MonteCarlo{
      .paths = 20000,
      .steps = 252,
      .antithetic = true,
      .seed = 42,
  };
  std::cout << "rough_heston_mc_price=" << mc.price(inst, rh, mkt, fin::pricer::RiskSpec::PriceOnly, cfg).price << "\n";
  std::cout << "rough_bergomi_mc_price=" << mc.price(inst, rb, mkt, fin::pricer::RiskSpec::PriceOnly, cfg).price << "\n";
  return 0;
}
