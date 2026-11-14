<!-- generated from pure_premium_loss_ratio_and_claim_cost.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Pure Premium, Loss Ratio and Claim Cost

[Back to Ratemaking, Credibility and Loss Reserving](../actuarial_04_ratemaking_credibility_and_loss_reserving.md)

Ratemaking asks what a unit of insurance should cost. Two measures answer from experience. The **pure premium** is the loss per unit of **exposure**, the amount of risk insured (car-years, payroll, policies): it is the cost the premium has to cover before expenses. The **loss ratio** is losses over premium, which says how much of the premium the losses consumed.

```{math}
\text{pure premium}=\frac{\sum_t L_t}{\sum_t E_t},\qquad
\text{loss ratio}=\frac{\sum_t L_t}{\sum_t P_t},
```

where {math}`L_t`, {math}`E_t` and {math}`P_t` are the incurred losses, the exposure and the earned premium of period {math}`t`. Both are ratios of sums, not averages of the yearly ratios: a year with more exposure counts for more. The yearly ratios are useful as curves, to see a trend, and have their own functions.

When the cost is built up from its causes instead, the expected cost of a period is the exposure times the claim frequency (claims per unit of exposure) times the severity (the average claim): {math}`E\cdot f\cdot s`.

```cpp
pluss::Series models::pure_premium(const pluss::Series &incurred_losses, const pluss::Series &exposure);
pluss::Series models::loss_ratio(const pluss::Series &incurred_losses, const pluss::Series &earned_premium);
pluss::Series models::premium_curve(const pluss::Series &incurred_losses, const pluss::Series &exposure);
pluss::Series models::ratio_curve(const pluss::Series &incurred_losses, const pluss::Series &earned_premium);
pluss::Series models::claim_cost(const pluss::Series &exposure, const pluss::Series &claim_frequency, const pluss::Series &claim_severity);
```

| Function | Returns | Notes |
|---|---|---|
| `pure_premium(losses, exposure)` | a rank-zero value: the total loss over the total exposure | the total exposure must be positive |
| `loss_ratio(losses, premium)` | a rank-zero value: the total loss over the total premium | the total premium must be positive |
| `premium_curve(losses, exposure)` | the loss over the exposure period by period | zero where the exposure is zero |
| `ratio_curve(losses, premium)` | the loss over the premium period by period | zero where the premium is zero |
| `claim_cost(exposure, frequency, severity)` | exposure × frequency × severity, element by element | |

All arguments are one-dimensional `Series` of one length, with finite values.

## Four years of experience

A book has four years of exposure, incurred losses and earned premium. The program prints the aggregate pure premium and loss ratio, and the yearly curves, which show a pure premium between 520 and 611 and a loss ratio between 74% and 89%.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace actuarial = pluss::ma::actuarial;
namespace life = actuarial::life_contingencies;
namespace losses = actuarial::losses;
namespace models = actuarial::models;
namespace distr = pluss::subsp::stats::distr;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6));
  const pluss::Series exposure = pluss::make_series({1000.0, 1200.0, 1500.0, 1800.0});
  const pluss::Series losses = pluss::make_series({520000.0, 690000.0, 810000.0, 1100000.0});
  const pluss::Series premium = pluss::make_series({700000.0, 820000.0, 1000000.0, 1230000.0});
  out.writeln("pure premium  =", models::pure_premium(losses, exposure).item<double>(), " by hand", 3120000.0 / 5500.0);
  out.writeln("loss ratio    =", models::loss_ratio(losses, premium).item<double>(), " by hand", 3120000.0 / 3750000.0);
  out.writeln("premium_curve =", models::premium_curve(losses, exposure).items<double>());
  out.writeln("ratio_curve   =", models::ratio_curve(losses, premium).items<double>());
}
```

Output

```text
pure premium  = 567.272727  by hand 567.272727
loss ratio    = 0.832000  by hand 0.832000
premium_curve = [520.000000, 575.000000, 540.000000, 611.111111]
ratio_curve   = [0.742857, 0.841463, 0.810000, 0.894309]
```

## The aggregate is not the average of the years

The aggregate pure premium weights each year by its exposure, and the simple average of the yearly values does not. The two differ when exposure changes, which is the usual case for a growing book.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace actuarial = pluss::ma::actuarial;
namespace life = actuarial::life_contingencies;
namespace losses = actuarial::losses;
namespace models = actuarial::models;
namespace distr = pluss::subsp::stats::distr;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6));
  const pluss::Series exposure = pluss::make_series({1000.0, 1200.0, 1500.0, 1800.0});
  const pluss::Series losses = pluss::make_series({520000.0, 690000.0, 810000.0, 1100000.0});
  out.writeln("aggregate pure premium      =", models::pure_premium(losses, exposure).item<double>());
  out.writeln("average of the yearly curve =", models::premium_curve(losses, exposure).mean().item<double>());
}
```

Output

```text
aggregate pure premium      = 567.272727
average of the yearly curve = 561.527778
```

## Building the cost from frequency and severity

If each year's claim frequency and average severity are known, `claim_cost` gives the expected total cost: the exposure times the frequency times the severity. The program builds the cost from a frequency of 6% to 8% claims per exposure unit and severities around 9,000.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace actuarial = pluss::ma::actuarial;
namespace life = actuarial::life_contingencies;
namespace losses = actuarial::losses;
namespace models = actuarial::models;
namespace distr = pluss::subsp::stats::distr;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6));
  const pluss::Series exposure = pluss::make_series({1000.0, 1200.0, 1500.0, 1800.0});
  const pluss::Series frequency = pluss::make_series({0.06, 0.06, 0.07, 0.08});
  const pluss::Series severity = pluss::make_series({8000.0, 9500.0, 9000.0, 9200.0});
  const pluss::Series cost = models::claim_cost(exposure, frequency, severity);
  out.writeln("expected claim cost =", cost.items<double>());
  out.writeln("per exposure unit   =", cost.div(exposure).items<double>());
  out.writeln("frequency * severity=", frequency.mul(severity).items<double>());
}
```

Output

```text
expected claim cost = [480000.000000, 684000.000000, 945000.000000, 1324800.000000]
per exposure unit   = [480.000000, 570.000000, 630.000000, 736.000000]
frequency * severity= [480.000000, 570.000000, 630.000000, 736.000000]
```

## Periods with no exposure

An empty period must not divide by zero. The curves return zero where the exposure or premium is zero, and the aggregate functions require the total to be positive.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace actuarial = pluss::ma::actuarial;
namespace life = actuarial::life_contingencies;
namespace losses = actuarial::losses;
namespace models = actuarial::models;
namespace distr = pluss::subsp::stats::distr;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6));
  const auto report = [&](const auto &call) {
    try {
      call();
    } catch (const std::exception &e) {
      std::string text(e.what());
      if (const auto cut = text.find("Triggered"); cut != std::string::npos) text.erase(cut);
      while (!text.empty() && text.back() == '\n') text.pop_back();
      out.writeln(text);
    }
  };
  const pluss::Series losses = pluss::make_series({100.0, 200.0});
  out.writeln("premium_curve with a zero exposure =", models::premium_curve(losses, pluss::make_series({0.0, 50.0})).items<double>());
  out.writeln("ratio_curve with a zero premium    =", models::ratio_curve(losses, pluss::make_series({40.0, 0.0})).items<double>());
  report([&] { (void)models::pure_premium(losses, pluss::make_series({0.0, 0.0})); });
  report([&] { (void)models::premium_curve(losses, pluss::make_series({1.0, 2.0, 3.0})); });
}
```

Output

```text
premium_curve with a zero exposure = [0.000000, 4.000000]
ratio_curve with a zero premium    = [2.500000, 0.000000]
pure_premium: aggregate exposure must be > 0
premium_curve: numerator and exposure must have same length
```

## Where to go next

The pure premium is the starting point of the rate: loads for expenses and profit turn it into an indicated premium ([Indicated Premium and Rate Change](indicated_premium_and_rate_change.md)), and a pure premium from a small book is blended with a manual rate according to its credibility ([Credibility](credibility.md)).

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="next-link" href="indicated_premium_and_rate_change.html">
    <span class="label">Next</span>
    <span class="title">Indicated Premium and Rate Change</span>
  </a>
</div>
<!-- apiref:nav:end -->
