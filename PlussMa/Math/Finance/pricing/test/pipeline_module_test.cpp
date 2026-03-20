#include <pluss/ma.h>

#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

using pluss::Config;
using pluss::ScalarType;
namespace fin = pluss::subsp::math::finance;
namespace mod = pluss::subsp::math::finance::module;

void expect(bool cond, const std::string& msg) {
  if (!cond) {
    throw std::runtime_error(msg);
  }
}

Config f64_cpu() {
  return Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);
}

void test_pipeline_heston_cos_loss_and_objective() {
  const Config cfg = f64_cpu();
  const auto strikes = pluss::linspace(80.0, 120.0, 9, cfg);
  const auto tau = pluss::full({9}, 1.0, cfg);

  mod::Field pipe{
      .instrument = fin::instrument::Vanilla{
          .type = fin::pricing::OptionType::Call,
          .strike = strikes,
          .maturity = tau,
      },
      .market = fin::market::State{
          .spot = pluss::full({9}, 100.0, cfg),
          .rate = pluss::full({9}, 0.02, cfg),
          .dividend_yield = pluss::full({9}, 0.00, cfg),
      },
      .model = fin::model::Heston{
          .kappa = pluss::full({1}, 1.5, cfg),
          .theta = pluss::full({1}, 0.04, cfg),
          .sigma = pluss::full({1}, 0.30, cfg),
          .rho = pluss::full({1}, -0.70, cfg),
          .v0 = pluss::full({1}, 0.04, cfg),
      },
      .controls = mod::Controls{
          .engine = mod::Engine::Cos,
          .risk = mod::Risk::All,
          .loss = mod::Loss::Huber,
          .huber_delta = 0.05,
      },
  };

  const auto priced = pipe.price(cfg);
  expect(priced.price.numel() == 9, "pipeline heston cos: price size mismatch");
  expect(priced.greeks.delta.defined(), "pipeline heston cos: greeks expected with Risk::All");

  const auto market_px = priced.price;
  const double fit0 = pipe.loss_value(market_px, cfg);
  expect(std::isfinite(fit0), "pipeline heston cos: loss must be finite");
  expect(std::abs(fit0) < 1e-10, "pipeline heston cos: self-fit loss must be ~0");

  auto objective = pipe.bind_objective_typed<fin::model::Heston>(market_px, cfg);
  fin::model::Heston shifted = pipe.model.as<fin::model::Heston>();
  shifted.v0 = pluss::full({1}, 0.08, cfg);
  const double fit1 = objective(shifted);
  expect(std::isfinite(fit1), "pipeline heston cos: objective must be finite");
  expect(fit1 >= fit0, "pipeline heston cos: shifted params should not improve exact self-fit");
}

void test_pipeline_black_scholes_analytic() {
  const Config cfg = f64_cpu();
  mod::Field pipe{
      .instrument = fin::instrument::Vanilla{
          .type = fin::pricing::OptionType::Call,
          .strike = pluss::full({1}, 100.0, cfg),
          .maturity = pluss::full({1}, 1.0, cfg),
      },
      .market = fin::market::State{
          .spot = pluss::full({1}, 100.0, cfg),
          .rate = pluss::full({1}, 0.02, cfg),
          .dividend_yield = pluss::full({1}, 0.00, cfg),
      },
      .model = fin::model::BlackScholes{
          .volatility = pluss::full({1}, 0.2, cfg),
      },
      .controls = mod::Controls{
          .engine = mod::Engine::Analytic,
          .risk = mod::Risk::All,
          .loss = mod::Loss::MSE,
      },
  };

  const auto priced = pipe.price(cfg);
  expect(priced.price.defined(), "pipeline bs analytic: price must be defined");
  expect(priced.greeks.gamma.defined(), "pipeline bs analytic: gamma must be defined");
}

void test_pipeline_black_scholes_auto_mixed_call_put() {
  const Config cfg = f64_cpu();
  mod::Field pipe{
      .instrument = fin::instrument::Vanilla{
          .type = fin::pricing::OptionType::Auto,
          .is_call = pluss::make_series(std::vector<double>{1.0, 0.0, 1.0, 0.0}, {4}, cfg),
          .strike = pluss::full({4}, 100.0, cfg),
          .maturity = pluss::full({4}, 1.0, cfg),
      },
      .market = fin::market::State{
          .spot = pluss::linspace(90.0, 110.0, 4, cfg),
          .rate = pluss::full({1}, 0.02, cfg),
          .dividend_yield = pluss::full({1}, 0.00, cfg),
      },
      .model = fin::model::BlackScholes{
          .volatility = pluss::full({1}, 0.2, cfg),
      },
      .controls = mod::Controls{
          .engine = mod::Engine::Analytic,
          .risk = mod::Risk::All,
          .loss = mod::Loss::MSE,
      },
  };

  const auto priced = pipe.price(cfg);
  expect(priced.price.numel() == 4, "pipeline bs auto: price size mismatch");
  expect(priced.price.isfinite().all().item<double>() == 1.0, "pipeline bs auto: price must be finite");
}

void test_pipeline_monte_carlo_paths() {
  const Config cfg = f64_cpu();
  mod::Field pipe{
      .instrument = fin::instrument::Vanilla{
          .type = fin::pricing::OptionType::Call,
          .strike = pluss::full({1}, 100.0, cfg),
          .maturity = pluss::full({1}, 1.0, cfg),
      },
      .market = fin::market::State{
          .spot = pluss::full({1}, 100.0, cfg),
          .rate = pluss::full({1}, 0.02, cfg),
          .dividend_yield = pluss::full({1}, 0.00, cfg),
      },
      .model = fin::model::Heston{
          .kappa = pluss::full({1}, 1.5, cfg),
          .theta = pluss::full({1}, 0.04, cfg),
          .sigma = pluss::full({1}, 0.30, cfg),
          .rho = pluss::full({1}, -0.70, cfg),
          .v0 = pluss::full({1}, 0.04, cfg),
      },
      .controls = mod::Controls{
          .engine = mod::Engine::MonteCarlo,
          .risk = mod::Risk::PriceOnly,
          .loss = mod::Loss::MSE,
      },
  };
  pipe.controls.mc.paths = 1024;
  pipe.controls.mc.steps = 64;
  pipe.controls.mc.seed = 77;

  pipe.simulate(cfg);
  expect(pipe.paths.asset.defined(), "pipeline mc simulate: paths.asset must be defined");
  expect(pipe.paths.vol.defined(), "pipeline mc simulate: paths.vol must be defined");
  expect(pipe.paths.variance.defined(), "pipeline mc simulate: paths.variance must be defined");
  expect(pipe.paths.asset.sizes().size() == 2, "pipeline mc simulate: asset rank mismatch");
  expect(pipe.paths.asset.sizes()[0] == 1024 && pipe.paths.asset.sizes()[1] == 65,
         "pipeline mc simulate: expected [paths, steps+1]");
  expect(pipe.paths.vol.sizes()[0] == 1024 && pipe.paths.vol.sizes()[1] == 65,
         "pipeline mc simulate: expected vol [paths, steps+1]");
  expect(pipe.paths.asset.isfinite().all().item<double>() == 1.0, "pipeline mc simulate: asset has non-finite");
  expect(pipe.paths.vol.isfinite().all().item<double>() == 1.0, "pipeline mc simulate: vol has non-finite");
}

void test_pipeline_mismatch_error_is_helpful() {
  const Config cfg = f64_cpu();
  mod::Field pipe{
      .instrument = fin::instrument::Vanilla{
          .type = fin::pricing::OptionType::Call,
          .strike = pluss::full({1}, 100.0, cfg),
          .maturity = pluss::full({1}, 1.0, cfg),
      },
      .market = fin::market::State{
          .spot = pluss::full({1}, 100.0, cfg),
          .rate = pluss::full({1}, 0.02, cfg),
          .dividend_yield = pluss::full({1}, 0.00, cfg),
      },
      .model = fin::model::RoughHeston{
          .kappa = pluss::full({1}, 1.5, cfg),
          .theta = pluss::full({1}, 0.04, cfg),
          .sigma = pluss::full({1}, 0.30, cfg),
          .alpha = pluss::full({1}, -0.2, cfg),
          .rho = pluss::full({1}, -0.6, cfg),
          .v0 = pluss::full({1}, 0.04, cfg),
      },
      .controls = mod::Controls{
          .engine = mod::Engine::Analytic,
          .risk = mod::Risk::PriceOnly,
      },
  };

  bool threw = false;
  try {
    (void)pipe.price(cfg);
  } catch (const std::exception& e) {
    const std::string msg = e.what();
    threw = msg.find("RoughHeston") != std::string::npos && msg.find("MonteCarlo") != std::string::npos &&
            msg.find("Analytic") != std::string::npos;
  }
  expect(threw, "pipeline mismatch: expected helpful RoughHeston/Analytic error");
}

} // namespace

int main() {
  try {
    test_pipeline_heston_cos_loss_and_objective();
    test_pipeline_black_scholes_analytic();
    test_pipeline_black_scholes_auto_mixed_call_put();
    test_pipeline_monte_carlo_paths();
    test_pipeline_mismatch_error_is_helpful();
    std::cout << "pipeline_module_test passed\n";
    return 0;
  } catch (const std::exception& e) {
    std::cerr << "pipeline_module_test failed: " << e.what() << '\n';
    return 1;
  }
}
