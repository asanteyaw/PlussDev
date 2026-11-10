<!-- generated from CredibilityInput.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `actuarial::CredibilityInput`

The three series a credibility operator blends: experience, manual rate and exposure.

## Signature

```cpp
actuarial::CredibilityInput::CredibilityInput(pluss::Series experience, pluss::Series manual, pluss::Series exposure);
pluss::Series actuarial::Credibility::operator()(const actuarial::CredibilityInput &input) const;
```

A cell with little data should lean on a rate from elsewhere. The blend is {math}`Z\,\text{experience}+(1-Z)\,\text{manual}` with credibility {math}`Z=\dfrac{\text{exposure}}{\text{exposure}+K}` and `K` the full-credibility standard bound into the [`Credibility`](Credibility.md) operator. The three series have the **same length**, one entry per cell.

## Fields

### `experience`

`pluss::Series`. The rate observed in each cell, finite and 0 or more. It gets weight `Z`.

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
  const actuarial::Credibility blend = actuarial::operators::credibility(100.0);
  const pluss::Series manual = pluss::make_series({0.65, 0.65, 0.65});
  const pluss::Series exposure = pluss::make_series({100.0, 100.0, 100.0});
  for (const double e : {0.50, 0.80, 1.10})
    out.writeln("experience", e, " blend", blend(actuarial::CredibilityInput{pluss::make_series({e, e, e}), manual, exposure}).items<double>()[0]);
}
```

Output

```text
experience 0.500000  blend 0.575000
experience 0.800000  blend 0.725000
experience 1.100000  blend 0.875000
```


### `manual`

`pluss::Series`. The complementary rate for each cell, finite and 0 or more, from a larger book or a filed rate. It gets weight `1 − Z`, so it is the answer where there is no exposure.

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
  const actuarial::Credibility blend = actuarial::operators::credibility(100.0);
  const pluss::Series experience = pluss::make_series({0.80, 0.80});
  for (const double m : {0.50, 0.65, 0.90})
    out.writeln("manual", m, " blend", blend(actuarial::CredibilityInput{experience, pluss::make_series({m, m}), pluss::make_series({0.0, 100.0})}).items<double>());
}
```

Output

```text
manual 0.500000  blend [0.500000, 0.650000]
manual 0.650000  blend [0.650000, 0.725000]
manual 0.900000  blend [0.900000, 0.850000]
```


### `exposure`

`pluss::Series`. The volume behind each cell's experience, finite and 0 or more. More exposure means higher `Z`; zero exposure returns the manual rate exactly, and as exposure grows the blend approaches the experience.

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
  const actuarial::Credibility blend = actuarial::operators::credibility(100.0);
  for (const double e : {0.0, 25.0, 100.0, 400.0, 10000.0}) {
    const auto r = blend(actuarial::CredibilityInput{pluss::make_series({0.80}), pluss::make_series({0.65}), pluss::make_series({e})});
    out.writeln("exposure", e, " Z", e / (e + 100.0), " blend", r.items<double>()[0]);
  }
}
```

Output

```text
exposure 0.000000  Z 0.000000  blend 0.650000
exposure 25.000000  Z 0.200000  blend 0.680000
exposure 100.000000  Z 0.500000  blend 0.725000
exposure 400.000000  Z 0.800000  blend 0.770000
exposure 10000.000000  Z 0.990099  blend 0.798515
```


## Practical Examples

### Example 4: three cells

Experience of 0.80 against a manual rate of 0.65 at three exposures, with K = 100.

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
  const actuarial::CredibilityInput input{pluss::make_series({0.80, 0.80, 0.80}), pluss::make_series({0.65, 0.65, 0.65}), pluss::make_series({0.0, 100.0, 10000.0})};
  out.writeln(actuarial::operators::credibility(100.0)(input).items<double>());
}
```

Output

```text
[0.650000, 0.725000, 0.798515]
```

### Example 5: by hand

Z is exposure / (exposure + K); the blend is the weighted average.

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
  const double z = 100.0 / (100.0 + 100.0);
  out.writeln(z * 0.80 + (1.0 - z) * 0.65);
  out.writeln(actuarial::operators::credibility(100.0)(actuarial::CredibilityInput{pluss::make_series({0.80}), pluss::make_series({0.65}), pluss::make_series({100.0})}).items<double>());
}
```

Output

```text
0.725000
[0.725000]
```

### Example 6: the errors

Unequal lengths, a negative exposure and undefined data are refused.

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
  report([&] { (void)actuarial::operators::credibility(100.0)(actuarial::CredibilityInput{pluss::make_series({0.8, 0.7}), pluss::make_series({0.6, 0.6, 0.6}), pluss::make_series({1.0})}); });
  report([&] { (void)actuarial::operators::credibility(100.0)(actuarial::CredibilityInput{pluss::make_series({0.8}), pluss::make_series({0.6}), pluss::make_series({-1.0})}); });
  report([&] { (void)actuarial::operators::credibility(100.0)(actuarial::CredibilityInput{pluss::make_series({-0.8}), pluss::make_series({0.6}), pluss::make_series({100.0})}); });
  report([&] { (void)actuarial::operators::credibility(100.0)(actuarial::CredibilityInput{}); });
}
```

Output

```text
credibility: experience, manual and exposure must have the same length
credibility_weight: exposure must be >= 0
credibility: experience must be >= 0
credibility: experience must be defined
```

## Changelog

- **0.1**: initial operator-first actuarial surface.

## See Also

[`Credibility`](Credibility.md) consumes it, and [`credibility`](credibility_operator.md) binds the standard.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="PricingResult.html">
    <span class="label">Previous</span>
    <span class="title">PricingResult</span>
  </a>
  <a class="next-link" href="Pricing.html">
    <span class="label">Next</span>
    <span class="title">Pricing</span>
  </a>
</div>
<!-- apiref:nav:end -->
