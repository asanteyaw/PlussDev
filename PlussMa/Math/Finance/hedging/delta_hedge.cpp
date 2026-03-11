#include "pluss/subsp/math/finance/hedging.h"

#include "CodeM/core/Errors.h"
#include "pluss/codem/series_factory.h"

#include <cmath>
#include <string>
#include <vector>

namespace pluss::subsp::math::finance::hedging {
namespace {

using pluss::Config;
using pluss::ScalarType;
using pluss::Series;

Config f64_cfg_(Config cfg) {
  return Config{}.with_dtype(ScalarType::Float64).with_platform(cfg.platform).with_pinned(cfg.pinned);
}

Series as_vector_(const Series& x, const char* op) {
  PLUSS_CHECK(x.defined(), std::string("subsp::math::finance::hedging::") + op + ": expected defined Series");
  const auto sizes = x.sizes();
  PLUSS_CHECK(!sizes.empty(), std::string("subsp::math::finance::hedging::") + op + ": expected non-scalar Series");
  if (sizes.size() == 1) return x.reshape({sizes[0]});
  PLUSS_CHECK(sizes.size() == 2 && sizes[1] == 1,
              std::string("subsp::math::finance::hedging::") + op + ": expected vector or Nx1 matrix");
  return x.reshape({sizes[0]});
}

Series scalar_index_(std::int64_t index, Config cfg) {
  const auto idx_cfg = Config{}.with_dtype(ScalarType::Int64).with_platform(cfg.platform).with_pinned(cfg.pinned);
  return pluss::make_series(std::vector<std::int64_t>{index}, {1}, idx_cfg);
}

} // namespace

DeltaHedgeResult delta_hedge(const Series& spot_path,
                             pricing::OptionType type,
                             double strike,
                             double rate,
                             double dividend_yield,
                             double maturity,
                             double volatility,
                             Config cfg) {
  const auto out_cfg = f64_cfg_(cfg);
  const Series spot = as_vector_(spot_path, "delta_hedge").to(out_cfg.platform, out_cfg.dtype);
  const std::int64_t n = spot.numel();

  PLUSS_CHECK(n >= 2, "subsp::math::finance::hedging::delta_hedge: spot_path requires at least 2 observations");
  PLUSS_CHECK(spot.isfinite().all().item<double>() == 1.0,
              "subsp::math::finance::hedging::delta_hedge: spot_path contains non-finite values");
  PLUSS_CHECK(spot.gt(0.0).all().item<double>() == 1.0,
              "subsp::math::finance::hedging::delta_hedge: spot_path values must be > 0");
  PLUSS_CHECK(std::isfinite(maturity) && maturity > 0.0,
              "subsp::math::finance::hedging::delta_hedge: maturity must be finite and > 0");
  PLUSS_CHECK(std::isfinite(rate), "subsp::math::finance::hedging::delta_hedge: rate must be finite");

  const double dt = maturity / static_cast<double>(n - 1);
  const double growth = std::exp(rate * dt);
  const Series tau = pluss::full({n}, maturity, out_cfg)
                         .sub(pluss::linspace(0.0, maturity, n, out_cfg))
                         .clamp_min(0.0);

  const Series option_value =
      pricing::black_scholes_price(type, spot, tau, strike, rate, dividend_yield, volatility, out_cfg);
  const Series delta = pricing::black_scholes_greeks(type, spot, tau, strike, rate, dividend_yield, volatility, out_cfg)
                           .delta;

  Series cash = pluss::zeros({n}, out_cfg);
  const Series idx0 = scalar_index_(0, out_cfg);
  const Series cash0 = option_value.select(0, 0).sub(delta.select(0, 0).mul(spot.select(0, 0)));
  cash = pluss::index_put(cash, std::vector<Series>{idx0}, cash0.reshape({1}), false);

  for (std::int64_t t = 1; t < n; ++t) {
    const Series b_prev = cash.select(0, t - 1);
    const Series delta_prev = delta.select(0, t - 1);
    const Series delta_cur = delta.select(0, t);
    const Series spot_cur = spot.select(0, t);
    const Series b_cur = b_prev.mul(growth).sub(delta_cur.sub(delta_prev).mul(spot_cur));
    const Series idx = scalar_index_(t, out_cfg);
    cash = pluss::index_put(cash, std::vector<Series>{idx}, b_cur.reshape({1}), false);
  }

  const Series portfolio = delta.mul(spot).add(cash);
  const Series hedge_error = portfolio.sub(option_value);
  const double terminal_pnl = hedge_error.select(0, n - 1).item<double>();

  PLUSS_CHECK(option_value.isfinite().all().item<double>() == 1.0,
              "subsp::math::finance::hedging::delta_hedge: option_value contains non-finite values");
  PLUSS_CHECK(delta.isfinite().all().item<double>() == 1.0,
              "subsp::math::finance::hedging::delta_hedge: delta contains non-finite values");
  PLUSS_CHECK(cash.isfinite().all().item<double>() == 1.0,
              "subsp::math::finance::hedging::delta_hedge: cash recursion produced non-finite values");
  PLUSS_CHECK(portfolio.isfinite().all().item<double>() == 1.0,
              "subsp::math::finance::hedging::delta_hedge: portfolio contains non-finite values");
  PLUSS_CHECK(hedge_error.isfinite().all().item<double>() == 1.0,
              "subsp::math::finance::hedging::delta_hedge: hedge_error contains non-finite values");

  return DeltaHedgeResult{
      .option_value = option_value.to(out_cfg.platform, out_cfg.dtype),
      .delta = delta.to(out_cfg.platform, out_cfg.dtype),
      .cash = cash.to(out_cfg.platform, out_cfg.dtype),
      .portfolio = portfolio.to(out_cfg.platform, out_cfg.dtype),
      .hedge_error = hedge_error.to(out_cfg.platform, out_cfg.dtype),
      .terminal_pnl = terminal_pnl,
  };
}

} // namespace pluss::subsp::math::finance::hedging
