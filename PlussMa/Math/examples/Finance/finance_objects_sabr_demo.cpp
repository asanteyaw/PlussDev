#include <pluss/ma.h>

#include <iostream>

int main() {
  namespace fin = pluss::subsp::math::finance;
  const auto cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  const fin::instrument::Vanilla inst{
      .type = fin::pricing::OptionType::Call,
      .strike = pluss::series::linspace(80.0, 120.0, 9, cfg),
      .maturity = pluss::series::full({9}, 1.0, cfg),
  };

  const fin::market::State mkt{
      .spot = pluss::series::full({9}, 100.0, cfg),
      .rate = pluss::series::full({9}, 0.02, cfg),
      .dividend_yield = pluss::series::full({9}, 0.0, cfg),
  };

  const fin::model::Sabr sabr{
      .alpha0 = pluss::series::full({1}, 0.20, cfg),
      .beta = pluss::series::full({1}, 0.50, cfg),
      .nu = pluss::series::full({1}, 0.40, cfg),
      .rho = pluss::series::full({1}, -0.30, cfg),
      .vol_type = fin::pricing::SabrVolType::Lognormal,
      .approx = fin::pricing::SabrApproxOptions{
          .model = fin::pricing::SabrApproxModel::Hagan2002,
          .shift = 0.0,
      },
  };

  fin::pricer::Analytic an;
  const auto px_approx = an.price(inst, sabr, mkt, fin::pricer::RiskSpec::PriceOnly, cfg).price;
  std::cout << "sabr_approx_price=" << px_approx << "\n";

  fin::pricer::MonteCarlo mc;
  mc.paths = 30000;
  mc.steps = 252;
  mc.antithetic = true;
  mc.seed = 7;
  const auto px_mc = mc.price(inst, sabr, mkt, fin::pricer::RiskSpec::PriceOnly, cfg).price;
  std::cout << "sabr_mc_price=" << px_mc << "\n";
  return 0;
}
