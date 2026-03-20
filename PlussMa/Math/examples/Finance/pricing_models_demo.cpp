#include <pluss/ma.h>

#include <iostream>

int main() {
  namespace fin = pluss::subsp::math::finance;

  const auto cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  auto inst = fin::instrument::make(fin::pricing::OptionType::Call, 100.0, 1.0, cfg);
  inst.strike = pluss::full({6}, 100.0, cfg);
  inst.maturity = pluss::full({6}, 1.0, cfg);

  const auto hest = fin::model::make(1.5, 0.04, 0.30, -0.70, 0.04, cfg);

  auto mkt = fin::market::make(100.0, 0.03, 0.01, cfg);
  mkt.spot = pluss::full({6}, 100.0, cfg);
  mkt.rate = pluss::full({6}, 0.03, cfg);
  mkt.dividend_yield = pluss::full({6}, 0.01, cfg);

  const auto fft = fin::pricer::Fft{
      .num_fft = 4096,
      .du = 0.01,
      .alpha = 1.5,
  };
  const auto heston_px = fft.price(inst, hest, mkt, fin::pricer::RiskSpec::PriceOnly, cfg).price;
  std::cout << "heston_fft_price=" << heston_px << "\n";
  const auto cos = fin::pricer::COS{
      .num_terms = 1024,
      .truncation = 12.0,
      .cumulant_step = 1e-3,
  };
  const auto heston_cos = cos.price(inst, hest, mkt, fin::pricer::RiskSpec::All, cfg);
  std::cout << "heston_cos_price=" << heston_cos.price << "\n";
  std::cout << "heston_cos_delta=" << heston_cos.greeks.delta << "\n";

  const auto bates = fin::model::make(1.5, 0.04, 0.30, -0.70, 0.04, 0.21, -0.03, 0.01, cfg);
  auto res = fft.price(inst, bates, mkt, fin::pricer::RiskSpec::All);
  std::cout << "price=" << res.price << "\n";
  std::cout << "delta=" << res.greeks.delta << "\n";
  const auto bates_cos = cos.price(inst, bates, mkt, fin::pricer::RiskSpec::All, cfg);
  std::cout << "bates_cos_price=" << bates_cos.price << "\n";
  std::cout << "bates_cos_delta=" << bates_cos.greeks.delta << "\n";

  const auto sabr = fin::model::make(
      0.20,
      0.50,
      0.40,
      -0.30,
      fin::pricing::SabrVolType::Lognormal,
      fin::pricing::SabrApproxOptions{
          .model = fin::pricing::SabrApproxModel::Hagan2002,
          .shift = 0.0,
      },
      cfg);
  fin::pricer::Analytic an;
  const auto sabr_px_approx = an.price(inst, sabr, mkt, fin::pricer::RiskSpec::PriceOnly, cfg).price;
  std::cout << "sabr_approx_price=" << sabr_px_approx << "\n";

  fin::pricer::MonteCarlo mc;
  mc.paths = 20000;
  mc.steps = 252;
  mc.antithetic = true;
  mc.seed = 42;
  const auto sabr_px_mc = mc.price(inst, sabr, mkt, fin::pricer::RiskSpec::PriceOnly, cfg).price;
  std::cout << "sabr_mc_price=" << sabr_px_mc << "\n";
  return 0;
}
