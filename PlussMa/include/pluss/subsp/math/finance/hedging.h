#pragma once

#include "pluss/subsp/math/finance/pricing.h"

namespace pluss::subsp::math::finance::hedging {

struct DeltaHedgeResult {
  pluss::codem::Series option_value = pluss::codem::Series::undefined();
  pluss::codem::Series delta = pluss::codem::Series::undefined();
  pluss::codem::Series cash = pluss::codem::Series::undefined();
  pluss::codem::Series portfolio = pluss::codem::Series::undefined();
  pluss::codem::Series hedge_error = pluss::codem::Series::undefined();
  double terminal_pnl = 0.0;
};

CODEM_API DeltaHedgeResult delta_hedge(const pluss::codem::Series& spot_path,
                                       pricing::OptionType type,
                                       double strike,
                                       double rate,
                                       double dividend_yield,
                                       double maturity,
                                       double volatility,
                                       pluss::codem::Config cfg = {});

} // namespace pluss::subsp::math::finance::hedging

namespace pluss::subsp::math::finance {
namespace hdg = hedging;
}
