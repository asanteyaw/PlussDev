#include <pluss/ma.h>

#include <cmath>
#include <functional>
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
  const auto strikes = pluss::series::linspace(80.0, 120.0, 9, cfg);
  const auto tau = pluss::series::full({9}, 1.0, cfg);

  mod::Field pipe{
      .instrument = fin::instrument::Vanilla{
          .type = fin::pricing::OptionType::Call,
          .strike = strikes,
          .maturity = tau,
      },
      .market = fin::market::State{
          .spot = pluss::series::full({9}, 100.0, cfg),
          .rate = pluss::series::full({9}, 0.02, cfg),
          .dividend_yield = pluss::series::full({9}, 0.00, cfg),
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

  const auto priced = pipe.price(cfg);
  expect(priced.price.numel() == 9, "pipeline heston cos: price size mismatch");
  expect(priced.greeks.delta.defined(), "pipeline heston cos: greeks expected with Risk::All");

  const auto market_px = priced.price;
  const double fit0 = pipe.loss_value(market_px, cfg);
  expect(std::isfinite(fit0), "pipeline heston cos: loss must be finite");
  expect(std::abs(fit0) < 1e-10, "pipeline heston cos: self-fit loss must be ~0");

  auto objective = pipe.bind_objective_typed<fin::model::Heston>(market_px, cfg);
  fin::model::Heston shifted = pipe.model.as<fin::model::Heston>();
  shifted.v0 = pluss::series::full({1}, 0.08, cfg);
  const double fit1 = objective(shifted);
  expect(std::isfinite(fit1), "pipeline heston cos: objective must be finite");
  expect(fit1 >= fit0, "pipeline heston cos: shifted params should not improve exact self-fit");
}

void test_pipeline_black_scholes_analytic() {
  const Config cfg = f64_cpu();
  mod::Field pipe{
      .instrument = fin::instrument::Vanilla{
          .type = fin::pricing::OptionType::Call,
          .strike = pluss::series::full({1}, 100.0, cfg),
          .maturity = pluss::series::full({1}, 1.0, cfg),
      },
      .market = fin::market::State{
          .spot = pluss::series::full({1}, 100.0, cfg),
          .rate = pluss::series::full({1}, 0.02, cfg),
          .dividend_yield = pluss::series::full({1}, 0.00, cfg),
      },
      .model = fin::model::BlackScholes{
          .volatility = pluss::series::full({1}, 0.2, cfg),
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
          .strike = pluss::series::full({4}, 100.0, cfg),
          .maturity = pluss::series::full({4}, 1.0, cfg),
      },
      .market = fin::market::State{
          .spot = pluss::series::linspace(90.0, 110.0, 4, cfg),
          .rate = pluss::series::full({1}, 0.02, cfg),
          .dividend_yield = pluss::series::full({1}, 0.00, cfg),
      },
      .model = fin::model::BlackScholes{
          .volatility = pluss::series::full({1}, 0.2, cfg),
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
          .strike = pluss::series::full({1}, 100.0, cfg),
          .maturity = pluss::series::full({1}, 1.0, cfg),
      },
      .market = fin::market::State{
          .spot = pluss::series::full({1}, 100.0, cfg),
          .rate = pluss::series::full({1}, 0.02, cfg),
          .dividend_yield = pluss::series::full({1}, 0.00, cfg),
      },
      .model = fin::model::Heston{
          .kappa = pluss::series::full({1}, 1.5, cfg),
          .theta = pluss::series::full({1}, 0.04, cfg),
          .sigma = pluss::series::full({1}, 0.30, cfg),
          .rho = pluss::series::full({1}, -0.70, cfg),
          .v0 = pluss::series::full({1}, 0.04, cfg),
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
          .strike = pluss::series::full({1}, 100.0, cfg),
          .maturity = pluss::series::full({1}, 1.0, cfg),
      },
      .market = fin::market::State{
          .spot = pluss::series::full({1}, 100.0, cfg),
          .rate = pluss::series::full({1}, 0.02, cfg),
          .dividend_yield = pluss::series::full({1}, 0.00, cfg),
      },
      .model = fin::model::RoughHeston{
          .kappa = pluss::series::full({1}, 1.5, cfg),
          .theta = pluss::series::full({1}, 0.04, cfg),
          .sigma = pluss::series::full({1}, 0.30, cfg),
          .alpha = pluss::series::full({1}, -0.2, cfg),
          .rho = pluss::series::full({1}, -0.6, cfg),
          .v0 = pluss::series::full({1}, 0.04, cfg),
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

mod::Field greeks_field(const Config& cfg, fin::model::Heston h) {
  return mod::Field{
      .instrument = fin::instrument::Vanilla{.type = fin::pricing::OptionType::Call,
                                             .strike = pluss::make_series({90.0, 100.0, 110.0}, {3}, cfg),
                                             .maturity = pluss::series::full({1}, 1.0, cfg)},
      .market = fin::market::State{.spot = pluss::series::full({1}, 100.0, cfg),
                                   .rate = pluss::series::full({1}, 0.02, cfg),
                                   .dividend_yield = pluss::series::full({1}, 0.0, cfg)},
      .model = h,
      .controls = mod::Controls{.engine = mod::Engine::Cos, .risk = mod::Risk::All},
  };
}

// Risk::All on the simulation and SABR engines must return greeks, not silently drop them: the Monte Carlo ones, with a fixed seed (common random
// numbers), agree with the COS ones to simulation error, and SABR with a vanishing vol-of-vol is Black-Scholes.
void test_pipeline_greeks_on_every_engine() {
  const Config cfg = f64_cpu();
  const fin::model::Heston h{.kappa = pluss::series::full({1}, 1.5, cfg), .theta = pluss::series::full({1}, 0.04, cfg),
                             .sigma = pluss::series::full({1}, 0.30, cfg), .rho = pluss::series::full({1}, -0.7, cfg),
                             .v0 = pluss::series::full({1}, 0.04, cfg)};
  mod::Field pipe = greeks_field(cfg, h);
  const auto cos = pipe.price(cfg);
  pipe.controls.engine = mod::Engine::MonteCarlo;
  pipe.controls.mc.paths = 40000;
  pipe.controls.mc.steps = 50;
  pipe.controls.mc.seed = 7;
  pipe.controls.mc.antithetic = true;
  const auto mc = pipe.price(cfg);
  expect(mc.greeks.delta.defined() && mc.greeks.gamma.defined() && mc.greeks.vega.defined() && mc.greeks.theta.defined() &&
             mc.greeks.rho.defined(),
         "greeks: Monte Carlo with Risk::All must return all five greeks");
  const auto d_cos = cos.greeks.delta.items<double>(), d_mc = mc.greeks.delta.items<double>();
  const auto v_cos = cos.greeks.vega.items<double>(), v_mc = mc.greeks.vega.items<double>();
  for (std::size_t i = 0; i < 3; ++i) {
    expect(std::abs(d_cos[i] - d_mc[i]) < 0.02, "greeks: Monte Carlo delta must match the COS delta");
    expect(std::abs(v_cos[i] - v_mc[i]) < 0.03 * std::abs(v_cos[i]), "greeks: Monte Carlo vega must match the COS vega");
  }

  mod::Field sabr{
      .instrument = fin::instrument::Vanilla{.type = fin::pricing::OptionType::Call, .strike = pluss::series::full({1}, 100.0, cfg),
                                             .maturity = pluss::series::full({1}, 1.0, cfg)},
      .market = fin::market::State{.spot = pluss::series::full({1}, 100.0, cfg), .rate = pluss::series::full({1}, 0.02, cfg),
                                   .dividend_yield = pluss::series::full({1}, 0.0, cfg)},
      .model = fin::model::Sabr{.alpha0 = pluss::series::full({1}, 0.2, cfg), .beta = pluss::series::full({1}, 1.0, cfg),
                                .nu = pluss::series::full({1}, 1e-6, cfg), .rho = pluss::series::full({1}, 0.0, cfg)},
      .controls = mod::Controls{.engine = mod::Engine::Analytic, .risk = mod::Risk::All},
  };
  const auto s = sabr.price(cfg);
  sabr.model = fin::model::BlackScholes{.volatility = pluss::series::full({1}, 0.2, cfg)};
  const auto b = sabr.price(cfg);
  expect(s.greeks.delta.defined() && s.greeks.vega.defined(), "greeks: SABR with Risk::All must return greeks");
  expect(std::abs(s.greeks.delta.items<double>()[0] - b.greeks.delta.items<double>()[0]) < 1e-5, "greeks: SABR delta must match Black-Scholes as nu -> 0");
  expect(std::abs(s.greeks.vega.items<double>()[0] - b.greeks.vega.items<double>()[0]) < 5e-3, "greeks: SABR vega must match Black-Scholes as nu -> 0");
}

void test_model_access_errors_name_the_held_model() {
  mod::Model m = fin::model::BlackScholes{};
  expect(m.name() == "BlackScholes" && m.holds<fin::model::BlackScholes>() && !m.holds<fin::model::Heston>(), "model: name()/holds() mismatch");
  bool named = false;
  try {
    (void)m.as<fin::model::Heston>();
  } catch (const std::exception& e) {
    const std::string msg = e.what();
    named = msg.find("BlackScholes") != std::string::npos && msg.find("Heston") != std::string::npos;
  }
  expect(named, "model: as<M>() on the wrong model must name both models");
}

} // namespace

void test_model_kind_is_explicit() {
  const Config cfg = f64_cpu();
  expect(fin::instrument::make(fin::pricing::Call, 100.0, 1.0).strike.scalar_type() == ScalarType::Float64 &&
             fin::market::make(100.0, 0.02, 0.0).rate.scalar_type() == ScalarType::Float64 &&
             fin::model::make(fin::model::Kind::Heston, {1.5, 0.04, 0.3, -0.7, 0.04}).as<fin::model::Heston>().v0.scalar_type() == ScalarType::Float64,
         "the finance factories store Float64 without being asked");
  mod::Field blank;
  expect(blank.model.kind() == fin::model::Kind::BlackScholes && blank.controls.engine == mod::Engine::Analytic,
         "a default Field holds Black-Scholes and uses the Analytic engine");
  const auto h = fin::model::make(fin::model::Kind::Heston, {1.5, 0.04, 0.3, -0.7, 0.04}, cfg);
  expect(h.kind() == fin::model::Kind::Heston && h.name() == "Heston", "kind selects the family");
  expect(fin::model::make(fin::model::Kind::Bates, {1.5, 0.04, 0.3, -0.7, 0.04, 0.1, -0.1, 0.15}, cfg).holds<fin::model::Bates>(), "eight numbers are Bates only when asked");
  bool threw = false;
  try {
    fin::model::make(fin::model::Kind::Heston, {1.5, 0.04, 0.3}, cfg);
  } catch (const std::exception& e) {
    threw = std::string(e.what()).find("kappa, theta, sigma, rho, v0") != std::string::npos;
  }
  expect(threw, "a wrong parameter count names the parameter order");

  const auto throws_with = [&](const std::function<void()>& f, const std::string& needle) {
    try {
      f();
    } catch (const std::exception& e) {
      return std::string(e.what()).find(needle) != std::string::npos;
    }
    return false;
  };
  using K = fin::model::Kind;
  expect(throws_with([&] { fin::model::make(K::Heston, {1.5, 0.04, 0.3, -0.7, 0.04, 0.1, -0.1, 0.15}, cfg); }, "Heston expects 5 parameters"), "Heston refuses eight numbers");
  expect(throws_with([&] { fin::model::make(K::Bates, {1.5, 0.04, 0.3, -0.7, 0.04}, cfg); }, "Bates expects 8 parameters"), "Bates refuses five numbers");
  expect(throws_with([&] { fin::model::make(K::Heston, {1.5, 0.04, 0.3, -1.2, 0.04}, cfg); }, "rho must be in (-1, 1)"), "correlation outside (-1,1)");
  expect(throws_with([&] { fin::model::make(K::Heston, {-1.5, 0.04, 0.3, -0.7, 0.04}, cfg); }, "kappa must be > 0"), "negative mean reversion");
  expect(throws_with([&] { fin::model::make(K::Heston, {1.5, 0.04, 0.3, -0.7, -0.01}, cfg); }, "v0 must be >= 0"), "negative initial variance");
  expect(throws_with([&] { fin::model::make(K::Bates, {1.5, 0.04, 0.3, -0.7, 0.04, -0.1, -0.1, 0.15}, cfg); }, "jump_intensity must be >= 0"), "negative jump intensity");
  expect(throws_with([&] { fin::model::make(K::Sabr, {0.2, 1.5, 0.3, 0.0}, cfg); }, "beta must be in [0, 1]"), "SABR beta above 1");
  expect(throws_with([&] { fin::model::make(K::BlackScholes, {0.0}, cfg); }, "volatility must be > 0"), "zero volatility");
  expect(throws_with([&] { fin::model::make(K::RoughHeston, {1.5, 0.04, 0.3, 0.2, -0.6, 0.04}, cfg); }, "alpha must be in (-0.5, 0)"), "rough exponent outside its range");
  expect(throws_with([&] { fin::model::make(K::RoughBergomi, {0.04, 0.8, -0.2, std::nan("")}, cfg); }, "must be finite"), "NaN parameter");
  mod::Field blank_priced;
  blank_priced.instrument = fin::instrument::make(fin::pricing::Call, 100.0, 1.0, cfg);
  blank_priced.market = fin::market::make(100.0, 0.02, 0.0, cfg);
  expect(throws_with([&] { blank_priced.price(cfg); }, "volatility must be defined"), "a Field with the default model asks for its volatility");
  mod::Field edited;
  edited.instrument = blank_priced.instrument;
  edited.market = blank_priced.market;
  edited.model = fin::model::make(K::Heston, {1.5, 0.04, 0.3, -0.7, 0.04}, cfg);
  edited.controls.engine = mod::Engine::Cos;
  edited.model.as<fin::model::Heston>().rho = pluss::series::full({1}, 1.5, cfg);
  expect(throws_with([&] { edited.price(cfg); }, "rho must be in (-1, 1)"), "a field edited into an invalid model is caught when it prices");
}

// Field::implied_vol: the Black-Scholes field recovers its own volatility, a Heston field gives a skewed smile, quotes are inverted, and a
// quote above the no-arbitrage bound comes back as NaN.
void test_field_implied_vol() {
  const Config cfg = f64_cpu();
  const auto strikes = pluss::series::linspace(80.0, 120.0, 9, cfg);
  mod::Field f{
      .instrument = fin::instrument::Vanilla{.type = fin::pricing::OptionType::Call, .strike = strikes,
                                             .maturity = pluss::series::full({9}, 1.0, cfg)},
      .market = fin::market::State{.spot = pluss::series::full({9}, 100.0, cfg), .rate = pluss::series::full({9}, 0.02, cfg),
                                   .dividend_yield = pluss::series::full({9}, 0.01, cfg)},
      .model = fin::model::BlackScholes{.volatility = pluss::series::full({1}, 0.25, cfg)},
      .controls = mod::Controls{.engine = mod::Engine::Analytic},
  };
  const auto flat = f.implied_vol({}, cfg);
  expect(flat.numel() == 9 && (flat - 0.25).abs().max().item<double>() < 1e-6, "field implied_vol: BS field must recover its own volatility");

  f.model = fin::model::Heston{.kappa = pluss::series::full({1}, 1.5, cfg), .theta = pluss::series::full({1}, 0.04, cfg),
                               .sigma = pluss::series::full({1}, 0.30, cfg), .rho = pluss::series::full({1}, -0.70, cfg),
                               .v0 = pluss::series::full({1}, 0.04, cfg)};
  f.controls.engine = mod::Engine::Cos;
  const auto smile = f.implied_vol({}, cfg).to(pluss::pCPU, ScalarType::Float64).host<double>();
  expect(smile.front() > smile.back() + 0.01, "field implied_vol: negative-rho Heston must slope down in strike");

  const auto quotes = f.price(cfg).price;
  const auto again = f.implied_vol(quotes, {}, cfg);
  expect((again - f.implied_vol({}, cfg)).abs().max().item<double>() < 1e-12, "field implied_vol: quotes form must match the model form");

  const auto bad = f.implied_vol(pluss::series::full({9}, 1000.0, cfg), {}, cfg).to(pluss::pCPU, ScalarType::Float64).host<double>();
  expect(std::isnan(bad[0]), "field implied_vol: unattainable quote must be NaN");
}

// Field::exercise_boundary: the put boundary starts at K min(1, r/q) at expiry and falls away from it with the time to maturity; other models refuse.
void test_field_exercise_boundary() {
  const Config cfg = f64_cpu();
  mod::Field f{
      .instrument = fin::instrument::american(fin::pricing::OptionType::Put, 100.0, 1.0),
      .market = fin::market::make(100.0, 0.05, 0.0),
      .model = fin::model::BlackScholes{.volatility = pluss::series::full({1}, 0.25, cfg)},
      .controls = mod::Controls{.engine = mod::Engine::Spectral},
  };
  const auto b = f.exercise_boundary(pluss::make_series({0.0, 0.25, 1.0}), cfg).to(pluss::pCPU, ScalarType::Float64).host<double>();
  expect(std::abs(b[0] - 100.0) < 1e-6 && b[1] < b[0] && b[2] < b[1] && b[2] > 0.0, "field exercise_boundary: put boundary must start at the strike and fall with time to maturity");
  bool refused = false;
  try {
    f.exercise_boundary(pluss::make_series({2.0}), cfg);
  } catch (const std::exception&) {
    refused = true;
  }
  expect(refused, "field exercise_boundary: a time beyond the maturity must be refused");
  f.model = fin::model::make(fin::model::Kind::Heston, {1.5, 0.04, 0.3, -0.7, 0.04});
  refused = false;
  try {
    f.exercise_boundary(pluss::make_series({0.5}), cfg);
  } catch (const std::exception&) {
    refused = true;
  }
  expect(refused, "field exercise_boundary: only Black-Scholes has a boundary curve");
}

// Closed-form COS Greeks (Heston and Bates, calls and puts, near and far from the money, short and long maturities) against central differences of the price.
void test_cos_closed_form_greeks() {
  const Config cfg = f64_cpu();
  const auto S = [&](std::vector<double> v) { return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}, cfg); };
  for (const bool bates : {false, true}) {
    for (const auto type : {fin::pricing::OptionType::Call, fin::pricing::OptionType::Put}) {
      auto build = [&](double spot, double rate, double maturity, double v0) {
        mod::Field f{.instrument = {.type = type, .strike = S({80.0, 100.0, 125.0, 100.0}), .maturity = S({1.0, 1.0, 1.0, 5.0})},
                     .market = {.spot = S({spot}), .rate = S({rate}), .dividend_yield = S({0.01})},
                     .model = bates ? fin::model::make(fin::model::Kind::Bates, {1.5, 0.04, 0.3, -0.7, v0, 0.2, -0.1, 0.15}) : fin::model::make(fin::model::Kind::Heston, {1.5, 0.04, 0.3, -0.7, v0}),
                     .controls = {.engine = mod::Engine::Cos, .risk = mod::Risk::PriceOnly}};
        if (maturity != 0.0) f.instrument.maturity = f.instrument.maturity + maturity;
        return f;
      };
      auto price = [&](double spot, double rate, double dt, double v0) { return build(spot, rate, dt, v0).price(cfg).price; };
      auto f = build(100.0, 0.03, 0.0, 0.04);
      f.controls.risk = mod::Risk::All;
      const auto g = f.price(cfg).greeks;
      const double h = 0.01, hr = 1e-5, ht = 1e-4, hv = 1e-6;
      const auto delta = (price(100.0 + h, 0.03, 0.0, 0.04) - price(100.0 - h, 0.03, 0.0, 0.04)) / (2.0 * h);
      const auto gamma = (price(100.0 + h, 0.03, 0.0, 0.04) - price(100.0, 0.03, 0.0, 0.04) * 2.0 + price(100.0 - h, 0.03, 0.0, 0.04)) / (h * h);
      const auto rho = (price(100.0, 0.03 + hr, 0.0, 0.04) - price(100.0, 0.03 - hr, 0.0, 0.04)) / (2.0 * hr);
      const auto theta = (price(100.0, 0.03, -ht, 0.04) - price(100.0, 0.03, ht, 0.04)) / (2.0 * ht);
      const auto vega = (price(100.0, 0.03, 0.0, 0.04 + hv) - price(100.0, 0.03, 0.0, 0.04 - hv)) / (2.0 * hv);
      const auto close = [&](const pluss::Series& a, const pluss::Series& b, double rel, const char* what) {
        const double worst = ((a - b).abs() / (b.abs() + 1.0)).max().sum().item<double>();
        expect(worst < rel, std::string("closed-form COS ") + what + " must match the central difference: " + std::to_string(worst));
      };
      close(g.delta, delta, 1e-6, "delta");
      close(g.gamma, gamma, 1e-5, "gamma");
      close(g.rho, rho, 1e-5, "rho");
      close(g.theta, theta, 1e-5, "theta");
      close(g.vega, vega, 1e-5, "vega");
    }
  }
}

// Monte Carlo Greeks from the shared paths of one simulation against the exact COS Greeks (Heston and Bates).
void test_mc_shared_path_greeks() {
  const Config cfg = f64_cpu();
  for (const bool bates : {false, true}) {
    mod::Field f{.instrument = {.type = fin::pricing::OptionType::Call, .strike = pluss::make_series({90.0, 100.0, 110.0}), .maturity = pluss::make_series({1.0})},
                 .market = fin::market::make(100.0, 0.03, 0.01),
                 .model = bates ? fin::model::make(fin::model::Kind::Bates, {1.5, 0.04, 0.3, -0.7, 0.04, 0.2, -0.1, 0.15}) : fin::model::make(fin::model::Kind::Heston, {1.5, 0.04, 0.3, -0.7, 0.04}),
                 .controls = {.engine = mod::Engine::Cos, .risk = mod::Risk::All}};
    const auto exact = f.price(cfg);
    f.controls.engine = mod::Engine::MonteCarlo;
    f.controls.mc.paths = 120000;
    f.controls.mc.steps = 100;
    f.controls.mc.scheme = fin::volatility::SimScheme::QuadExp;
    f.controls.mc.seed = 3;
    const auto mc = f.price(cfg);
    const auto close = [&](const pluss::Series& a, const pluss::Series& b, double tol, const char* what) {
      const double worst = (a - b).abs().max().sum().item<double>();
      expect(worst < tol, std::string("shared-path Monte Carlo ") + what + " must agree with the exact value: " + std::to_string(worst));
    };
    close(mc.price, exact.price, 0.15, "price");
    close(mc.greeks.delta, exact.greeks.delta, 0.02, "delta");
    close(mc.greeks.gamma, exact.greeks.gamma, 0.003, "gamma");
    close(mc.greeks.rho, exact.greeks.rho, 1.5, "rho");
    close(mc.greeks.vega, exact.greeks.vega, 3.0, "vega");
    close(mc.greeks.theta, exact.greeks.theta, 1.2, "theta");
  }
}

// The Monte Carlo vega is pathwise: the derivative of the simulated paths in the initial variance, equal to the difference of two simulations that share their
// random numbers (for every scheme, the exact transition included, and for the rough models).
void test_mc_vega_is_pathwise() {
  const Config cfg = f64_cpu();
  struct Case { fin::model::Kind kind; std::vector<double> params; fin::volatility::SimScheme scheme; };
  const std::vector<Case> cases{
      {fin::model::Kind::Heston, {1.5, 0.04, 0.3, -0.7, 0.04}, fin::volatility::SimScheme::Euler},
      {fin::model::Kind::Heston, {1.5, 0.04, 0.3, -0.7, 0.04}, fin::volatility::SimScheme::QuadExp},
      {fin::model::Kind::Heston, {3.0, 0.04, 0.3, -0.7, 0.04}, fin::volatility::SimScheme::Transition},
      {fin::model::Kind::Bates, {1.5, 0.04, 0.3, -0.7, 0.04, 0.2, -0.1, 0.15}, fin::volatility::SimScheme::Euler},
  };
  for (const auto& c : cases) {
    const auto build = [&](double v0) {
      auto params = c.params;
      params[4] = v0;
      mod::Field f{.instrument = {.type = fin::pricing::OptionType::Call, .strike = pluss::make_series({90.0, 100.0, 110.0}), .maturity = pluss::make_series({1.0})},
                   .market = fin::market::make(100.0, 0.03, 0.01), .model = fin::model::make(c.kind, params),
                   .controls = {.engine = mod::Engine::MonteCarlo, .risk = mod::Risk::PriceOnly}};
      f.controls.mc.paths = 20000;
      f.controls.mc.steps = 50;
      f.controls.mc.scheme = c.scheme;
      f.controls.mc.seed = 5;
      return f;
    };
    auto f = build(0.04);
    f.controls.risk = mod::Risk::All;
    const auto g = f.price(cfg).greeks.vega.items<double>();
    const auto up = build(0.04 + 1e-5).price(cfg).price.items<double>(), dn = build(0.04 - 1e-5).price(cfg).price.items<double>();
    for (std::size_t i = 0; i < g.size(); ++i) {
      const double bump = (up[i] - dn[i]) / 2e-5;
      expect(std::abs(g[i] - bump) < 5e-3 * (1.0 + std::abs(bump)), "Monte Carlo vega must equal the common-random-number difference: " + std::to_string(g[i]) + " vs " + std::to_string(bump));
    }
  }
}

// FFT strips: one transform serves every strike, and the Greeks from the grid and three multiplied transforms agree with the exact COS Greeks.
void test_fft_strip_and_greeks() {
  const Config cfg = f64_cpu();
  for (const bool bates : {false, true}) {
    for (const auto type : {fin::pricing::OptionType::Call, fin::pricing::OptionType::Put}) {
      mod::Field f{.instrument = {.type = type, .strike = pluss::make_series({70.0, 90.0, 100.0, 115.0, 140.0}), .maturity = pluss::make_series({1.0})},
                   .market = fin::market::make(100.0, 0.03, 0.01),
                   .model = bates ? fin::model::make(fin::model::Kind::Bates, {1.5, 0.04, 0.3, -0.7, 0.04, 0.2, -0.1, 0.15}) : fin::model::make(fin::model::Kind::Heston, {1.5, 0.04, 0.3, -0.7, 0.04}),
                   .controls = {.engine = mod::Engine::Cos, .risk = mod::Risk::All}};
      const auto exact = f.price(cfg);
      f.controls.engine = mod::Engine::Fft;
      const auto fft = f.price(cfg);
      const auto close = [&](const pluss::Series& a, const pluss::Series& b, double tol, const char* what) {
        const double worst = ((a - b).abs() / (b.abs() + 1.0)).max().sum().item<double>();
        expect(worst < tol, std::string("FFT ") + what + " must agree with the exact value: " + std::to_string(worst));
      };
      close(fft.price, exact.price, 1e-6, "price");
      close(fft.greeks.delta, exact.greeks.delta, 1e-5, "delta");
      close(fft.greeks.gamma, exact.greeks.gamma, 1e-4, "gamma");
      close(fft.greeks.vega, exact.greeks.vega, 1e-5, "vega");
      close(fft.greeks.rho, exact.greeks.rho, 1e-5, "rho");
      close(fft.greeks.theta, exact.greeks.theta, 1e-5, "theta");
    }
  }
}

// Greeks read off the solved grid: the binomial lattice against the Black-Scholes closed form, the Heston PDE against the exact COS Greeks.
void test_grid_greeks() {
  const Config cfg = f64_cpu();
  const auto close = [&](const pluss::Series& a, const pluss::Series& b, double tol, const std::string& what) {
    const double worst = ((a - b).abs() / (b.abs() + 1.0)).max().sum().item<double>();
    expect(worst < tol, what + " must agree with the exact value: " + std::to_string(worst));
  };
  for (const auto type : {fin::pricing::OptionType::Call, fin::pricing::OptionType::Put}) {
    mod::Field f{.instrument = {.type = type, .strike = pluss::make_series({85.0, 100.0, 120.0}), .maturity = pluss::make_series({1.0})}, .market = fin::market::make(100.0, 0.03, 0.01),
                 .model = fin::model::make(fin::model::Kind::BlackScholes, {0.25}), .controls = {.engine = mod::Engine::Analytic, .risk = mod::Risk::All}};
    const auto exact = f.price(cfg);
    f.controls.engine = mod::Engine::Lattice;
    f.controls.lattice.steps = 1000;
    const auto tree = f.price(cfg);
    close(tree.greeks.delta, exact.greeks.delta, 2e-3, "lattice delta");
    close(tree.greeks.gamma, exact.greeks.gamma, 2e-3, "lattice gamma");
    close(tree.greeks.theta, exact.greeks.theta, 2e-2, "lattice theta");
    close(tree.greeks.vega, exact.greeks.vega, 2e-2, "lattice vega");
    close(tree.greeks.rho, exact.greeks.rho, 2e-2, "lattice rho");
  }
  mod::Field h{.instrument = {.type = fin::pricing::OptionType::Call, .strike = pluss::make_series({90.0, 100.0, 110.0}), .maturity = pluss::make_series({1.0})}, .market = fin::market::make(100.0, 0.03, 0.01),
               .model = fin::model::make(fin::model::Kind::Heston, {1.5, 0.04, 0.3, -0.7, 0.04}), .controls = {.engine = mod::Engine::Cos, .risk = mod::Risk::All}};
  const auto exact = h.price(cfg);
  h.controls.engine = mod::Engine::Pde;
  const auto pde = h.price(cfg);
  close(pde.price, exact.price, 5e-3, "PDE price");
  close(pde.greeks.delta, exact.greeks.delta, 5e-3, "PDE delta");
  close(pde.greeks.gamma, exact.greeks.gamma, 5e-4, "PDE gamma");
  close(pde.greeks.vega, exact.greeks.vega, 5e-2, "PDE vega");
  close(pde.greeks.theta, exact.greeks.theta, 5e-2, "PDE theta");
  close(pde.greeks.rho, exact.greeks.rho, 5e-2, "PDE rho");
}

// Rough models: the shared-path Greeks of one simulation against central differences of the price with the same random numbers.
void test_rough_shared_path_greeks() {
  const Config cfg = f64_cpu();
  for (const bool bergomi : {false, true}) {
    const auto build = [&](double spot, double rate, double level) {
      mod::Field f{.instrument = {.type = fin::pricing::OptionType::Call, .strike = pluss::make_series({95.0, 105.0}), .maturity = pluss::make_series({0.5})},
                   .market = fin::market::make(spot, rate, 0.01),
                   .model = bergomi ? fin::model::make(fin::model::Kind::RoughBergomi, {level, 1.5, -0.1, -0.7}) : fin::model::make(fin::model::Kind::RoughHeston, {1.5, 0.04, 0.3, -0.3, -0.7, level}),
                   .controls = {.engine = mod::Engine::MonteCarlo, .risk = mod::Risk::PriceOnly}};
      f.controls.mc.paths = 20000;
      f.controls.mc.steps = 25;
      f.controls.mc.seed = 4;
      return f;
    };
    auto f = build(100.0, 0.03, 0.04);
    f.controls.risk = mod::Risk::All;
    const auto g = f.price(cfg).greeks;
    const auto price = [&](double spot, double rate, double level) { return build(spot, rate, level).price(cfg).price; };
    const double h = 0.5, hr = 1e-3, hl = 4e-4;
    const auto delta = (price(100.0 + h, 0.03, 0.04) - price(100.0 - h, 0.03, 0.04)) / (2.0 * h);
    const auto rho = (price(100.0, 0.03 + hr, 0.04) - price(100.0, 0.03 - hr, 0.04)) / (2.0 * hr);
    const auto vega = (price(100.0, 0.03, 0.04 + hl) - price(100.0, 0.03, 0.04 - hl)) / (2.0 * hl);
    const auto close = [&](const pluss::Series& a, const pluss::Series& b, double tol, const char* what) {
      const double worst = ((a - b).abs() / (b.abs() + 1.0)).max().sum().item<double>();
      expect(worst < tol, std::string("rough Monte Carlo ") + what + " must match the common-random-numbers difference: " + std::to_string(worst));
    };
    close(g.delta, delta, 5e-3, "delta");
    close(g.rho, rho, 5e-3, "rho");
    close(g.vega, vega, 4e-2, "vega");
  }
}

// Black-Scholes by Monte Carlo: pathwise Greeks from the same normals against the closed form.
void test_black_scholes_mc_pathwise_greeks() {
  const Config cfg = f64_cpu();
  for (const auto type : {fin::pricing::OptionType::Call, fin::pricing::OptionType::Put}) {
    for (const bool antithetic : {false, true}) {
      mod::Field f{.instrument = {.type = type, .strike = pluss::make_series({90.0, 100.0, 115.0}), .maturity = pluss::make_series({1.0})}, .market = fin::market::make(100.0, 0.03, 0.01),
                   .model = fin::model::make(fin::model::Kind::BlackScholes, {0.25}), .controls = {.engine = mod::Engine::Analytic, .risk = mod::Risk::All}};
      const auto exact = f.price(cfg);
      f.controls.engine = mod::Engine::MonteCarlo;
      f.controls.mc.paths = 400000;
      f.controls.mc.antithetic = antithetic;
      f.controls.mc.seed = 5;
      const auto mc = f.price(cfg);
      const auto close = [&](const pluss::Series& a, const pluss::Series& b, double tol, const char* what) {
        const double worst = ((a - b).abs() / (b.abs() + 1.0)).max().sum().item<double>();
        expect(worst < tol, std::string("Black-Scholes Monte Carlo ") + what + " must match the closed form: " + std::to_string(worst));
      };
      close(mc.price, exact.price, 5e-3, "price");
      close(mc.greeks.delta, exact.greeks.delta, 5e-3, "delta");
      close(mc.greeks.gamma, exact.greeks.gamma, 1e-2, "gamma");
      close(mc.greeks.vega, exact.greeks.vega, 1.5e-2, "vega");
      close(mc.greeks.rho, exact.greeks.rho, 1.5e-2, "rho");
      close(mc.greeks.theta, exact.greeks.theta, 1.5e-2, "theta");
    }
  }
}

// American Black-Scholes options by the spectral engine: Greeks from the solved boundary against central differences of independently solved prices.
void test_spectral_greeks() {
  const Config cfg = f64_cpu();
  for (const auto type : {fin::pricing::OptionType::Put, fin::pricing::OptionType::Call}) {
    const auto build = [&](double spot, double rate, double yield, double maturity, double vol) {
      return mod::Field{.instrument = fin::instrument::american(type, 100.0, maturity), .market = fin::market::make(spot, rate, yield),
                        .model = fin::model::make(fin::model::Kind::BlackScholes, {vol}), .controls = {.engine = mod::Engine::Spectral, .risk = mod::Risk::PriceOnly}};
    };
    const double r = type == fin::pricing::OptionType::Put ? 0.05 : 0.03, q = type == fin::pricing::OptionType::Put ? 0.01 : 0.07;
    // strikes of the option are fixed at 100: vary the spot across the in- and out-of-the-money cases
    for (const double spot : {90.0, 100.0, 112.0}) {
      auto f = build(spot, r, q, 1.0, 0.25);
      f.controls.risk = mod::Risk::All;
      const auto g = f.price(cfg).greeks;
      const auto p = [&](double s, double rate, double yield, double t, double v) { return build(s, rate, yield, t, v).price(cfg).price.items<double>()[0]; };
      const double d = (p(spot * 1.001, r, q, 1.0, 0.25) - p(spot * 0.999, r, q, 1.0, 0.25)) / (0.002 * spot);
      const double rho = (p(spot, r + 1e-4, q, 1.0, 0.25) - p(spot, r - 1e-4, q, 1.0, 0.25)) / 2e-4;
      const double vega = (p(spot, r, q, 1.0, 0.2505) - p(spot, r, q, 1.0, 0.2495)) / 0.001;
      const double theta = -(p(spot, r, q, 1.0 + 1e-3, 0.25) - p(spot, r, q, 1.0 - 1e-3, 0.25)) / 2e-3;
      const auto at = [&](const pluss::Series& x) { return x.items<double>()[0]; };
      expect(std::abs(at(g.delta) - d) < 2e-3, "spectral delta must match the difference of solved prices");
      expect(std::abs(at(g.rho) - rho) < 1e-5 + 1e-6 * std::abs(rho), "spectral rho must match the difference of solved prices");
      expect(std::abs(at(g.vega) - vega) < 1e-4 + 1e-5 * std::abs(vega), "spectral vega must match the difference of solved prices");
      expect(std::abs(at(g.theta) - theta) < 5e-2 + 5e-3 * std::abs(theta), "spectral theta must match the difference of solved prices");
    }
  }
}

// SABR (Analytic engine): Greeks with the exact Black / Bachelier derivatives against central differences of the price, for the lognormal and normal quotes.
void test_sabr_greeks() {
  const Config cfg = f64_cpu();
  for (const auto vol_type : {fin::pricing::SabrVolType::Lognormal, fin::pricing::SabrVolType::Normal}) {
    for (const auto approx : {fin::pricing::SabrApproxModel::Hagan2002, fin::pricing::SabrApproxModel::Obloj2008}) {
      for (const auto type : {fin::pricing::OptionType::Call, fin::pricing::OptionType::Put}) {
        const auto build = [&](double spot, double rate, double maturity, double alpha) {
          fin::model::Sabr m{.alpha0 = pluss::make_series({alpha}), .beta = pluss::make_series({0.5}), .nu = pluss::make_series({0.4}), .rho = pluss::make_series({-0.3})};
          m.vol_type = vol_type;
          m.approx.model = approx;
          return mod::Field{.instrument = {.type = type, .strike = pluss::make_series({85.0, 100.0, 118.0}), .maturity = pluss::make_series({maturity})},
                            .market = fin::market::make(spot, rate, 0.01), .model = mod::Model{m}, .controls = {.engine = mod::Engine::Analytic, .risk = mod::Risk::PriceOnly}};
        };
        auto f = build(100.0, 0.03, 1.0, 2.0);
        f.controls.risk = mod::Risk::All;
        const auto g = f.price(cfg).greeks;
        const auto price = [&](double s, double r, double t, double a) { return build(s, r, t, a).price(cfg).price; };
        const double h = 0.05, hr = 1e-5, ht = 1e-4, ha = 1e-4;
        const auto delta = (price(100.0 + h, 0.03, 1.0, 2.0) - price(100.0 - h, 0.03, 1.0, 2.0)) / (2.0 * h);
        const auto gamma = (price(100.0 + h, 0.03, 1.0, 2.0) - price(100.0, 0.03, 1.0, 2.0) * 2.0 + price(100.0 - h, 0.03, 1.0, 2.0)) / (h * h);
        const auto rho = (price(100.0, 0.03 + hr, 1.0, 2.0) - price(100.0, 0.03 - hr, 1.0, 2.0)) / (2.0 * hr);
        const auto theta = (price(100.0, 0.03, 1.0 - ht, 2.0) - price(100.0, 0.03, 1.0 + ht, 2.0)) / (2.0 * ht);
        const auto vega = (price(100.0, 0.03, 1.0, 2.0 + ha) - price(100.0, 0.03, 1.0, 2.0 - ha)) / (2.0 * ha);
        const auto close = [&](const pluss::Series& a, const pluss::Series& b, double tol, const char* what) {
          const double worst = ((a - b).abs() / (b.abs() + 1.0)).max().sum().item<double>();
          expect(worst < tol, std::string("SABR ") + what + " must match the central difference: " + std::to_string(worst));
        };
        close(g.delta, delta, 1e-5, "delta");
        close(g.gamma, gamma, 1e-4, "gamma");
        close(g.rho, rho, 1e-5, "rho");
        close(g.theta, theta, 1e-5, "theta");
        close(g.vega, vega, 1e-5, "vega");
      }
    }
  }
}

// American Black-Scholes options by the COS recursion: delta and gamma from the final expansion, against the spectral engine's Greeks.
void test_cos_exercise_greeks() {
  const Config cfg = f64_cpu();
  for (const auto type : {fin::pricing::OptionType::Put, fin::pricing::OptionType::Call}) {
    mod::Field f{.instrument = {.type = type, .strike = pluss::make_series({100.0}), .maturity = pluss::make_series({1.0}), .exercise = fin::pricing::ExerciseStyle::American},
                 .market = {.spot = pluss::make_series({95.0, 100.0, 108.0}), .rate = pluss::make_series({type == fin::pricing::OptionType::Put ? 0.05 : 0.03}),
                            .dividend_yield = pluss::make_series({type == fin::pricing::OptionType::Put ? 0.01 : 0.07})},
                 .model = fin::model::make(fin::model::Kind::BlackScholes, {0.25}), .controls = {.engine = mod::Engine::Spectral, .risk = mod::Risk::All}};
    const auto reference = f.price(cfg);
    f.controls.engine = mod::Engine::Cos;
    const auto cos = f.price(cfg);
    const auto close = [&](const pluss::Series& a, const pluss::Series& b, double tol, const char* what) {
      const double worst = ((a - b).abs() / (b.abs() + 1.0)).max().sum().item<double>();
      expect(worst < tol, std::string("American COS ") + what + " must match the spectral engine: " + std::to_string(worst));
    };
    close(cos.price, reference.price, 1e-4, "price");
    close(cos.greeks.delta, reference.greeks.delta, 2e-3, "delta");
    close(cos.greeks.gamma, reference.greeks.gamma, 2e-2, "gamma");
    close(cos.greeks.vega, reference.greeks.vega, 1e-3, "vega");
    close(cos.greeks.rho, reference.greeks.rho, 1e-3, "rho");
    close(cos.greeks.theta, reference.greeks.theta, 2e-3, "theta");
  }
}

// American Heston put by the COS recursion (delta, gamma, vega from the final expansion) against the PDE engine's Greeks read off its surface.
void test_heston_cos_exercise_greeks() {
  const Config cfg = f64_cpu();
  mod::Field f{.instrument = fin::instrument::american(fin::pricing::OptionType::Put, 100.0, 1.0), .market = {.spot = pluss::make_series({92.0, 100.0, 108.0}), .rate = pluss::make_series({0.05}), .dividend_yield = pluss::make_series({0.01})},
               .model = fin::model::make(fin::model::Kind::Heston, {2.0, 0.04, 0.4, -0.6, 0.04}), .controls = {.engine = mod::Engine::Pde, .risk = mod::Risk::All}};
  f.instrument.strike = pluss::make_series({100.0});
  const auto reference = f.price(cfg);
  f.controls.engine = mod::Engine::Cos;
  const auto cos = f.price(cfg);
  const auto close = [&](const pluss::Series& a, const pluss::Series& b, double tol, const char* what) {
    const double worst = ((a - b).abs() / (b.abs() + 1.0)).max().sum().item<double>();
    expect(worst < tol, std::string("American Heston COS ") + what + " must match the PDE engine: " + std::to_string(worst));
  };
  close(cos.price, reference.price, 5e-3, "price");
  close(cos.greeks.delta, reference.greeks.delta, 1e-2, "delta");
  close(cos.greeks.gamma, reference.greeks.gamma, 1e-2, "gamma");
  close(cos.greeks.vega, reference.greeks.vega, 5e-2, "vega");
  close(cos.greeks.rho, reference.greeks.rho, 5e-2, "rho");
  close(cos.greeks.theta, reference.greeks.theta, 1e-1, "theta");
}

// The Greeks of American Heston and Bates options by the COS recursion are exact derivatives (the rate through the recursion, the variance through the last
// kernel, the time from the pricing equation): they match central differences of the price itself.
void test_cos_exercise_greeks_are_exact() {
  const Config cfg = f64_cpu();
  for (const bool bates : {false, true}) {
    const auto build = [&](double rate, double v0, double maturity) {
      std::vector<double> params{2.0, 0.04, 0.4, -0.6, v0};
      if (bates) params.insert(params.end(), {0.3, -0.1, 0.15});
      mod::Field f{.instrument = fin::instrument::american(fin::pricing::OptionType::Put, 100.0, maturity),
                   .market = {.spot = pluss::make_series({100.0}), .rate = pluss::make_series({rate}), .dividend_yield = pluss::make_series({0.01})},
                   .model = fin::model::make(bates ? fin::model::Kind::Bates : fin::model::Kind::Heston, params), .controls = {.engine = mod::Engine::Cos, .risk = mod::Risk::PriceOnly}};
      f.instrument.strike = pluss::make_series({100.0});
      return f;
    };
    auto f = build(0.05, 0.04, 1.0);
    f.controls.risk = mod::Risk::All;
    const auto g = f.price(cfg).greeks;
    const auto p = [&](double r, double v, double t) { return build(r, v, t).price(cfg).price.items<double>()[0]; };
    const double rho = (p(0.05 + 1e-4, 0.04, 1.0) - p(0.05 - 1e-4, 0.04, 1.0)) / 2e-4;
    const double vega = (p(0.05, 0.04 + 1e-4, 1.0) - p(0.05, 0.04 - 1e-4, 1.0)) / 2e-4;
    const double theta = -(p(0.05, 0.04, 1.0 + 2e-3) - p(0.05, 0.04, 1.0 - 2e-3)) / 4e-3;
    expect(std::abs(g.rho.items<double>()[0] - rho) < 2e-3 * (1.0 + std::abs(rho)), std::string(bates ? "Bates" : "Heston") + " COS exercise rho must equal the derivative of the price");
    expect(std::abs(g.vega.items<double>()[0] - vega) < 2e-3 * (1.0 + std::abs(vega)), std::string(bates ? "Bates" : "Heston") + " COS exercise vega must equal the derivative of the price");
    expect(std::abs(g.theta.items<double>()[0] - theta) < 5e-3 * (1.0 + std::abs(theta)), std::string(bates ? "Bates" : "Heston") + " COS exercise theta must equal the derivative of the price");
  }
}

// American options by Longstaff-Schwartz: the Greeks come from one fit and its exercise rule (no repricing) and agree with the spectral / PDE engines.
void test_lsm_greeks() {
  const Config cfg = f64_cpu();
  for (const bool heston : {false, true}) {
    mod::Field f{.instrument = fin::instrument::american(fin::pricing::OptionType::Put, 100.0, 1.0), .market = {.spot = pluss::make_series({92.0}), .rate = pluss::make_series({0.05}), .dividend_yield = pluss::make_series({0.01})},
                 .model = heston ? fin::model::make(fin::model::Kind::Heston, {2.0, 0.04, 0.4, -0.6, 0.04}) : fin::model::make(fin::model::Kind::BlackScholes, {0.25}),
                 .controls = {.engine = heston ? mod::Engine::Pde : mod::Engine::Spectral, .risk = mod::Risk::All}};
    f.instrument.strike = pluss::make_series({100.0});
    const auto reference = f.price(cfg);
    f.controls.engine = mod::Engine::MonteCarlo;
    f.controls.mc.paths = 200000;
    f.controls.mc.steps = 50;
    f.controls.mc.seed = 11;
    const auto mc = f.price(cfg);
    const auto at = [](const pluss::Series& x) { return x.items<double>()[0]; };
    const auto close = [&](const pluss::Series& a, const pluss::Series& b, double tol, const char* what) {
      expect(std::abs(at(a) - at(b)) < tol, std::string(heston ? "Heston" : "Black-Scholes") + " Longstaff-Schwartz " + what + " must match the grid engine: " + std::to_string(at(a)) + " vs " + std::to_string(at(b)));
    };
    close(mc.price, reference.price, 0.25, "price");
    close(mc.greeks.delta, reference.greeks.delta, 0.02, "delta");
    close(mc.greeks.gamma, reference.greeks.gamma, 0.006, "gamma");
    close(mc.greeks.vega, reference.greeks.vega, 2.0, "vega");
    close(mc.greeks.rho, reference.greeks.rho, 0.5, "rho");
    close(mc.greeks.theta, reference.greeks.theta, heston ? 0.8 : 0.15, "theta");
  }
}

// The Heston PDE rho is the derivative of the discrete solution itself (the scheme and its exercise splitting differentiated step by step): it equals the
// difference of two solves in the rate.
void test_pde_rho_is_exact() {
  const Config cfg = f64_cpu();
  for (const auto type : {fin::pricing::OptionType::Put, fin::pricing::OptionType::Call}) {
    for (const auto style : {fin::pricing::ExerciseStyle::European, fin::pricing::ExerciseStyle::American}) {
      const auto build = [&](double rate) {
        mod::Field f{.instrument = {.type = type, .strike = pluss::make_series({100.0}), .maturity = pluss::make_series({1.0}), .exercise = style},
                     .market = {.spot = pluss::make_series({95.0, 105.0}), .rate = pluss::make_series({rate}), .dividend_yield = pluss::make_series({0.02})},
                     .model = fin::model::make(fin::model::Kind::Heston, {2.0, 0.04, 0.4, -0.6, 0.04}), .controls = {.engine = mod::Engine::Pde, .risk = mod::Risk::PriceOnly}};
        return f;
      };
      auto f = build(0.05);
      f.controls.risk = mod::Risk::All;
      const auto rho = f.price(cfg).greeks.rho.items<double>();
      const auto up = build(0.05 + 1e-5).price(cfg).price.items<double>(), dn = build(0.05 - 1e-5).price(cfg).price.items<double>();
      for (std::size_t i = 0; i < rho.size(); ++i) {
        const double bump = (up[i] - dn[i]) / 2e-5;
        expect(std::abs(rho[i] - bump) < 1e-4 * (1.0 + std::abs(bump)), "Heston PDE rho must equal the derivative of the solved price: " + std::to_string(rho[i]) + " vs " + std::to_string(bump));
      }
    }
  }
}

// SABR by Monte Carlo (beta < 1: the paths are not proportional to the spot): the Greeks of one simulation agree with the SABR expansion's Greeks.
void test_sabr_mc_greeks() {
  const Config cfg = f64_cpu();
  mod::Field f{.instrument = {.type = fin::pricing::OptionType::Call, .strike = pluss::make_series({90.0, 100.0, 110.0}), .maturity = pluss::make_series({1.0})},
               .market = fin::market::make(100.0, 0.03, 0.01), .model = fin::model::make(fin::model::Kind::Sabr, {0.3, 0.7, 0.4, -0.3}),
               .controls = {.engine = mod::Engine::Analytic, .risk = mod::Risk::All}};
  const auto ref = f.price(cfg);
  f.controls.engine = mod::Engine::MonteCarlo;
  f.controls.mc.paths = 200000;
  f.controls.mc.steps = 100;
  f.controls.mc.seed = 4;
  const auto mc = f.price(cfg);
  const auto close = [&](const pluss::Series& a, const pluss::Series& b, double tol, const char* what) {
    const double worst = (a - b).abs().max().sum().item<double>();
    expect(worst < tol, std::string("SABR Monte Carlo ") + what + " must agree with the expansion: " + std::to_string(worst));
  };
  close(mc.price, ref.price, 0.15, "price");
  close(mc.greeks.delta, ref.greeks.delta, 0.02, "delta");
  close(mc.greeks.gamma, ref.greeks.gamma, 0.003, "gamma");
  close(mc.greeks.vega, ref.greeks.vega, 3.0, "vega");
  close(mc.greeks.rho, ref.greeks.rho, 1.5, "rho");
  close(mc.greeks.theta, ref.greeks.theta, 1.5, "theta");
}

// SABR American exercise by Longstaff-Schwartz (beta < 1: paths not proportional to the spot): the Greeks of one fit, from the tracked spot, drift and level, agree
// with central differences of the same simulation's price (common random numbers).
void test_sabr_american_greeks() {
  const Config cfg = f64_cpu();
  const auto build = [&](double spot, double rate, double alpha) {
    mod::Field f{.instrument = fin::instrument::american(fin::pricing::OptionType::Put, 100.0, 1.0), .market = {.spot = pluss::make_series({spot}), .rate = pluss::make_series({rate}), .dividend_yield = pluss::make_series({0.01})},
                 .model = fin::model::make(fin::model::Kind::Sabr, {alpha, 0.7, 0.4, -0.3}), .controls = {.engine = mod::Engine::MonteCarlo, .risk = mod::Risk::PriceOnly}};
    f.instrument.strike = pluss::make_series({100.0});
    f.controls.mc.paths = 200000;
    f.controls.mc.steps = 50;
    f.controls.mc.seed = 3;
    return f;
  };
  const auto at = [](const pluss::Series& x) { return x.items<double>()[0]; };
  auto f = build(100.0, 0.05, 1.5);
  f.controls.risk = mod::Risk::All;
  const auto g = f.price(cfg);
  const double hs = 4.0, hr = 1e-3, ha = 1e-3;
  const double up = at(build(100.0 + hs, 0.05, 1.5).price(cfg).price), mid = at(build(100.0, 0.05, 1.5).price(cfg).price), dn = at(build(100.0 - hs, 0.05, 1.5).price(cfg).price);
  const double rho = (at(build(100.0, 0.05 + hr, 1.5).price(cfg).price) - at(build(100.0, 0.05 - hr, 1.5).price(cfg).price)) / (2 * hr);
  const double vega = (at(build(100.0, 0.05, 1.5 + ha).price(cfg).price) - at(build(100.0, 0.05, 1.5 - ha).price(cfg).price)) / (2 * ha);
  expect(std::abs(at(g.greeks.delta) - (up - dn) / (2 * hs)) < 0.03, "SABR American delta must match the difference of the simulation");
  expect(std::abs(at(g.greeks.gamma) - (up - 2 * mid + dn) / (hs * hs)) < 0.008, "SABR American gamma must match the difference of the simulation");
  expect(std::abs(at(g.greeks.rho) - rho) < 1.5, "SABR American rho must match the difference of the simulation");
  expect(std::abs(at(g.greeks.vega) - vega) < 1.0, "SABR American vega must match the difference of the simulation");
}

// Control variates and importance sampling leave the SABR Monte Carlo Greeks (pathwise, reweighted) consistent with the plain estimator's.
void test_sabr_mc_greeks_with_variance_reduction() {
  const Config cfg = f64_cpu();
  mod::Field f{.instrument = {.type = fin::pricing::OptionType::Call, .strike = pluss::make_series({100.0}), .maturity = pluss::make_series({1.0})},
               .market = fin::market::make(100.0, 0.03, 0.01), .model = fin::model::make(fin::model::Kind::Sabr, {0.3, 0.7, 0.4, -0.3}),
               .controls = {.engine = mod::Engine::Analytic, .risk = mod::Risk::All}};
  const auto ref = f.price(cfg);
  f.controls.engine = mod::Engine::MonteCarlo;
  f.controls.mc.paths = 200000;
  f.controls.mc.steps = 100;
  f.controls.mc.seed = 4;
  for (const int mode : {0, 1}) {
    f.controls.mc.control_variate = mode == 0;
    f.controls.mc.importance_sampling = mode == 1;
    f.controls.mc.importance_shift = 0.3;
    const auto mc = f.price(cfg);
    const std::string what = mode == 0 ? "control variate" : "importance sampling";
    expect((mc.price - ref.price).abs().max().sum().item<double>() < 0.15, "SABR Monte Carlo price with " + what + " must agree with the expansion");
    expect((mc.greeks.delta - ref.greeks.delta).abs().max().sum().item<double>() < 0.02, "SABR Monte Carlo delta with " + what + " must agree with the expansion");
    expect((mc.greeks.vega - ref.greeks.vega).abs().max().sum().item<double>() < 3.0, "SABR Monte Carlo vega with " + what + " must agree with the expansion");
    expect((mc.greeks.gamma - ref.greeks.gamma).abs().max().sum().item<double>() < (mode == 0 ? 0.004 : 0.012), "SABR Monte Carlo gamma with " + what + " must agree with the expansion");
  }
}

// The original-form FFT engine takes its Greeks from the multiplied transforms of the little-trap form (the same analytic function).
void test_fft_original_form_greeks() {
  const Config cfg = f64_cpu();
  mod::Field f{.instrument = {.type = fin::pricing::OptionType::Call, .strike = pluss::make_series({90.0, 100.0, 110.0}), .maturity = pluss::make_series({1.0})},
               .market = fin::market::make(100.0, 0.03, 0.01), .model = fin::model::make(fin::model::Kind::Heston, {1.5, 0.04, 0.3, -0.7, 0.04}),
               .controls = {.engine = mod::Engine::Cos, .risk = mod::Risk::All}};
  const auto exact = f.price(cfg);
  f.controls.engine = mod::Engine::Fft;
  f.controls.fft.mode = fin::pricing::HestonCfMode::Original;
  const auto fft = f.price(cfg);
  expect((fft.greeks.delta - exact.greeks.delta).abs().max().sum().item<double>() < 1e-4, "original-form FFT delta must match the exact value");
  expect((fft.greeks.vega - exact.greeks.vega).abs().max().sum().item<double>() < 1e-2, "original-form FFT vega must match the exact value");
}

// The tangent-mode lattice differentiates the discrete price itself: vega and rho equal central differences of the same tree's price (American put and call).
void test_lattice_tangent_is_exact() {
  const Config cfg = f64_cpu();
  for (const auto type : {fin::pricing::OptionType::Put, fin::pricing::OptionType::Call}) {
    const auto build = [&](double rate, double vol) {
      mod::Field f{.instrument = fin::instrument::american(type, 100.0, 1.0), .market = {.spot = pluss::make_series({90.0, 100.0, 110.0}), .rate = pluss::make_series({rate}), .dividend_yield = pluss::make_series({0.03})},
                   .model = fin::model::make(fin::model::Kind::BlackScholes, {vol}), .controls = {.engine = mod::Engine::Lattice, .risk = mod::Risk::PriceOnly}};
      f.instrument.strike = pluss::make_series({100.0});
      f.controls.lattice.steps = 400;
      return f;
    };
    auto f = build(0.05, 0.25);
    f.controls.risk = mod::Risk::All;
    const auto g = f.price(cfg).greeks;
    const double h = 1e-6;
    const auto vega = (build(0.05, 0.25 + h).price(cfg).price - build(0.05, 0.25 - h).price(cfg).price) / (2.0 * h);
    const auto rho = (build(0.05 + h, 0.25).price(cfg).price - build(0.05 - h, 0.25).price(cfg).price) / (2.0 * h);
    expect((g.vega - vega).abs().max().sum().item<double>() < 5e-4 && (g.rho - rho).abs().max().sum().item<double>() < 5e-3, "lattice tangent vega and rho must equal the derivatives of the tree price");
  }
}

int main() {
  try {
    test_pipeline_heston_cos_loss_and_objective();
    test_field_implied_vol();
    test_cos_closed_form_greeks();
    test_mc_shared_path_greeks();
    test_mc_vega_is_pathwise();
    test_lsm_greeks();
    test_pde_rho_is_exact();
    test_sabr_mc_greeks();
    test_sabr_american_greeks();
    test_sabr_mc_greeks_with_variance_reduction();
    test_fft_original_form_greeks();
    test_fft_strip_and_greeks();
    test_grid_greeks();
    test_lattice_tangent_is_exact();
    test_rough_shared_path_greeks();
    test_black_scholes_mc_pathwise_greeks();
    test_spectral_greeks();
    test_sabr_greeks();
    test_cos_exercise_greeks();
    test_cos_exercise_greeks_are_exact();
    test_heston_cos_exercise_greeks();
    test_field_exercise_boundary();
    test_pipeline_black_scholes_analytic();
    test_pipeline_black_scholes_auto_mixed_call_put();
    test_pipeline_monte_carlo_paths();
    test_pipeline_mismatch_error_is_helpful();
    test_pipeline_greeks_on_every_engine();
    test_model_access_errors_name_the_held_model();
    test_model_kind_is_explicit();
    std::cout << "pipeline_module_test passed\n";
    return 0;
  } catch (const std::exception& e) {
    std::cerr << "pipeline_module_test failed: " << e.what() << '\n';
    return 1;
  }
}
