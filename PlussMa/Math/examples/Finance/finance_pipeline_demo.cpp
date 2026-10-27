#include <pluss/ma.h>

#include <iostream>

int main() {
  namespace fin = pluss::subsp::math::finance;
  namespace mod = pluss::subsp::math::finance::module;

  const auto cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  const auto strikes = pluss::series::linspace(80.0, 120.0, 9, cfg);
  const auto maturities = pluss::series::full({9}, 1.0, cfg);
  const auto spots = pluss::series::full({9}, 100.0, cfg);
  const auto rates = pluss::series::full({9}, 0.02, cfg);
  const auto divs = pluss::series::full({9}, 0.00, cfg);

  mod::Field pipe{
      .instrument = fin::instrument::Vanilla{
          .type = fin::pricing::OptionType::Call,
          .strike = strikes,
          .maturity = maturities,
      },
      .market = fin::market::State{
          .spot = spots,
          .rate = rates,
          .dividend_yield = divs,
      },
      .model = fin::model::Heston{
          .kappa = pluss::series::full({1}, 1.5, cfg),
          .theta = pluss::series::full({1}, 0.04, cfg),
          .sigma = pluss::series::full({1}, 0.30, cfg),
          .rho = pluss::series::full({1}, -0.70, cfg),
          .v0 = pluss::series::full({1}, 0.04, cfg),
      },
      .controls = mod::Controls{
          .engine = mod::Engine::Cos,
          .risk = mod::Risk::All,
          .loss = mod::Loss::Huber,
          .huber_delta = 0.05,
      },
  };

  const auto out = pipe.price(cfg);
  std::cout << "price:\n" << out.price << "\n";
  std::cout << "delta:\n" << out.greeks.delta << "\n";

  const auto market_px = out.price;
  std::cout << "self loss: " << pipe.loss(market_px, cfg) << "\n";

  auto shifted = pipe.model.parameters(cfg).to(pluss::pCPU, pluss::ScalarType::Float64).host<double>();
  shifted[0] = 1.3;
  shifted[1] = 0.05;
  shifted[2] = 0.34;
  shifted[3] = -0.60;
  shifted[4] = 0.05;
  pipe.model.parameters(shifted, cfg);
  std::cout << "objective(shifted): " << pipe.objective(market_px, cfg) << "\n";

  pipe.controls.engine = mod::Engine::MonteCarlo;
  pipe.controls.risk = mod::Risk::PriceOnly;
  pipe.controls.mc.paths = 5000;
  pipe.controls.mc.steps = 64;
  pipe.controls.mc.seed = 42;
  pipe.simulate(cfg);
  std::cout << "paths.asset:\n" << pipe.paths.asset << "\n";
  std::cout << "paths.vol:\n" << pipe.paths.vol << "\n";
  return 0;
}
