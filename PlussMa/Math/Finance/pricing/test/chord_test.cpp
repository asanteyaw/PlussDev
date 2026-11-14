#include <pluss/ma.h>

#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <vector>

namespace {

namespace fin = pluss::subsp::math::finance;
namespace mod = fin::module;
using pluss::Series;

// Field and Chord name their results the same way: a Field's price() returns Field::Result (the pricer layer's Result) and a Chord's returns a
// Chord::Result holding one Field::Result per asset.
static_assert(std::is_same_v<mod::Field::Result, fin::pricer::Result>, "Field::Result is the pricer layer's Result");
static_assert(std::is_same_v<decltype(std::declval<const mod::Chord&>().price()), mod::Chord::Result>, "Chord::price returns Chord::Result");
static_assert(std::is_same_v<decltype(std::declval<const mod::Chord&>().simulate_paths()), mod::Chord::Paths>, "Chord::simulate_paths returns Chord::Paths");

void expect(bool cond, const std::string& msg) {
  if (!cond) throw std::runtime_error(msg);
}

Series S(std::vector<double> v) { return pluss::make_series(v); }

mod::Field bs(double spot, double strike, double vol, double maturity = 1.0) {
  return mod::Field{.instrument = fin::instrument::make(fin::pricing::Call, strike, maturity), .market = fin::market::make(spot, 0.02, 0.01),
                    .model = fin::model::BlackScholes{.volatility = S({vol})}, .controls = {.engine = mod::Engine::Analytic}};
}

mod::Field heston(double spot, std::vector<double> strikes, std::vector<double> params = {1.5, 0.04, 0.3, -0.7, 0.04}) {
  return mod::Field{.instrument = {.type = fin::pricing::Call, .strike = S(strikes), .maturity = S({1.0})}, .market = fin::market::make(spot, 0.02, 0.0),
                    .model = fin::model::make(fin::model::Kind::Heston, params), .controls = {.engine = mod::Engine::Cos, .risk = mod::Risk::PriceOnly}};
}

// How much higher the variance increment is after a strongly negative price shock than after a strongly positive one, as a t statistic. With rho < 0 it is large and
// positive; an independent draw in a branch of the variance step would leave it near zero for the steps in that branch.
double negative_shock_variance_lift(const Series& dv, const Series& shock) {
  const double s = std::sqrt(shock.square().mean().item<double>());
  const Series low = shock.lt(-s).to(pluss::ScalarType::Float64), high = shock.gt(s).to(pluss::ScalarType::Float64);
  const double nl = low.sum().item<double>(), nh = high.sum().item<double>();
  const double ml = dv.mul(low).sum().item<double>() / nl, mh = dv.mul(high).sum().item<double>() / nh;
  const double vl = dv.sub(ml).square().mul(low).sum().item<double>() / (nl - 1.0), vh = dv.sub(mh).square().mul(high).sum().item<double>() / (nh - 1.0);
  return (ml - mh) / std::sqrt(vl / nl + vh / nh);
}

double worst(const Series& a, const Series& b) { return (a - b).abs().max().sum().item<double>(); }

// Black-Scholes assets with different spots, strikes and volatilities share one vectorised call and agree with pricing each alone.
void test_black_scholes_batches_into_one_call() {
  mod::Chord chord;
  chord.add("a", bs(100.0, 100.0, 0.2)).add("b", bs(50.0, 55.0, 0.35, 0.5)).add("c", bs(230.0, 200.0, 0.15, 2.0));
  expect(chord.batches() == 1, "chord: three Black-Scholes assets must form one batch");
  const auto r = chord.price();
  expect(r.offsets.size() == 4 && r.price.numel() == 3, "chord: result must hold three prices");
  for (const char* name : {"a", "b", "c"}) {
    const auto alone = chord.at(name).price();
    expect(worst(r[name].price, alone.price) < 1e-12 && worst(r[name].greeks.delta, alone.greeks.delta) < 1e-12 && worst(r[name].greeks.vega, alone.greeks.vega) < 1e-9,
           std::string("chord: batched price and Greeks must equal the asset priced alone: ") + name);
  }
}

// Heston assets batch through the COS engine whatever their parameters (strips of different lengths, spots and parameter sets share one call); MC makes its own.
void test_equal_parameters_batch_and_others_do_not() {
  mod::Chord chord;
  chord.add("h1", heston(100.0, {90.0, 100.0, 110.0})).add("h2", heston(80.0, {75.0, 85.0})).add("h3", heston(100.0, {100.0}, {2.0, 0.09, 0.4, -0.5, 0.09}));
  mod::Field mc = heston(100.0, {100.0});
  mc.controls.engine = mod::Engine::MonteCarlo;
  mc.controls.mc.paths = 2000;
  mc.controls.mc.steps = 10;
  chord.add("mc", mc);
  expect(chord.batches() == 2, "chord: three Heston COS assets with different parameters share one call, and the Monte Carlo asset makes another");
  const auto r = chord.price();
  expect(r.price.numel() == 3 + 2 + 1 + 1 && r.offsets[2] == 5, "chord: offsets must follow the strips");
  for (const char* name : {"h1", "h2", "h3", "mc"}) {
    expect(worst(r[name].price, chord.at(name).price().price) < 1e-8, std::string("chord: result must equal the asset priced alone: ") + name);
  }
}

// A dictionary of assets builds a Chord at construction and adds several at once, keeping the order, and a bad entry changes nothing.
void test_dictionary_construction_and_add() {
  mod::Chord built{{"a", bs(100.0, 100.0, 0.2)}, {"b", bs(50.0, 55.0, 0.35, 0.5)}, {"h", heston(100.0, {90.0, 110.0})}};
  mod::Chord chained;
  chained.add("a", bs(100.0, 100.0, 0.2)).add("b", bs(50.0, 55.0, 0.35, 0.5)).add("h", heston(100.0, {90.0, 110.0}));
  expect(built.size() == 3 && built.names() == chained.names(), "chord: a dictionary construction must keep the order of the list");
  expect(worst(built.price().price, chained.price().price) < 1e-12, "chord: a dictionary construction must price like chained add");

  mod::Chord grown{{"a", bs(100.0, 100.0, 0.2)}};
  grown.add({{"b", bs(50.0, 55.0, 0.35, 0.5)}, {"h", heston(100.0, {90.0, 110.0})}}).add("c", bs(10.0, 10.0, 0.1));
  expect(grown.size() == 4 && grown.names()[1] == "b" && grown.names()[3] == "c", "chord: add of a dictionary must append in order and chain");

  std::vector<mod::Chord::Entry> list;
  list.emplace_back("x", bs(100.0, 100.0, 0.2));
  list.emplace_back("y", bs(100.0, 105.0, 0.2));
  mod::Chord from_vector(list);
  mod::Chord added;
  added.add(list);
  expect(from_vector.names() == added.names() && from_vector.size() == 2, "chord: a vector of pairs must construct and add");

  const auto refused = [](auto&& f) {
    try {
      f();
    } catch (const std::exception&) {
      return true;
    }
    return false;
  };
  mod::Chord base{{"a", bs(100.0, 100.0, 0.2)}};
  expect(refused([&] { base.add({{"b", bs(1.0, 1.0, 0.1)}, {"a", bs(2.0, 2.0, 0.1)}}); }) && base.size() == 1, "chord: a name already present must refuse the whole list");
  expect(refused([&] { base.add({{"b", bs(1.0, 1.0, 0.1)}, {"b", bs(2.0, 2.0, 0.1)}}); }) && base.size() == 1, "chord: a name repeated in the list must refuse the whole list");
  expect(refused([&] { base.add({{"b", bs(1.0, 1.0, 0.1)}, {"", bs(2.0, 2.0, 0.1)}}); }) && base.size() == 1, "chord: an empty name must refuse the whole list");
  expect(refused([] { mod::Chord c{{"a", bs(1.0, 1.0, 0.1)}, {"a", bs(1.0, 1.0, 0.1)}}; }), "chord: a repeated name at construction must be refused");

  mod::Chord empty{};
  expect(empty.size() == 0 && empty.parallel, "chord: an empty Chord keeps its defaults");
}

// A table of parameters (columns per asset, as in a calibration file) builds the same Chord as writing every Field, and the correlations of the table are used.
void test_from_model_params() {
  const auto cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float64);
  mod::Chord::Parameters table;
  table.names = {"a", "b", "c"};
  table.spot = {100.0, 50.0, 80.0};
  table.rate = {0.03, 0.01, 0.02};
  table.dividend_yield = {0.01};
  table.kappa = {1.5, 2.0, 1.0};
  table.theta = {0.04, 0.06, 0.05};
  table.sigma = {0.3, 0.4, 0.25};
  table.rho = {-0.7, 0.0, -0.4};
  table.v0 = {0.09, 0.03, 0.05};
  table.maturity = {1.0};
  table.controls.mc.paths = 4000;
  table.controls.mc.steps = 20;
  table.controls.mc.seed = 5;
  table.price_corr = pluss::make_series({1.0, 0.6, -0.3, 0.6, 1.0, 0.2, -0.3, 0.2, 1.0}, {3, 3}, cfg);
  table.var_corr = pluss::make_series({1.0, 0.5, 0.0, 0.5, 1.0, 0.0, 0.0, 0.0, 1.0}, {3, 3}, cfg);
  const mod::Chord built = mod::Chord::from_model_params(table);

  const auto field = [&](double spot, double rate, std::vector<double> params) {
    mod::Field f{.instrument = fin::instrument::make(fin::pricing::OptionType::Call, spot, 1.0), .market = fin::market::make(spot, rate, 0.01),
                 .model = fin::model::make(fin::model::Kind::Heston, params), .controls = {.engine = mod::Engine::MonteCarlo}};
    f.controls.mc.paths = 4000;
    f.controls.mc.steps = 20;
    f.controls.mc.seed = 5;
    return f;
  };
  mod::Chord written{{"a", field(100.0, 0.03, {1.5, 0.04, 0.3, -0.7, 0.09})}, {"b", field(50.0, 0.01, {2.0, 0.06, 0.4, 0.0, 0.03})}, {"c", field(80.0, 0.02, {1.0, 0.05, 0.25, -0.4, 0.05})}};
  written.price_corr = table.price_corr;
  written.var_corr = table.var_corr;
  expect(built.names() == written.names() && built.size() == 3, "chord: a parameter table must keep the names and the order");
  const auto p1 = built.simulate_paths(), p2 = written.simulate_paths();
  expect(worst(p1.asset, p2.asset) < 1e-12 && worst(p1.variance, p2.variance) < 1e-12 && worst(p1.asset_shocks, p2.asset_shocks) < 1e-12,
         "chord: a parameter table must simulate the same paths as the Chord written field by field");
  expect(worst(built.at("b").market.spot, S({50.0})) < 1e-12 && built.at("a").instrument.strike.items<double>()[0] == 100.0, "chord: the strike must default to the spot");

  mod::Chord::Parameters defaults;
  defaults.spot = {100.0, 90.0};
  defaults.kappa = {1.5};
  defaults.theta = {0.04};
  defaults.sigma = {0.3};
  defaults.rho = {-0.7};
  defaults.v0 = {0.04};
  const mod::Chord plain = mod::Chord::from_model_params(defaults);
  expect(plain.size() == 2 && plain.names()[0] == "asset1" && plain.names()[1] == "asset2", "chord: unnamed assets must be called asset1, asset2, ...");
  expect(plain.at("asset2").model.holds<fin::model::Heston>() && !plain.price_corr.defined(), "chord: no jump columns gives Heston assets and independent shocks");

  mod::Chord::Parameters bates = defaults;
  bates.jump_intensity = {0.15};
  bates.jump_mean = {-0.08};
  bates.jump_vol = {0.2};
  expect(mod::Chord::from_model_params(bates).at("asset1").model.holds<fin::model::Bates>(), "chord: the three jump columns together give Bates assets");

  const auto refused = [](auto&& f) {
    try { f(); } catch (const std::exception&) { return true; }
    return false;
  };
  mod::Chord::Parameters partial = defaults;
  partial.jump_intensity = {0.15};
  expect(refused([&] { (void)mod::Chord::from_model_params(partial); }), "chord: jump columns given only in part must be refused");
  mod::Chord::Parameters wrong = defaults;
  wrong.kappa = {1.5, 2.0, 3.0};
  expect(refused([&] { (void)mod::Chord::from_model_params(wrong); }), "chord: a column that is neither one entry nor one per asset must be refused");
  mod::Chord::Parameters missing = defaults;
  missing.v0.clear();
  expect(refused([&] { (void)mod::Chord::from_model_params(missing); }), "chord: a missing required column must be refused");
  mod::Chord::Parameters shape = defaults;
  shape.price_corr = pluss::make_series({1.0, 0.5, 0.5, 1.0, 0.0, 0.0, 0.0, 0.0, 1.0}, {3, 3}, cfg);
  expect(refused([&] { (void)mod::Chord::from_model_params(shape); }), "chord: a correlation matrix of the wrong size must be refused");

  // A variance correlation that is not symmetric (0.2 against 0.02) is refused when the paths are simulated.
  mod::Chord::Parameters asymmetric = defaults;
  asymmetric.spot = {100.0, 90.0};
  asymmetric.var_corr = pluss::make_series({1.0, 0.2, 0.02, 1.0}, {2, 2}, cfg);
  asymmetric.controls.mc.paths = 100;
  asymmetric.controls.mc.steps = 5;
  expect(refused([&] { (void)mod::Chord::from_model_params(asymmetric).simulate_paths(); }), "chord: an asymmetric correlation must be refused by simulate_paths");
}

// With the quadratic-exponential scheme the variance of each asset keeps its correlation with its price and with the other assets' variances, also where the
// exponential branch applies (a small variance and a large volatility of variance put almost every step there).
void test_quadexp_correlation_across_assets() {
  const auto cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float64);
  mod::Chord::Parameters p;
  p.spot = {100.0, 90.0};
  p.kappa = {1.0};
  p.theta = {0.01};
  p.sigma = {1.5};
  p.rho = {-0.7};
  p.v0 = {0.01};
  p.controls.mc.paths = 4000;
  p.controls.mc.steps = 100;
  p.controls.mc.seed = 17;
  p.controls.mc.scheme = fin::volatility::SimScheme::QuadExp;
  p.price_corr = pluss::make_series({1.0, 0.8, 0.8, 1.0}, {2, 2}, cfg);
  const auto r = mod::Chord::from_model_params(p).simulate_paths();
  const std::int64_t n = 4000 * 100;
  const auto dv = [&](int a) { return r.variance.slice(1, 1, 101).select(2, a).sub(r.variance.slice(1, 0, 100).select(2, a)).reshape({n}); };
  const auto shock = [&](int a) { return r.asset_shocks.select(2, a).reshape({n}); };
  const double own = negative_shock_variance_lift(dv(0), shock(0)), other_asset = negative_shock_variance_lift(dv(0), shock(1));
  expect(own > 8.0, "chord: with quadexp each variance must rise after its own negative price shocks (t statistic " + std::to_string(own) + ")");
  expect(other_asset > 4.0, "chord: with quadexp a variance must also rise after the other asset's negative price shocks, through price_corr (t statistic " + std::to_string(other_asset) + ")");
}

// Black-Scholes, Heston, SABR and Bates assets simulated together, in an order that interleaves the kinds, with the three correlation matrices.
void test_mixed_models() {
  const auto cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float64);
  const auto mc_field = [&](double spot, mod::Model model) {
    mod::Field f{.instrument = fin::instrument::make(fin::pricing::OptionType::Call, spot, 1.0), .market = fin::market::make(spot, 0.02, 0.01), .model = model,
                 .controls = {.engine = mod::Engine::MonteCarlo, .risk = mod::Risk::PriceOnly}};
    f.controls.mc.paths = 40000;
    f.controls.mc.steps = 20;
    f.controls.mc.seed = 5;
    return f;
  };
  mod::Chord chord{{"h", mc_field(100.0, fin::model::make(fin::model::Kind::Heston, {1.5, 0.04, 0.3, -0.7, 0.04}))},
                   {"b", mc_field(80.0, fin::model::BlackScholes{.volatility = S({0.2})})},
                   {"s", mc_field(120.0, fin::model::make(fin::model::Kind::Sabr, {0.3, 0.7, 0.5, -0.3}))},
                   {"j", mc_field(90.0, fin::model::make(fin::model::Kind::Bates, {1.0, 0.05, 0.25, -0.4, 0.05, 0.3, -0.1, 0.15}))}};
  chord.price_corr = pluss::make_series({1.0, 0.5, 0.4, 0.3, 0.5, 1.0, 0.2, 0.1, 0.4, 0.2, 1.0, 0.25, 0.3, 0.1, 0.25, 1.0}, {4, 4}, cfg);
  chord.var_corr = pluss::make_series({1.0, 0.0, 0.3, 0.2, 0.0, 1.0, 0.0, 0.0, 0.3, 0.0, 1.0, 0.1, 0.2, 0.0, 0.1, 1.0}, {4, 4}, cfg);
  const auto r = chord.simulate_paths();
  expect(r.names == std::vector<std::string>({"h", "b", "s", "j"}) && r.asset.sizes() == std::vector<std::int64_t>({40000, 21, 4}), "chord: mixed paths must keep the order and shape of the assets");

  const std::int64_t n = 40000 * 20;
  const auto corr = [](const Series& x, const Series& y) {
    const Series a = x.sub(x.mean()), b = y.sub(y.mean());
    return a.mul(b).mean().item<double>() / std::sqrt(a.square().mean().item<double>() * b.square().mean().item<double>());
  };
  const auto z = [&](int a) { return r.asset_shocks.select(2, a).reshape({n}); };
  const auto w = [&](int a) { return r.vol_shocks.select(2, a).reshape({n}); };
  const double pc[4][4] = {{1, .5, .4, .3}, {.5, 1, .2, .1}, {.4, .2, 1, .25}, {.3, .1, .25, 1}};
  for (int a = 0; a < 4; ++a)
    for (int b = a + 1; b < 4; ++b) expect(std::abs(corr(z(a), z(b)) - pc[a][b]) < 0.01, "chord: the price shocks of mixed models must carry price_corr");
  expect(std::abs(corr(w(0), w(2)) - 0.3) < 0.01 && std::abs(corr(w(0), w(3)) - 0.2) < 0.01 && std::abs(corr(w(2), w(3)) - 0.1) < 0.01,
         "chord: the second-factor shocks of Heston, SABR and Bates assets must carry var_corr exactly");
  expect(std::abs(corr(w(1), w(0))) < 0.01 && std::abs(corr(w(1), w(2))) < 0.01, "chord: a Black-Scholes asset has no second factor correlated with the others");
  const double rho_h = -0.7, rho_s = -0.3;
  expect(std::abs(corr(z(1), w(0)) - rho_h * 0.5) < 0.01 && std::abs(corr(z(1), w(2)) - rho_s * 0.2) < 0.01 && std::abs(corr(z(0), w(2)) - rho_s * 0.4) < 0.01,
         "chord: the cross correlations across model kinds must be built as rho_b price_corr[a][b]");

  // marginals: each asset keeps its own law whichever kinds it is simulated with
  const auto terminal = [&](int a) { return r.asset.select(2, a).select(1, 20); };
  const double growth = 0.01;
  const double spots[4] = {100.0, 80.0, 120.0, 90.0};
  for (int a = 0; a < 4; ++a) {
    const Series t = terminal(a);
    const double mean = t.mean().item<double>(), se = std::sqrt(t.sub(mean).square().mean().item<double>() / 40000.0);
    expect(std::abs(mean - spots[a] * std::exp(growth)) < 4.0 * se, "chord: every asset of a mixed Chord must reproduce its forward");
  }
  const Series bs_variance = r.variance.select(2, 1);
  expect(bs_variance.sub(0.04).abs().max().item<double>() < 1e-12, "chord: a Black-Scholes asset has the constant variance sigma^2");
  const Series alpha_t = r.variance.select(2, 2).select(1, 20).sqrt();
  const double alpha_mean = alpha_t.mean().item<double>(), alpha_se = std::sqrt(alpha_t.sub(alpha_mean).square().mean().item<double>() / 40000.0);
  expect(std::abs(alpha_mean - 0.3) < 4.0 * alpha_se, "chord: the SABR alpha must be a martingale (its mean stays at alpha0)");
  const double ev = r.variance.select(2, 0).select(1, 20).mean().item<double>();
  expect(std::abs(ev - (0.04 + (0.04 - 0.04) * std::exp(-1.5))) < 2e-3, "chord: the Heston variance of a mixed Chord must keep its mean");

  // a Black-Scholes asset has no variance shock to correlate
  mod::Chord bad = chord;
  bad.var_corr = pluss::make_series({1.0, 0.3, 0.3, 0.2, 0.3, 1.0, 0.0, 0.0, 0.3, 0.0, 1.0, 0.1, 0.2, 0.0, 0.1, 1.0}, {4, 4}, cfg);
  std::string message;
  try { (void)bad.simulate_paths(); } catch (const std::exception& e) { message = e.what(); }
  expect(message.find("Black-Scholes asset and has no variance shock") != std::string::npos, "chord: a var_corr entry for a Black-Scholes asset must be refused, naming it");
  bad = chord;
  bad.price_var_cross = pluss::make_series({-0.7, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, -0.3, 0.0, 0.0, 0.0, 0.0, -0.4}, {4, 4}, cfg);
  bad.price_var_cross = pluss::make_series({-0.7, 0.2, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, -0.3, 0.0, 0.0, 0.0, 0.0, -0.4}, {4, 4}, cfg);   // price(h) with the variance of b
  message.clear();
  try { (void)bad.simulate_paths(); } catch (const std::exception& e) { message = e.what(); }
  expect(message.find("price_var_cross") != std::string::npos, "chord: a price_var_cross entry for a Black-Scholes variance must be refused");
}

// The rough models in the mix: they depend on the whole past of their shocks, which are the correlated shocks of the joint simulation.
void test_rough_models_in_the_mix() {
  const auto cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float64);
  const std::int64_t paths = 20000, steps = 24;
  const auto mc_field = [&](double spot, mod::Model model) {
    mod::Field f{.instrument = fin::instrument::make(fin::pricing::OptionType::Call, spot, 1.0), .market = fin::market::make(spot, 0.02, 0.01), .model = model,
                 .controls = {.engine = mod::Engine::MonteCarlo, .risk = mod::Risk::PriceOnly}};
    f.controls.mc.paths = paths;
    f.controls.mc.steps = steps;
    f.controls.mc.seed = 9;
    return f;
  };
  const double xi = 0.04, eta = 1.2, alpha = -0.3, rho_rb = -0.7;
  mod::Chord chord{{"rh", mc_field(100.0, fin::model::make(fin::model::Kind::RoughHeston, {1.5, 0.04, 0.3, -0.2, -0.7, 0.04}))},
                   {"h", mc_field(100.0, fin::model::make(fin::model::Kind::Heston, {1.5, 0.04, 0.3, -0.7, 0.04}))},
                   {"rb", mc_field(90.0, fin::model::make(fin::model::Kind::RoughBergomi, {xi, eta, alpha, rho_rb}))},
                   {"bs", mc_field(80.0, fin::model::BlackScholes{.volatility = S({0.2})})}};
  chord.price_corr = pluss::make_series({1.0, 0.5, 0.4, 0.3, 0.5, 1.0, 0.2, 0.1, 0.4, 0.2, 1.0, 0.25, 0.3, 0.1, 0.25, 1.0}, {4, 4}, cfg);
  chord.var_corr = pluss::make_series({1.0, 0.3, 0.2, 0.0, 0.3, 1.0, 0.1, 0.0, 0.2, 0.1, 1.0, 0.0, 0.0, 0.0, 0.0, 1.0}, {4, 4}, cfg);
  const auto r = chord.simulate_paths();
  expect(r.asset.sizes() == std::vector<std::int64_t>({paths, steps + 1, 4}), "chord: rough paths must have the shape [paths, steps + 1, assets]");

  const std::int64_t n = paths * steps;
  const auto corr = [](const Series& x, const Series& y) {
    const Series a = x.sub(x.mean()), b = y.sub(y.mean());
    return a.mul(b).mean().item<double>() / std::sqrt(a.square().mean().item<double>() * b.square().mean().item<double>());
  };
  const auto w = [&](int a) { return r.vol_shocks.select(2, a).reshape({n}); };
  expect(std::abs(corr(w(0), w(1)) - 0.3) < 0.015 && std::abs(corr(w(0), w(2)) - 0.2) < 0.015 && std::abs(corr(w(1), w(2)) - 0.1) < 0.015,
         "chord: the second-factor shocks of rough and classical assets must carry var_corr");

  // rough Bergomi: v_T = xi exp(eta Y_T - eta^2 T^(2H) / 2) has mean xi, and log v_T has standard deviation eta T^H
  const double H = alpha + 0.5;
  const Series v_rb = r.variance.select(2, 2).select(1, steps);
  const double mean_v = v_rb.mean().item<double>(), se_v = std::sqrt(v_rb.sub(mean_v).square().mean().item<double>() / static_cast<double>(paths));
  expect(std::abs(mean_v - xi) < 4.0 * se_v, "chord: the rough Bergomi variance must have mean xi");
  const Series log_v = v_rb.log();
  const double sd_log = std::sqrt(log_v.sub(log_v.mean()).square().mean().item<double>());
  expect(std::abs(sd_log - eta * std::pow(1.0, H)) < 0.03, "chord: the log of the rough Bergomi variance must have standard deviation eta T^H (" + std::to_string(sd_log) + ")");
  // its variance moves against the price: log v_T against the sum of the price shocks of the same asset
  const Series shock_sum = r.asset_shocks.select(2, 2).sum(1);
  expect(corr(log_v, shock_sum) < -0.4, "chord: the rough Bergomi variance must carry the leverage rho (" + std::to_string(corr(log_v, shock_sum)) + ")");

  // rough Heston: same law as the single-asset simulation of the same Field, two independent estimates of the mean variance and of the forward
  const mod::Field single = chord.at("rh");
  const auto own = single.simulate_paths();
  const auto estimate = [](const Series& x) { return std::make_pair(x.mean().item<double>(), std::sqrt(x.sub(x.mean()).square().mean().item<double>() / static_cast<double>(x.numel()))); };
  const auto joint_v = estimate(r.variance.select(2, 0).select(1, steps)), alone_v = estimate(own.variance.select(1, steps));
  expect(std::abs(joint_v.first - alone_v.first) < 5.0 * std::hypot(joint_v.second, alone_v.second), "chord: the rough Heston variance must have the law of the single-asset simulation");
  const auto joint_s = estimate(r.asset.select(2, 0).select(1, steps)), alone_s = estimate(own.asset.select(1, steps));
  expect(std::abs(joint_s.first - alone_s.first) < 5.0 * std::hypot(joint_s.second, alone_s.second) && std::abs(joint_s.first - 100.0 * std::exp(0.01)) < 5.0 * joint_s.second,
         "chord: the rough Heston spot must have the law of the single-asset simulation and reproduce its forward");
}

// A parameter table with a model per asset builds all six models, reading from each column only what the model uses.
void test_from_model_params_all_models() {
  using K = fin::model::Kind;
  const double nan = std::numeric_limits<double>::quiet_NaN();
  mod::Chord::Parameters p;
  p.names = {"h", "bs", "s", "j", "rh", "rb"};
  p.kind = {K::Heston, K::BlackScholes, K::Sabr, K::Bates, K::RoughHeston, K::RoughBergomi};
  p.spot = {100.0, 80.0, 120.0, 90.0, 100.0, 95.0};
  p.rate = {0.02};
  p.volatility = {nan, 0.2, nan, nan, nan, nan};
  p.alpha0 = {nan, nan, 0.3, nan, nan, nan};
  p.beta = {nan, nan, 0.7, nan, nan, nan};
  p.nu = {nan, nan, 0.5, nan, nan, nan};
  p.kappa = {1.5, nan, nan, 1.0, 1.5, nan};
  p.theta = {0.04, nan, nan, 0.05, 0.04, nan};
  p.sigma = {0.3, nan, nan, 0.25, 0.3, nan};
  p.rho = {-0.7, nan, -0.3, -0.4, -0.7, -0.7};
  p.v0 = {0.04, nan, nan, 0.05, 0.04, nan};
  p.jump_intensity = {nan, nan, nan, 0.3, nan, nan};
  p.jump_mean = {nan, nan, nan, -0.1, nan, nan};
  p.jump_vol = {nan, nan, nan, 0.15, nan, nan};
  p.alpha = {nan, nan, nan, nan, -0.2, -0.3};
  p.xi = {nan, nan, nan, nan, nan, 0.04};
  p.eta = {nan, nan, nan, nan, nan, 1.2};
  p.controls.mc.paths = 2000;
  p.controls.mc.steps = 10;
  const mod::Chord chord = mod::Chord::from_model_params(p);
  expect(chord.size() == 6 && chord.at("h").model.holds<fin::model::Heston>() && chord.at("bs").model.holds<fin::model::BlackScholes>() && chord.at("s").model.holds<fin::model::Sabr>() &&
             chord.at("j").model.holds<fin::model::Bates>() && chord.at("rh").model.holds<fin::model::RoughHeston>() && chord.at("rb").model.holds<fin::model::RoughBergomi>(),
         "chord: a kind column must give each asset its model");
  expect(chord.at("s").model.parameters().items<double>() == std::vector<double>({0.3, 0.7, 0.5, -0.3}) && chord.at("rb").model.parameters().items<double>() == std::vector<double>({0.04, 1.2, -0.3, -0.7}) &&
             chord.at("rh").model.parameters().items<double>() == std::vector<double>({1.5, 0.04, 0.3, -0.2, -0.7, 0.04}),
         "chord: each model must read its own columns in its own order");
  expect(chord.simulate_paths().asset.sizes() == std::vector<std::int64_t>({2000, 11, 6}), "chord: a table of all six models must simulate together");

  const auto refused = [](auto&& f) {
    try { f(); } catch (const std::exception&) { return true; }
    return false;
  };
  mod::Chord::Parameters unused = p;
  unused.kind = {K::Heston};
  unused.kind.resize(6, K::Heston);
  expect(refused([&] { (void)mod::Chord::from_model_params(unused); }), "chord: columns no model of the table uses must be refused");
  mod::Chord::Parameters nan_entry = p;
  nan_entry.volatility = {nan, nan, nan, nan, nan, nan};
  expect(refused([&] { (void)mod::Chord::from_model_params(nan_entry); }), "chord: a missing entry for an asset whose model reads the column must be refused");
  mod::Chord::Parameters missing = p;
  missing.xi.clear();
  expect(refused([&] { (void)mod::Chord::from_model_params(missing); }), "chord: a column a model needs and the table lacks must be refused");
}

void test_implied_vol_and_errors() {
  mod::Chord chord;
  chord.add("flat", bs(100.0, 100.0, 0.25)).add("smile", heston(100.0, {90.0, 110.0}));
  const auto iv = chord.implied_vol();
  expect(iv.size() == 2 && std::abs(iv[0].items<double>()[0] - 0.25) < 1e-6 && iv[1].numel() == 2 && iv[1].items<double>()[0] > iv[1].items<double>()[1],
         "chord: implied volatilities must be per asset, flat for Black-Scholes and skewed for Heston");

  bool refused = false;
  try {
    chord.add("flat", bs(1.0, 1.0, 0.1));
  } catch (const std::exception&) {
    refused = true;
  }
  expect(refused, "chord: a repeated name must be refused");

  mod::Chord bad;
  bad.add("good", bs(100.0, 100.0, 0.2)).add("worse", bs(-1.0, 100.0, 0.0));
  std::string message;
  try {
    bad.price();
  } catch (const std::exception& e) {
    message = e.what();
  }
  expect(message.find("asset 'worse'") != std::string::npos && message.find("good") == std::string::npos, "chord: validation must name the failing asset only");
}

// Concurrent execution must not change a single bit: engines of every kind, priced on the calling thread and on many threads, repeatedly.
void test_parallel_equals_serial() {
  mod::Chord chord;
  for (int i = 0; i < 6; ++i) {
    chord.add("hc" + std::to_string(i), heston(100.0 + i, {90.0 + i, 100.0, 110.0}, {1.0 + 0.3 * i, 0.04, 0.3, -0.7, 0.04}));
    mod::Field f = heston(100.0, {95.0, 105.0}, {1.5, 0.04 + 0.01 * i, 0.3, -0.5, 0.04});
    f.controls.engine = mod::Engine::Fft;
    chord.add("hf" + std::to_string(i), f);
    mod::Field mc = heston(100.0, {100.0}, {1.5, 0.04, 0.3 + 0.05 * i, -0.7, 0.04});
    mc.controls.engine = mod::Engine::MonteCarlo;
    mc.controls.mc.paths = 4000;
    mc.controls.mc.steps = 20;
    mc.controls.mc.seed = 17 + i;
    chord.add("mc" + std::to_string(i), mc);
    mod::Field lat = bs(100.0, 100.0 + i, 0.2);
    lat.controls.engine = mod::Engine::Lattice;
    lat.controls.lattice.steps = 300;
    chord.add("lat" + std::to_string(i), lat);
    mod::Field am{.instrument = fin::instrument::american(fin::pricing::OptionType::Put, 100.0, 1.0), .market = fin::market::make(100.0, 0.05, 0.0),
                  .model = fin::model::BlackScholes{.volatility = S({0.2 + 0.02 * i})}, .controls = {.engine = mod::Engine::Spectral}};
    chord.add("am" + std::to_string(i), am);
  }
  chord.parallel = false;
  const auto serial = chord.price();
  chord.parallel = true;
  for (int round = 0; round < 5; ++round) {
    const auto parallel = chord.price();
    expect(parallel.price.numel() == serial.price.numel(), "chord: parallel result length must equal serial");
    expect(worst(parallel.price, serial.price) == 0.0, "chord: parallel prices must equal serial prices bit for bit");
    for (const auto& name : chord.names()) {
      const auto& a = parallel[name];
      const auto& b = serial[name];
      if (b.greeks.delta.defined()) expect(worst(a.greeks.delta, b.greeks.delta) == 0.0 && worst(a.greeks.vega, b.greeks.vega) == 0.0, "chord: parallel Greeks must equal serial Greeks: " + name);
    }
  }
}

// Heston and Bates COS assets with different parameters, rates and spots in one vectorised call agree with each asset priced alone, puts and calls mixed.
void test_cos_batches_across_parameters() {
  mod::Chord chord;
  for (int i = 0; i < 4; ++i) {
    mod::Field f = heston(90.0 + 5.0 * i, {85.0 + 5.0 * i, 100.0}, {1.0 + 0.5 * i, 0.03 + 0.01 * i, 0.2 + 0.1 * i, -0.8 + 0.2 * i, 0.04});
    f.market.rate = S({0.01 * i});
    f.instrument.type = i % 2 ? fin::pricing::OptionType::Put : fin::pricing::OptionType::Call;
    f.controls.risk = mod::Risk::All;
    chord.add("h" + std::to_string(i), f);
  }
  for (int i = 0; i < 3; ++i) {
    mod::Field f{.instrument = {.type = fin::pricing::OptionType::Call, .strike = S({95.0, 110.0}), .maturity = S({0.5 + 0.5 * i})}, .market = fin::market::make(100.0, 0.03, 0.01),
                 .model = fin::model::make(fin::model::Kind::Bates, {1.5, 0.04, 0.3, -0.7, 0.04, 0.1 + 0.2 * i, -0.1, 0.15 + 0.05 * i}),
                 .controls = {.engine = mod::Engine::Cos, .risk = mod::Risk::PriceOnly}};
    chord.add("b" + std::to_string(i), f);
  }
  expect(chord.batches() == 2, "chord: Heston and Bates COS assets form one call each");
  const auto r = chord.price();
  for (const auto& name : chord.names()) {
    expect(worst(r[name].price, chord.at(name).price().price) < 1e-8, "chord: batched COS price must equal the asset priced alone: " + name);
  }
  expect(worst(r["h0"].greeks.delta, chord.at("h0").price().greeks.delta) < 1e-6, "chord: batched COS Greeks must equal the asset priced alone");
}

// Correlated simulation of several Heston / Bates assets in one pass: shapes, marginal laws, and the cross-asset correlation of the shocks.
void test_correlated_simulation() {
  const auto cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float64);
  const auto mc = [&](fin::model::Kind kind, std::vector<double> params, double spot, double rate) {
    mod::Field f{.instrument = fin::instrument::make(fin::pricing::OptionType::Call, 100.0, 1.0), .market = fin::market::make(spot, rate, 0.01), .model = fin::model::make(kind, params),
                 .controls = {.engine = mod::Engine::MonteCarlo, .risk = mod::Risk::PriceOnly}};
    f.controls.mc.paths = 40000;
    f.controls.mc.steps = 50;
    f.controls.mc.seed = 5;
    return f;
  };
  mod::Chord chord;
  chord.add("a", mc(fin::model::Kind::Heston, {1.5, 0.04, 0.3, -0.7, 0.09}, 100.0, 0.03))
      .add("b", mc(fin::model::Kind::Heston, {2.0, 0.06, 0.4, 0.0, 0.03}, 50.0, 0.01))
      .add("c", mc(fin::model::Kind::Bates, {1.0, 0.05, 0.25, -0.4, 0.05, 0.3, -0.1, 0.15}, 80.0, 0.02));
  chord.price_corr = pluss::make_series({1.0, 0.6, -0.3, 0.6, 1.0, 0.2, -0.3, 0.2, 1.0}, {3, 3}, cfg);
  chord.var_corr = pluss::make_series({1.0, 0.5, 0.0, 0.5, 1.0, 0.0, 0.0, 0.0, 1.0}, {3, 3}, cfg);
  const auto r = chord.simulate_paths();
  expect(r.asset.sizes() == std::vector<std::int64_t>({40000, 51, 3}) && r.asset_shocks.sizes() == std::vector<std::int64_t>({40000, 50, 3}), "chord: path shapes must be [paths, steps + 1, assets]");

  const auto corr = [&](const Series& z, int i, int j) {
    const Series a = z.select(2, i).reshape({z.sizes()[0] * z.sizes()[1]}), b = z.select(2, j).reshape({z.sizes()[0] * z.sizes()[1]});
    return (a * b).mean().item<double>() / std::sqrt(a.square().mean().item<double>() * b.square().mean().item<double>());
  };
  expect(std::abs(corr(r.asset_shocks, 0, 1) - 0.6) < 0.01 && std::abs(corr(r.asset_shocks, 0, 2) + 0.3) < 0.01 && std::abs(corr(r.asset_shocks, 1, 2) - 0.2) < 0.01,
         "chord: the price shocks must carry the requested cross-asset correlation");
  // var_corr is the correlation of the variance shocks themselves, so that is what the shocks must carry; the cross terms are built as rho_b R_s[a][b].
  expect(std::abs(corr(r.vol_shocks, 0, 1) - 0.5) < 0.01 && std::abs(corr(r.vol_shocks, 0, 2)) < 0.01 && std::abs(corr(r.vol_shocks, 1, 2)) < 0.01,
         "chord: the variance shocks must carry the requested var_corr");
  expect(std::abs(corr(r.asset_shocks, 0, 1) - 0.6) < 0.01, "chord: the price shocks must still carry price_corr");
  const auto cross = [&](int a, int b) {
    const Series x = r.asset_shocks.select(2, a).reshape({r.asset_shocks.sizes()[0] * r.asset_shocks.sizes()[1]}), y = r.vol_shocks.select(2, b).reshape({r.vol_shocks.sizes()[0] * r.vol_shocks.sizes()[1]});
    return (x * y).mean().item<double>() / std::sqrt(x.square().mean().item<double>() * y.square().mean().item<double>());
  };
  const double rho_of[3] = {-0.7, 0.0, -0.4};
  const double rs[3][3] = {{1.0, 0.6, -0.3}, {0.6, 1.0, 0.2}, {-0.3, 0.2, 1.0}};
  expect(std::abs(cross(0, 1) - rho_of[1] * rs[0][1]) < 0.01 && std::abs(cross(1, 0) - rho_of[0] * rs[1][0]) < 0.01 && std::abs(cross(0, 2) - rho_of[2] * rs[0][2]) < 0.01,
         "chord: the cross terms must be built as rho_b price_corr[a][b]");

  const double growth[3] = {0.02, 0.0, 0.01};
  const double spot0[3] = {100.0, 50.0, 80.0};
  const double v0[3] = {0.09, 0.03, 0.05}, kappa[3] = {1.5, 2.0, 1.0}, theta[3] = {0.04, 0.06, 0.05};
  for (int i = 0; i < 3; ++i) {
    const Series terminal = r.asset.select(2, i).select(1, 50);
    const double mean = terminal.mean().item<double>();
    const double se = std::sqrt(terminal.sub(mean).square().mean().item<double>() / 40000.0);
    expect(std::abs(mean - spot0[i] * std::exp(growth[i])) < 4.0 * se, "chord: every asset must reproduce its forward");
    const double ev = r.variance.select(2, i).select(1, 50).mean().item<double>();
    expect(std::abs(ev - (theta[i] + (v0[i] - theta[i]) * std::exp(-kappa[i]))) < 2e-3, "chord: every asset must reproduce its mean variance");
  }

  mod::Chord independent = chord;
  independent.price_corr = pluss::series::undefined();
  independent.var_corr = pluss::series::undefined();
  expect(std::abs(corr(independent.simulate_paths().asset_shocks, 0, 1)) < 0.01, "chord: without a correlation the assets must be independent");

  // A var_corr that the price correlation and the rho of the assets cannot produce (a correlation of 0.99 between a variance shock with rho = -0.7 and one with rho = 0)
  // is refused, and so is one that is not symmetric.
  {
    mod::Chord inconsistent = chord;
    inconsistent.price_corr = pluss::series::undefined();
    inconsistent.var_corr = pluss::make_series({1.0, 0.99, 0.0, 0.99, 1.0, 0.0, 0.0, 0.0, 1.0}, {3, 3}, cfg);
    std::string message;
    try { (void)inconsistent.simulate_paths(); } catch (const std::exception& e) { message = e.what(); }
    expect(message.find("var_corr") != std::string::npos, "chord: a var_corr that rho and price_corr cannot produce must be refused, naming var_corr");
    mod::Chord asymmetric = chord;
    asymmetric.var_corr = pluss::make_series({1.0, 0.2, 0.0, 0.02, 1.0, 0.0, 0.0, 0.0, 1.0}, {3, 3}, cfg);
    message.clear();
    try { (void)asymmetric.simulate_paths(); } catch (const std::exception& e) { message = e.what(); }
    expect(message.find("var_corr must be symmetric") != std::string::npos, "chord: an asymmetric var_corr must be refused");
  }

  bool refused = false;
  mod::Chord bad = chord;
  bad.price_corr = pluss::make_series({1.0, 0.9, 0.9, 0.9, 1.0, -0.9, 0.9, -0.9, 1.0}, {3, 3}, cfg);
  try { (void)bad.simulate_paths(); } catch (const std::exception&) { refused = true; }
  expect(refused, "chord: a correlation matrix that is not positive definite must be refused");
  // every model of a Field can be simulated with the others (see test_mixed_models and test_rough_models_in_the_mix)
}

// An explicit cross correlation between the price shock of one asset and the variance shock of another.
void test_cross_price_var_cross() {
  const auto cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float64);
  mod::Field f{.instrument = fin::instrument::make(fin::pricing::OptionType::Call, 100.0, 1.0), .market = fin::market::make(100.0, 0.03, 0.0),
               .model = fin::model::make(fin::model::Kind::Heston, {1.5, 0.04, 0.3, -0.5, 0.04}), .controls = {.engine = mod::Engine::MonteCarlo}};
  f.controls.mc.paths = 40000;
  f.controls.mc.steps = 50;
  f.controls.mc.seed = 9;
  mod::Chord chord;
  chord.add("a", f).add("b", f);
  chord.var_corr = pluss::make_series({1.0, 0.4, 0.4, 1.0}, {2, 2}, cfg);
  chord.price_var_cross = pluss::make_series({0.0, 0.3, -0.2, 0.0}, {2, 2}, cfg);   // the diagonal is the asset's own rho
  const auto r = chord.simulate_paths();
  const auto corr = [&](const Series& u, int i, const Series& v, int j) {
    const Series a = u.select(2, i).reshape({u.sizes()[0] * u.sizes()[1]}), b = v.select(2, j).reshape({v.sizes()[0] * v.sizes()[1]});
    return (a * b).mean().item<double>() / std::sqrt(a.square().mean().item<double>() * b.square().mean().item<double>());
  };
  expect(std::abs(corr(r.asset_shocks, 0, r.vol_shocks, 1) - 0.3) < 0.01 && std::abs(corr(r.asset_shocks, 1, r.vol_shocks, 0) + 0.2) < 0.01, "chord: the cross price-variance correlation must be as requested");
  expect(std::abs(corr(r.asset_shocks, 0, r.vol_shocks, 0) + 0.5) < 0.01, "chord: the within-asset correlation must stay rho");
  expect(std::abs(corr(r.vol_shocks, 0, r.vol_shocks, 1) - 0.4) < 0.01 && std::abs(corr(r.asset_shocks, 0, r.asset_shocks, 1)) < 0.01, "chord: the variance shocks must carry var_corr directly");
  bool refused = false;
  chord.price_var_cross = pluss::make_series({0.0, 0.99, 0.99, 0.0}, {2, 2}, cfg);
  try { (void)chord.simulate_paths(); } catch (const std::exception&) { refused = true; }
  expect(refused, "chord: a joint correlation that is not positive definite must be refused");
}

// Monte Carlo assets with the same controls share one simulation with an asset axis; the results agree with each asset simulated alone to Monte Carlo error.
void test_monte_carlo_assets_share_one_simulation() {
  mod::Chord chord;
  for (int i = 0; i < 5; ++i) {
    mod::Field f{.instrument = {.type = i % 2 ? fin::pricing::OptionType::Put : fin::pricing::OptionType::Call, .strike = S({90.0 + 5.0 * i, 100.0 + 5.0 * i}), .maturity = S({1.0})},
                 .market = fin::market::make(100.0 + 5.0 * i, 0.02 + 0.005 * i, 0.01),
                 .model = i == 4 ? fin::model::make(fin::model::Kind::Bates, {1.5, 0.04, 0.3, -0.7, 0.04, 0.2, -0.1, 0.15})
                                 : fin::model::make(fin::model::Kind::Heston, {1.0 + 0.5 * i, 0.04 + 0.01 * i, 0.3, -0.7 + 0.1 * i, 0.04}),
                 .controls = {.engine = mod::Engine::MonteCarlo, .risk = mod::Risk::All}};
    f.controls.mc.paths = 80000;
    f.controls.mc.steps = 60;
    f.controls.mc.scheme = fin::volatility::SimScheme::QuadExp;
    f.controls.mc.seed = 21;
    chord.add("m" + std::to_string(i), f);
  }
  expect(chord.batches() == 1, "chord: five Monte Carlo Heston / Bates assets with the same controls share one simulation");
  const auto together = chord.price();
  for (const auto& name : chord.names()) {
    const auto alone = chord.at(name).price();
    expect(worst(together[name].price, alone.price) < 0.25, "chord: merged Monte Carlo price must agree with the asset alone: " + name);
    expect(worst(together[name].greeks.delta, alone.greeks.delta) < 0.03 && worst(together[name].greeks.rho, alone.greeks.rho) < 3.0, "chord: merged Monte Carlo Greeks must agree with the asset alone: " + name);
  }
  // a control variate or other controls keep an asset on its own
  chord.at("m0").controls.mc.control_variate = true;
  expect(chord.batches() == 2, "chord: a control variate takes an asset out of the shared simulation");
}

} // namespace

int main() {
  try {
    test_black_scholes_batches_into_one_call();
    test_equal_parameters_batch_and_others_do_not();
    test_cos_batches_across_parameters();
    test_dictionary_construction_and_add();
    test_from_model_params();
    test_quadexp_correlation_across_assets();
    test_mixed_models();
    test_rough_models_in_the_mix();
    test_from_model_params_all_models();
    test_implied_vol_and_errors();
    test_parallel_equals_serial();
    test_correlated_simulation();
    test_cross_price_var_cross();
    test_monte_carlo_assets_share_one_simulation();
    std::cout << "chord_test passed\n";
    return 0;
  } catch (const std::exception& e) {
    std::cerr << "chord_test failed: " << e.what() << '\n';
    return 1;
  }
}
