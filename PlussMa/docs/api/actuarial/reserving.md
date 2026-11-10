<!-- generated from reserving.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Reserving

The reserving family estimates the claims still to come from a triangle of cumulative claims. A [`Triangle`](reserving/Triangle.md) holds the data and its observation mask; an estimator, a [`Reserving`](reserving/Reserving.md) operator, turns it into a [`ReserveResult`](reserving/ReserveResult.md) with the development factors, the ultimate claims of each origin period and the reserves. Both estimators have the same type, so comparing methods on one triangle is a loop over operators.

## What This Family Owns

| You want | Use |
|---|---|
| To put claims data into the form the estimators need | [`triangle::cumulative`](reserving/cumulative.md), [`Triangle`](reserving/Triangle.md) |
| To project reported claims with the development pattern | [`operators::chain_ladder`](reserving/chain_ladder.md) |
| To combine the reported claims with a prior expectation | [`operators::bornhuetter_ferguson`](reserving/bornhuetter_ferguson.md) |
| The factors, ultimates and reserves | [`ReserveResult`](reserving/ReserveResult.md) |
| The common type of the estimators | [`Reserving`](reserving/Reserving.md) |

Chain ladder lets the data speak: it multiplies each origin's latest amount by the factors the older origins went through, which suits mature origins and overreacts on young ones. Bornhuetter-Ferguson adds to the latest amount a share of an a priori ultimate and so barely reacts to the latest diagonal, which suits immature origins where the data is thin.

## Practical Examples

### Example 1: a triangle and a chain-ladder reserve

The factors, ultimates and reserves of a four-year triangle.

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
  out.print_defaults(PrintOptions{}.fixed(3));
  const pluss::Series cumulative = pluss::make_series({100.0, 180.0, 220.0, 240.0,
                                                       110.0, 200.0, 250.0, 0.0,
                                                       120.0, 215.0, 0.0, 0.0,
                                                       130.0, 0.0, 0.0, 0.0}, {4, 4});
  const actuarial::Triangle triangle = actuarial::triangle::cumulative(cumulative);
  const actuarial::ReserveResult r = actuarial::operators::chain_ladder()(triangle);
  out.writeln("age to age =", r.age_to_age.items<double>());
  out.writeln("ultimate   =", r.ultimate.items<double>());
  out.writeln("reserve    =", r.reserve.items<double>());
}
```

Output

```text
age to age = [1.803, 1.237, 1.091]
ultimate   = [240.000, 272.727, 290.096, 316.264]
reserve    = [0.000, 22.727, 75.096, 186.264]
```

### Example 2: two methods side by side

The same triangle through both estimators.

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
  out.print_defaults(PrintOptions{}.fixed(2));
  const pluss::Series cumulative = pluss::make_series({100.0, 180.0, 220.0, 240.0,
                                                       110.0, 200.0, 250.0, 0.0,
                                                       120.0, 215.0, 0.0, 0.0,
                                                       130.0, 0.0, 0.0, 0.0}, {4, 4});
  const actuarial::Triangle triangle = actuarial::triangle::cumulative(cumulative);
  const actuarial::Reserving cl = actuarial::operators::chain_ladder();
  const actuarial::Reserving bf = actuarial::operators::bornhuetter_ferguson(pluss::make_series({250.0, 280.0, 300.0, 330.0}), pluss::make_series({0.96, 0.90, 0.75, 0.50}));
  out.writeln(cl.key(), cl(triangle).reserve.items<double>());
  out.writeln(bf.key(), bf(triangle).reserve.items<double>());
}
```

Output

```text
chain_ladder [0.00, 22.73, 75.10, 186.26]
bornhuetter_ferguson [10.00, 28.00, 75.00, 165.00]
```

### Example 3: invalid data is refused at the door

A triangle whose cumulative amounts decrease cannot be built.

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
  report([&] { (void)actuarial::triangle::cumulative(pluss::make_series({100.0, 90.0, 0.0, 0.0}, {2, 2})); });
}
```

Output

```text
triangle::cumulative: observed cumulative values must be non-decreasing
```

## Related Pages

Use the persistent sidebar to open each type or factory. The {guide}`actuarial guide <actuarial>` develops longer workflows and mathematical background.

```{toctree}
:maxdepth: 1
:hidden:

reserving/Triangle
reserving/ReserveResult
reserving/Reserving
reserving/cumulative
reserving/chain_ladder
reserving/bornhuetter_ferguson
```

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 2
:hidden:

reserving/Triangle
reserving/ReserveResult
reserving/Reserving
reserving/cumulative
reserving/chain_ladder
reserving/bornhuetter_ferguson
```
<!-- apiref:toctree:end -->
