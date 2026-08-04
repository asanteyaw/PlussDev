#include <catch2/catch_test_macros.hpp>

#include <pluss/ma.h>

#include <cmath>
#include <vector>

namespace {

using pluss::Config;
using pluss::ScalarType;
namespace fin = pluss::subsp::math::finance;
namespace prc = pluss::subsp::math::finance::pricing;
namespace vol = pluss::subsp::math::finance::volatility;

Config f64_cpu() {
  return Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);
}

double max_abs_diff(const pluss::Series& a, const pluss::Series& b) {
  return a.sub(b).abs().max().item<double>();
}

bool all_finite(const pluss::Series& x) {
  return x.defined() && x.isfinite().all().item<double>() == 1.0;
}

} // namespace

TEST_CASE("COS pricing tracks Heston NI and FFT across strikes", "[finance][pricing][cos][heston]") {
  const auto cfg = f64_cpu();
  vol::HestonParams p{
      .kappa = 1.9,
      .theta = 0.04,
      .sigma = 0.32,
      .rho = -0.7,
      .v0 = 0.045,
  };
  prc::HestonIntegrationOptions ni_opt;
  ni_opt.integration_steps = 3200;
  ni_opt.integration_u_max = 180.0;
  ni_opt.mode = prc::HestonCfMode::LittleTrap;

  prc::HestonFftOptions fft_opt;
  fft_opt.num_fft = 4096;
  fft_opt.characteristic_fcn_step = 0.01;
  fft_opt.damping_factor = 1.5;
  fft_opt.mode = prc::HestonCfMode::LittleTrap;

  prc::CosOptions cos_opt;
  cos_opt.num_terms = 2048;
  cos_opt.truncation = 12.0;
  cos_opt.cumulant_step = 1e-3;

  const double s = 100.0;
  const double r = 0.02;
  const double q = 0.0;
  const double t = 1.0;
  const std::vector<double> strikes{70.0, 80.0, 90.0, 100.0, 110.0, 120.0, 130.0};

  for (double k : strikes) {
    const double px_ni = prc::heston_price(prc::OptionType::Call, s, k, r, q, t, p, ni_opt);
    const double px_fft = prc::heston_price_fft(prc::OptionType::Call, s, k, r, q, t, p, fft_opt, cfg);
    const double px_cos = prc::heston_price_cos(prc::OptionType::Call, s, k, r, q, t, p, cos_opt, cfg);
    REQUIRE(std::isfinite(px_cos));
    REQUIRE(px_cos > 0.0);
    REQUIRE(std::abs(px_cos - px_ni) < 0.45);
    REQUIRE(std::abs(px_cos - px_fft) < 0.6);
  }

  const double k = 105.0;
  const double c = prc::heston_price_cos(prc::OptionType::Call, s, k, r, q, t, p, cos_opt, cfg);
  const double pput = prc::heston_price_cos(prc::OptionType::Put, s, k, r, q, t, p, cos_opt, cfg);
  const double parity = s * std::exp(-q * t) - k * std::exp(-r * t);
  REQUIRE(std::abs((c - pput) - parity) < 0.2);

  const auto spot_s = pluss::series::full({static_cast<std::int64_t>(strikes.size())}, s, cfg);
  const auto strike_s = pluss::make_series(strikes, {static_cast<std::int64_t>(strikes.size())}, cfg);
  const auto maturity_s = pluss::series::full({static_cast<std::int64_t>(strikes.size())}, t, cfg);
  const auto series_px = prc::heston_price_cos(prc::OptionType::Call, spot_s, strike_s, r, q, maturity_s, p, cos_opt, cfg);
  REQUIRE(series_px.numel() == static_cast<std::int64_t>(strikes.size()));
  REQUIRE(all_finite(series_px));
  for (std::int64_t i = 0; i < series_px.numel(); ++i) {
    const double scalar_px = prc::heston_price_cos(
        prc::OptionType::Call, s, strikes[static_cast<std::size_t>(i)], r, q, t, p, cos_opt, cfg);
    REQUIRE(std::abs(series_px.select(0, i).item<double>() - scalar_px) < 1.0e-10);
  }
}

TEST_CASE("COS pricing tracks Bates FFT", "[finance][pricing][cos][bates]") {
  const auto cfg = f64_cpu();
  vol::BatesParams p{
      .kappa = 1.6,
      .theta = 0.04,
      .sigma = 0.30,
      .rho = -0.65,
      .v0 = 0.04,
      .jump_intensity = 0.18,
      .jump_mean = -0.08,
      .jump_vol = 0.22,
  };
  prc::HestonFftOptions fft_opt;
  fft_opt.num_fft = 4096;
  fft_opt.characteristic_fcn_step = 0.01;
  fft_opt.damping_factor = 1.5;

  prc::CosOptions cos_opt;
  cos_opt.num_terms = 2048;
  cos_opt.truncation = 12.0;
  cos_opt.cumulant_step = 1e-3;

  const double s = 100.0;
  const double r = 0.02;
  const double q = 0.0;
  const double t = 1.0;
  for (double k : {80.0, 90.0, 100.0, 110.0, 120.0}) {
    const double px_fft = prc::bates_price_fft(prc::OptionType::Call, s, k, r, q, t, p, fft_opt, cfg);
    const double px_cos = prc::bates_price_cos(prc::OptionType::Call, s, k, r, q, t, p, cos_opt, cfg);
    REQUIRE(std::isfinite(px_cos));
    REQUIRE(px_cos > 0.0);
    REQUIRE(std::abs(px_cos - px_fft) < 0.6);
  }
}

TEST_CASE("Object FFT and COS expose greeks for Heston/Bates", "[finance][pricing][greeks][object]") {
  const auto cfg = f64_cpu();

  const auto inst = fin::instrument::Vanilla{
      .type = prc::OptionType::Call,
      .strike = pluss::series::linspace(85.0, 115.0, 7, cfg),
      .maturity = pluss::series::full({7}, 1.0, cfg),
  };
  const auto mkt = fin::market::State{
      .spot = pluss::series::full({7}, 100.0, cfg),
      .rate = pluss::series::full({7}, 0.02, cfg),
      .dividend_yield = pluss::series::full({7}, 0.0, cfg),
  };

  const auto heston = fin::model::Heston{
      .kappa = pluss::series::full({1}, 1.7, cfg),
      .theta = pluss::series::full({1}, 0.04, cfg),
      .sigma = pluss::series::full({1}, 0.30, cfg),
      .rho = pluss::series::full({1}, -0.6, cfg),
      .v0 = pluss::series::full({1}, 0.04, cfg),
  };

  const auto bates = fin::model::Bates{
      .kappa = pluss::series::full({1}, 1.7, cfg),
      .theta = pluss::series::full({1}, 0.04, cfg),
      .sigma = pluss::series::full({1}, 0.30, cfg),
      .rho = pluss::series::full({1}, -0.6, cfg),
      .v0 = pluss::series::full({1}, 0.04, cfg),
      .jump_intensity = pluss::series::full({1}, 0.15, cfg),
      .jump_mean = pluss::series::full({1}, -0.08, cfg),
      .jump_vol = pluss::series::full({1}, 0.20, cfg),
  };

  const fin::pricer::Fft fft{
      .num_fft = 4096,
      .du = 0.01,
      .alpha = 1.5,
  };
  const fin::pricer::COS cos{
      .num_terms = 1024,
      .truncation = 12.0,
      .cumulant_step = 1e-3,
  };

  const auto h_fft = fft.price(inst, heston, mkt, fin::pricer::RiskSpec::All, cfg);
  const auto h_cos = cos.price(inst, heston, mkt, fin::pricer::RiskSpec::All, cfg);
  const auto b_fft = fft.price(inst, bates, mkt, fin::pricer::RiskSpec::All, cfg);
  const auto b_cos = cos.price(inst, bates, mkt, fin::pricer::RiskSpec::All, cfg);

  REQUIRE(all_finite(h_fft.price));
  REQUIRE(all_finite(h_cos.price));
  REQUIRE(all_finite(b_fft.price));
  REQUIRE(all_finite(b_cos.price));

  REQUIRE(all_finite(h_fft.greeks.delta));
  REQUIRE(all_finite(h_fft.greeks.gamma));
  REQUIRE(all_finite(h_fft.greeks.vega));
  REQUIRE(all_finite(h_fft.greeks.theta));
  REQUIRE(all_finite(h_fft.greeks.rho));

  REQUIRE(all_finite(h_cos.greeks.delta));
  REQUIRE(all_finite(h_cos.greeks.gamma));
  REQUIRE(all_finite(h_cos.greeks.vega));
  REQUIRE(all_finite(h_cos.greeks.theta));
  REQUIRE(all_finite(h_cos.greeks.rho));

  REQUIRE(all_finite(b_fft.greeks.delta));
  REQUIRE(all_finite(b_fft.greeks.gamma));
  REQUIRE(all_finite(b_cos.greeks.delta));
  REQUIRE(all_finite(b_cos.greeks.gamma));

  REQUIRE(max_abs_diff(h_fft.price, h_cos.price) < 0.75);
  REQUIRE(max_abs_diff(b_fft.price, b_cos.price) < 1.0);
}
