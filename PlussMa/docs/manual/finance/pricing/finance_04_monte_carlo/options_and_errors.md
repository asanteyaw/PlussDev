<!-- generated from options_and_errors.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Options and errors

[Back to Monte Carlo](../finance_04_monte_carlo.md)

## Every option

| `MonteCarloOptions` | Meaning | Default | Requirement |
|---|---|---|---|
| `paths` | the number of simulated paths | 40000 | positive |
| `steps` | the number of time steps per path | 252 | positive |
| `scheme` | the variance scheme ([Simulated paths](simulated_paths.md)) | `Euler` | |
| `rand_method` | `Pseudo`, `Sobol`, `Halton`, `Torus` ([Quasi-random sequences](quasi_random_sequences.md)) | `Pseudo` | |
| `randomized_quasi` | randomise the quasi-random sequence | false | |
| `sobol_scrambling` | the Sobol scrambling variant | 0 | 0 to 3 |
| `antithetic` | antithetic pairs ([Variance reduction](variance_reduction.md)) | false | |
| `control_variate` | the Black-Scholes control | false | |
| `control_vol` | the control's volatility; 0 means the long-run volatility of the model | 0 | finite and non-negative |
| `importance_sampling` | shift the spot shocks and reweight | false | |
| `importance_shift` | the per-step shift of the spot shocks | 0 | finite |
| `seed` | the random stream | 0 | |
| `offset` | where a quasi-random sequence starts | 0 | |

The estimate of one run is a single number; the number of paths and steps should be chosen against the error bar found by repeating with different seeds ([Pricing by simulation](pricing_by_simulation.md)). One path is allowed and returns that path's discounted payoff.

## Conditions and messages

```cpp
#include <pluss/ma.h>
#include <pluss/tils/auxiliary/output.h>

#include <limits>

namespace prc = pluss::subsp::math::finance::pricing;
namespace vol = pluss::subsp::math::finance::volatility;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const vol::HestonParams params;
  auto reason = [](const std::exception &e) {
    const std::string message = e.what();
    const auto start = message.find("_price_mc: ") + 11;
    return message.substr(start, message.find('\n') - start);
  };
  auto attempt = [&](const char *label, auto edit) {
    prc::MonteCarloOptions options;
    options.paths = 2000;
    options.steps = 16;
    edit(options);
    try {
      out.writeln(label, "-> price", prc::heston_price_mc(prc::Call, 100.0, 100.0, 0.03, 0.01, 1.0, params, options));
    } catch (const std::exception &e) {
      out.writeln(label, "->", reason(e));
    }
  };
  attempt("paths = 0              ", [](prc::MonteCarloOptions &o) { o.paths = 0; });
  attempt("steps = 0              ", [](prc::MonteCarloOptions &o) { o.steps = 0; });
  attempt("paths = 1              ", [](prc::MonteCarloOptions &o) { o.paths = 1; });
  attempt("importance_shift = NaN ", [](prc::MonteCarloOptions &o) { o.importance_sampling = true; o.importance_shift = std::numeric_limits<double>::quiet_NaN(); });
  attempt("control_vol = -1       ", [](prc::MonteCarloOptions &o) { o.control_variate = true; o.control_vol = -1.0; });
  attempt("sobol_scrambling = 7   ", [](prc::MonteCarloOptions &o) { o.rand_method = vol::RandMethod::Sobol; o.randomized_quasi = true; o.sobol_scrambling = 7; });
  out.writeln("maturity 0 (intrinsic value) ->", prc::heston_price_mc(prc::Call, 110.0, 100.0, 0.03, 0.01, 0.0, params));
  try {
    prc::heston_price_mc(prc::OptionType::Auto, 100.0, 100.0, 0.03, 0.01, 1.0, params);
  } catch (const std::exception &e) {
    out.writeln("OptionType::Auto ->", reason(e));
  }
}
```

Output

```text
paths = 0               -> monte-carlo paths must be > 0
steps = 0               -> monte-carlo steps must be > 0
paths = 1               -> price 0.0000
importance_shift = NaN  -> importance_shift must be finite
control_vol = -1        -> control_vol must be finite and >= 0
sobol_scrambling = 7    -> sobol_scrambling must be in [0,3]
maturity 0 (intrinsic value) -> 10.0000
OptionType::Auto -> option type must be Call or Put (OptionType::Auto is only valid for an instrument that supplies is_call)
```

Each invalid setting is rejected with the message shown. One path is a valid run, and it prices the option at that path's discounted payoff (0 here, as the single path ends below the strike). A zero maturity returns the intrinsic value, and `OptionType::Auto` is not valid for a pricer that takes no instrument.
