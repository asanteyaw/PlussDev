#pragma once

#include "pluss/subsp/math/finance/module.h"
#include "pluss/subsp/math/finance/pricing.h"

namespace pluss::subsp::math::finance::hedging {

struct DeltaHedgeResult {
  pluss::codem::Series option_value = pluss::series::undefined();
  pluss::codem::Series delta = pluss::series::undefined();
  pluss::codem::Series cash = pluss::series::undefined();
  pluss::codem::Series portfolio = pluss::series::undefined();
  pluss::codem::Series hedge_error = pluss::series::undefined();
  double terminal_pnl = 0.0;
};

// Delta hedge of a European option in a field: the option is marked and its delta read from the field's own model and engine at every
// date of the spot path (analytic where the model has a closed form, bump-and-reprice otherwise); the path spans the field's maturity.
CODEM_API DeltaHedgeResult delta_hedge(const module::Field& field, const pluss::codem::Series& spot_path, pluss::codem::Config cfg = {});

namespace detail {

// Implementation layer: the Black-Scholes hedge behind the field-based one. Not a public interface.
CODEM_API DeltaHedgeResult delta_hedge(const pluss::codem::Series& spot_path,
                                       pricing::OptionType type,
                                       double strike,
                                       double rate,
                                       double dividend_yield,
                                       double maturity,
                                       double volatility,
                                       pluss::codem::Config cfg = {});

} // namespace detail

} // namespace pluss::subsp::math::finance::hedging

namespace pluss::subsp::math::finance {
namespace hdg = hedging;
}
