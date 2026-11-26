# Fitting counts

[Back to Biology 23: Census Data](../bio_23_census.md)

Population counts are the most common data in conservation, and they are noisy twice over: the population itself fluctuates from year to year, and each count measures it with error. You want the trend and the risk of falling below a threshold, without mistaking one source of noise for the other. A state-space model separates them. This page fits such a model to annual counts, computes extinction risk from it, and shows how weakly short series constrain the long-term drift.

## How it works

The code is in `pluss::ma::natsci::biology::population::count_pva` (`pva::` below). The log abundance {math}`x_t = \ln N_t` follows a random walk with drift {math}`\mu` and process variance {math}`\sigma^2`, and each count {math}`y_t` is observed with error variance {math}`\eta^2`:

```{math}
\begin{aligned}
&x_{t+1} = x_t + \mu + \sigma\,\varepsilon_t, \\
&y_t = x_t + \eta\,\zeta_t .
\end{aligned}
```

`pva::fit(counts)` finds the three parameters by maximum likelihood with a Kalman filter and returns the log likelihood, the filtered current log abundance `filtered_mean`, and the smoothed log abundance. From a fit, `pva::extinction_risk(fit, threshold, horizons)` is the probability of falling below a threshold within each horizon, `pva::ultimate_extinction_risk(fit, threshold)` the long-run probability, and `pva::loglik(counts, parameters)` evaluates the likelihood at any parameters. A threshold above the current abundance has risk one.

## Fitting the model

The program fits twenty-five annual counts of a bird population and prints the three parameters and the current abundance.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <algorithm>
#include <cmath>

using namespace pluss::tils;
namespace biology = pluss::ma::natsci::biology;
namespace dynamics = biology::dynamics;
namespace population = biology::population;
static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

namespace pva = population::count_pva;
int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  // twenty-five annual counts of a bird population
  const auto counts = vec({200, 198, 215, 213, 209, 191, 190, 221, 238, 275, 289, 309, 322, 269, 304, 330, 357, 298, 246, 226, 218, 230, 234, 254, 240});
  const auto fit = pva::fit(counts);
  out.writeln("drift of ln N per year        mu     = ", fit.parameters.mu, "");
  out.writeln("process variance              sigma2 = ", fit.parameters.sigma2);
  out.writeln("observation-error variance    eta2   = ", fit.parameters.eta2);
  out.writeln("log likelihood = ", fit.loglik, ", converged = ", fit.converged ? "yes" : "no");
  out.writeln("estimated current abundance: exp(", fit.filtered_mean, ") = ", std::exp(fit.filtered_mean));
}
```

Output

```text
drift of ln N per year        mu     = 0.0076
process variance              sigma2 = 0.0093
observation-error variance    eta2   = 0.0000
log likelihood = 22.0800, converged = yes
estimated current abundance: exp(5.4806) = 240.0000
```

The fit finds a small positive drift, `0.0076` per year, a process variance of `0.0093` and no observation error (`0.0000`), so the filtered abundance is exactly the last count, `240.0000`.

## Extinction risk

The program asks for four thresholds at horizons of `10`, `25`, `50` and `100` years, and for the ultimate risk.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <algorithm>
#include <cmath>

using namespace pluss::tils;
namespace biology = pluss::ma::natsci::biology;
namespace dynamics = biology::dynamics;
namespace population = biology::population;
static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

namespace pva = population::count_pva;
int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  const auto counts = vec({200, 198, 215, 213, 209, 191, 190, 221, 238, 275, 289, 309, 322, 269, 304, 330, 357, 298, 246, 226, 218, 230, 234, 254, 240});
  const auto fit = pva::fit(counts);
  // risk of falling below 150 or 200 within 10, 25, 50 and 100 years, and in the long run
  const auto horizons = vec({10.0, 25.0, 50.0, 100.0});
  for (double threshold : {150.0, 200.0, 230.0, 260.0})
    out.writeln("threshold ", threshold, ": risk by 10, 25, 50, 100 years = ", flat(pva::extinction_risk(fit, threshold, horizons)), ", ultimate risk ", pva::ultimate_extinction_risk(fit, threshold));
}
```

Output

```text
threshold 150.0000: risk by 10, 25, 50, 100 years = [0.0822, 0.2158, 0.3139, 0.3879], ultimate risk 0.4640
threshold 200.0000: risk by 10, 25, 50, 100 years = [0.4684, 0.5959, 0.6606, 0.7028], ultimate risk 0.7424
threshold 230.0000: risk by 10, 25, 50, 100 years = [0.8559, 0.8932, 0.9111, 0.9224], ultimate risk 0.9328
threshold 260.0000: risk by 10, 25, 50, 100 years = [1.0000, 1.0000, 1.0000, 1.0000], ultimate risk 1.0000
```

For the threshold `150` the risk is `0.0822` within `10` years and `0.3879` within `100`, with an ultimate risk of `0.4640`. The threshold `200` is only `20%` below the current count, so its risk is already `0.4684` in `10` years; `230` gives `0.8559`, and `260`, above the current count, gives `1.0000`.

## How well is the drift known?

The drift is the hardest parameter to estimate from a short series. The program evaluates the log likelihood at five drifts with the other parameters held at their estimates, and prints the smoothed abundance at both ends of the series.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <algorithm>
#include <cmath>

using namespace pluss::tils;
namespace biology = pluss::ma::natsci::biology;
namespace dynamics = biology::dynamics;
namespace population = biology::population;
static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

namespace pva = population::count_pva;
int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(3).with_sep(""));
  const auto counts = vec({200, 198, 215, 213, 209, 191, 190, 221, 238, 275, 289, 309, 322, 269, 304, 330, 357, 298, 246, 226, 218, 230, 234, 254, 240});
  const auto fit = pva::fit(counts);
  // the likelihood as the drift varies, with the other two parameters held at their estimates: the drift is only weakly determined
  for (double mu : {-0.02, 0.0, 0.01, 0.02, 0.04}) {
    pva::Parameters q = fit.parameters; q.mu = mu;
    out.writeln("mu = ", mu, ": log likelihood ", pva::loglik(counts, q), " (best ", fit.loglik, ")");
  }
  // the smoothed abundance, which removes the observation error, at the first and last five census times
  const auto smooth = flat(fit.smoothed_mean);
  std::vector<double> first, last;
  for (int i = 0; i < 5; ++i) first.push_back(std::exp(smooth[i]));
  for (std::size_t i = smooth.size() - 5; i < smooth.size(); ++i) last.push_back(std::exp(smooth[i]));
  out.writeln("smoothed abundance, first five after the first census: ", first);
  out.writeln("smoothed abundance, last five: ", last);
}
```

Output

```text
mu = -0.020: log likelihood 21.097 (best 22.080)
mu = 0.000: log likelihood 22.006 (best 22.080)
mu = 0.010: log likelihood 22.073 (best 22.080)
mu = 0.020: log likelihood 21.882 (best 22.080)
mu = 0.040: log likelihood 20.725 (best 22.080)
smoothed abundance, first five after the first census: [198.000, 215.000, 213.000, 209.000, 191.000]
smoothed abundance, last five: [218.000, 230.000, 234.000, 254.000, 240.000]
```

The log likelihood changes by under `1.4` between drifts of `-0.020` and `0.040` (`21.097` and `20.725` against `22.080` at the best fit), so the data cannot tell a slow decline from a slow growth, and the risks above rest on a drift they hardly determine. With no observation error the smoothed abundance equals the counts.

## Decisions

- Use `ultimate_extinction_risk` for a long-run statement and `extinction_risk` with horizons for a planning period; report the risk with the likelihood profile over the drift, not as a single number.
- Pass counts, not their logarithms, and mark a missing census with a NaN rather than dropping it, so that the gap between censuses is kept.
- Collect more years before you trust a trend: ten counts are hardly enough to fix a drift.

## Pitfalls

The fit needs at least four positive counts, and it expects counts rather than logs. The program tries too few counts, a zero count and the logarithms of the counts, and then compares a ten-year fit with the full one.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <algorithm>
#include <cmath>

using namespace pluss::tils;
namespace biology = pluss::ma::natsci::biology;
namespace dynamics = biology::dynamics;
namespace population = biology::population;
static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

namespace pva = population::count_pva;
int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  try { (void)pva::fit(vec({200, 198})); } catch (const pluss::Error &e) { out.writeln("two counts:  ", e.message()); }
  try { (void)pva::fit(vec({200, 0, 215, 213, 209, 191})); } catch (const pluss::Error &e) { out.writeln("a zero count: ", e.message()); }
  // counts are not logs: the model works with ln N itself, so give it the counts, not their logarithms
  const auto counts = vec({200, 198, 215, 213, 209, 191, 190, 221, 238, 275, 289, 309, 322, 269, 304, 330, 357, 298, 246, 226, 218, 230, 234, 254, 240});
  const auto fit = pva::fit(counts);
  out.writeln("estimated current abundance from the counts:    ", std::exp(fit.filtered_mean));
  const auto bad = pva::fit(counts.log());
  out.writeln("estimated current abundance from log counts:    ", std::exp(bad.filtered_mean), ", converged = ", bad.converged ? "yes" : "no");
  // a short series, with a likelihood that is flat in the drift: ten counts
  const auto short_fit = pva::fit(vec({200, 198, 215, 213, 209, 191, 190, 221, 238, 275}));
  out.writeln("ten counts: drift ", short_fit.parameters.mu, ", ultimate risk of falling below 100 ", pva::ultimate_extinction_risk(short_fit, 100.0));
  out.writeln("25 counts:  drift ", fit.parameters.mu, ", ultimate risk of falling below 100 ", pva::ultimate_extinction_risk(fit, 100.0));
}
```

Output

```text
two counts:  natsci: count_pva: need at least four censuses to identify mu, sigma^2 and eta^2
a zero count: natsci: count_pva: counts must be positive (NaN marks a missing census)
estimated current abundance from the counts:    240.0000
estimated current abundance from log counts:    5.4806, converged = yes
ten counts: drift 0.0354, ultimate risk of falling below 100 0.0000
25 counts:  drift 0.0076, ultimate risk of falling below 100 0.2392
```

The first two are rejected with the rule. The third is the quiet one: given logarithms, the fit converges and reports a current abundance of `5.4806`, the logarithm of the real `240`. The last lines show how much a short series can mislead: with ten counts the drift looks like `0.0354` and the ultimate risk of falling below `100` is `0.0000`, while the full series gives a drift of `0.0076` and a risk of `0.2392`.

## Where next

[Biology 3: Demography and Extinction](../bio_03_demography.md) gives the models behind these risks, and [Biology 22: Size-Structured Populations](../bio_22_size.md) the structured models you can compare them with.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
</div>
<!-- apiref:nav:end -->
