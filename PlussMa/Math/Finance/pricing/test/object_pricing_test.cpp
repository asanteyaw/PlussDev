#include <pluss/ma.h>

#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

using pluss::Config;
using pluss::ScalarType;
using pluss::Series;
namespace obj = pluss::subsp::math::finance::object;
namespace prc = pluss::subsp::math::finance::pricing;
namespace vol = pluss::subsp::math::finance::volatility;
namespace mdl = pluss::subsp::math::finance::models;

void expect(bool cond, const std::string& msg) {
  if (!cond) {
    throw std::runtime_error(msg);
  }
}

Config f64_cpu() {
  return Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);
}

void test_bs_analytic_series_lane() {
  const Config cfg = f64_cpu();

  const obj::Vanilla inst{
      .type = prc::OptionType::Call,
      .strike = pluss::series::full({5}, 100.0, cfg),
      .maturity = pluss::series::full({5}, 1.0, cfg),
  };
  const obj::MarketState mkt{
      .spot = pluss::series::linspace(80.0, 120.0, 5, cfg),
      .rate = pluss::series::full({1}, 0.03, cfg),
      .dividend_yield = pluss::series::full({1}, 0.01, cfg),
  };
  const obj::model::BlackScholes mdl{
      .volatility = pluss::series::full({5}, 0.20, cfg),
  };

  const auto out = obj::price(inst, mdl, mkt, obj::pricer::Analytic{.with_greeks = true}, cfg);
  const auto ref = prc::black_scholes_price(prc::OptionType::Call, mkt.spot, 100.0, 0.03, 0.01, 1.0, 0.20, cfg);
  const double err = out.price.sub(ref).abs().max().item<double>();

  expect(out.price.numel() == 5, "object bs analytic: output size mismatch");
  expect(out.delta.defined() && out.gamma.defined() && out.vega.defined(), "object bs analytic: greek outputs missing");
  expect(err < 1e-10, "object bs analytic: must match functional black_scholes_price");
}

void test_bs_analytic_auto_mixed_lane() {
  const Config cfg = f64_cpu();

  const Series is_call = pluss::make_series(std::vector<double>{1.0, 0.0, 1.0, 0.0}, {4}, cfg);
  const obj::Vanilla inst{
      .type = prc::OptionType::Auto,
      .is_call = is_call,
      .strike = pluss::series::full({4}, 100.0, cfg),
      .maturity = pluss::series::full({4}, 1.0, cfg),
  };
  const obj::MarketState mkt{
      .spot = pluss::series::linspace(90.0, 110.0, 4, cfg),
      .rate = pluss::series::full({1}, 0.03, cfg),
      .dividend_yield = pluss::series::full({1}, 0.01, cfg),
  };
  const obj::model::BlackScholes mdl{
      .volatility = pluss::series::full({1}, 0.20, cfg),
  };

  const auto out = obj::price(inst, mdl, mkt, obj::pricer::Analytic{.with_greeks = false}, cfg);
  const Series call_px = prc::black_scholes_price(prc::OptionType::Call, mkt.spot, 100.0, 0.03, 0.01, 1.0, 0.20, cfg);
  const Series put_px = prc::black_scholes_price(prc::OptionType::Put, mkt.spot, 100.0, 0.03, 0.01, 1.0, 0.20, cfg);
  const Series expected = call_px.mul(is_call).add(put_px.mul(is_call.mul(-1.0).add(1.0)));
  const double err = out.price.sub(expected).abs().max().item<double>();

  expect(err < 1e-10, "object bs analytic auto: mixed call/put output mismatch");
}

void test_bs_mc_series_lane() {
  const Config cfg = f64_cpu();

  const obj::Vanilla inst{
      .type = prc::OptionType::Put,
      .strike = pluss::series::full({1}, 100.0, cfg),
      .maturity = pluss::series::full({1}, 1.0, cfg),
  };
  const obj::MarketState mkt{
      .spot = pluss::series::full({1}, 100.0, cfg),
      .rate = pluss::series::full({1}, 0.03, cfg),
      .dividend_yield = pluss::series::full({1}, 0.01, cfg),
  };
  const obj::model::BlackScholes mdl{
      .volatility = pluss::series::full({1}, 0.20, cfg),
  };
  obj::pricer::MonteCarlo mc;
  mc.options.paths = 120000;
  mc.options.antithetic = true;
  mc.options.seed = 11;

  const auto out_mc = obj::price(inst, mdl, mkt, mc, cfg);
  const auto out_an = obj::price(inst, mdl, mkt, obj::pricer::Analytic{.with_greeks = false}, cfg);
  const double mc_px = out_mc.price.reshape({1}).host<double>().front();
  const double an_px = out_an.price.reshape({1}).host<double>().front();

  expect(std::isfinite(mc_px), "object bs mc: price must be finite");
  expect(std::abs(mc_px - an_px) < 0.08, "object bs mc: must be close to analytic price");
}

void test_heston_object_finite() {
  const Config cfg = f64_cpu();

  const obj::Vanilla inst{
      .type = prc::OptionType::Call,
      .strike = pluss::series::full({1}, 100.0, cfg),
      .maturity = pluss::series::full({1}, 1.0, cfg),
  };
  const obj::MarketState mkt{
      .spot = pluss::series::full({1}, 100.0, cfg),
      .rate = pluss::series::full({1}, 0.03, cfg),
      .dividend_yield = pluss::series::full({1}, 0.01, cfg),
  };
  const obj::model::Heston mdl{
      .kappa = pluss::series::full({1}, 1.4, cfg),
      .theta = pluss::series::full({1}, 0.04, cfg),
      .sigma = pluss::series::full({1}, 0.35, cfg),
      .rho = pluss::series::full({1}, -0.65, cfg),
      .v0 = pluss::series::full({1}, 0.04, cfg),
  };

  const auto out_fft = obj::price(inst, mdl, mkt, obj::pricer::Fft{}, cfg);
  obj::pricer::MonteCarlo mc;
  mc.options.paths = 60000;
  mc.options.steps = 252;
  mc.options.antithetic = true;
  mc.options.seed = 5;
  const auto out_mc = obj::price(inst, mdl, mkt, mc, cfg);

  expect(std::isfinite(out_fft.price.reshape({1}).host<double>().front()), "object heston fft: price must be finite");
  expect(std::isfinite(out_mc.price.reshape({1}).host<double>().front()), "object heston mc: price must be finite");
}

void test_bates_fft_object_finite() {
  const Config cfg = f64_cpu();

  const obj::Vanilla inst{
      .type = prc::OptionType::Call,
      .strike = pluss::series::full({1}, 100.0, cfg),
      .maturity = pluss::series::full({1}, 1.0, cfg),
  };
  const obj::MarketState mkt{
      .spot = pluss::series::full({1}, 100.0, cfg),
      .rate = pluss::series::full({1}, 0.03, cfg),
      .dividend_yield = pluss::series::full({1}, 0.01, cfg),
  };
  const obj::model::Bates mdl{
      .kappa = pluss::series::full({1}, 1.4, cfg),
      .theta = pluss::series::full({1}, 0.04, cfg),
      .sigma = pluss::series::full({1}, 0.35, cfg),
      .rho = pluss::series::full({1}, -0.65, cfg),
      .v0 = pluss::series::full({1}, 0.04, cfg),
      .jump_intensity = pluss::series::full({1}, 0.15, cfg),
      .jump_mean = pluss::series::full({1}, -0.08, cfg),
      .jump_vol = pluss::series::full({1}, 0.2, cfg),
  };

  const auto out_fft = obj::price(inst, mdl, mkt, obj::pricer::Fft{}, cfg);
  expect(std::isfinite(out_fft.price.reshape({1}).host<double>().front()), "object bates fft: price must be finite");
}

void test_sabr_object_finite() {
  const Config cfg = f64_cpu();

  const obj::Vanilla inst{
      .type = prc::OptionType::Call,
      .strike = pluss::series::full({1}, 100.0, cfg),
      .maturity = pluss::series::full({1}, 1.0, cfg),
  };
  const obj::MarketState mkt{
      .spot = pluss::series::full({1}, 100.0, cfg),
      .rate = pluss::series::full({1}, 0.02, cfg),
      .dividend_yield = pluss::series::full({1}, 0.0, cfg),
  };
  const obj::model::Sabr mdl{
      .alpha0 = pluss::series::full({1}, 0.2, cfg),
      .beta = pluss::series::full({1}, 0.5, cfg),
      .nu = pluss::series::full({1}, 0.4, cfg),
      .rho = pluss::series::full({1}, -0.3, cfg),
      .vol_type = prc::SabrVolType::Lognormal,
      .approx = prc::SabrApproxOptions{
          .model = prc::SabrApproxModel::Hagan2002,
          .shift = 0.0,
      },
  };

  const auto out_an = obj::price(inst, mdl, mkt, obj::pricer::Analytic{}, cfg);
  obj::pricer::MonteCarlo mc;
  mc.options.paths = 30000;
  mc.options.steps = 252;
  mc.options.antithetic = true;
  mc.options.seed = 7;
  const auto out_mc = obj::price(inst, mdl, mkt, mc, cfg);

  expect(std::isfinite(out_an.price.reshape({1}).host<double>().front()), "object sabr analytic: price must be finite");
  expect(std::isfinite(out_mc.price.reshape({1}).host<double>().front()), "object sabr mc: price must be finite");
}

void test_models_vectorized_overloads() {
  const Config cfg = f64_cpu();

  mdl::Heston h(vol::HestonParams{
      .kappa = 1.4,
      .theta = 0.04,
      .sigma = 0.35,
      .rho = -0.65,
      .v0 = 0.04,
  });
  const Series spot = pluss::series::full({4}, 100.0, cfg);
  const Series strike = pluss::series::linspace(90.0, 110.0, 4, cfg);
  const Series rate = pluss::series::full({4}, 0.03, cfg);
  const Series div = pluss::series::full({4}, 0.01, cfg);
  const Series tau = pluss::series::full({4}, 1.0, cfg);

  const Series px_fft = h.price(prc::OptionType::Call, spot, strike, rate, div, tau, prc::HestonFftOptions{}, cfg);
  const Series px_mc = h.price_mc(prc::OptionType::Call, spot, strike, rate, div, tau, prc::MonteCarloOptions{}, cfg);
  expect(px_fft.numel() == 4, "models::Heston vectorized price output size mismatch");
  expect(px_mc.numel() == 4, "models::Heston vectorized price_mc output size mismatch");
  expect(px_fft.isfinite().all().item<double>() == 1.0, "models::Heston vectorized price must be finite");
  expect(px_mc.isfinite().all().item<double>() == 1.0, "models::Heston vectorized price_mc must be finite");

  mdl::Bates b(vol::BatesParams{});
  const Series px_b_mc = b.price_mc(prc::OptionType::Call, spot, strike, rate, div, tau, prc::MonteCarloOptions{}, cfg);
  expect(px_b_mc.numel() == 4, "models::Bates vectorized price_mc output size mismatch");
  expect(px_b_mc.isfinite().all().item<double>() == 1.0, "models::Bates vectorized price_mc must be finite");
}

} // namespace

int main() {
  try {
    test_bs_analytic_series_lane();
    test_bs_analytic_auto_mixed_lane();
    test_bs_mc_series_lane();
    test_heston_object_finite();
    test_bates_fft_object_finite();
    test_sabr_object_finite();
    test_models_vectorized_overloads();
    std::cout << "object_pricing_test passed\n";
    return 0;
  } catch (const std::exception& e) {
    std::cerr << "object_pricing_test failed: " << e.what() << '\n';
    return 1;
  }
}
