<!-- generated from bornhuetter_ferguson.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `actuarial::operators::bornhuetter_ferguson`

Creates a Bornhuetter-Ferguson reserving estimator.

## Signature

```cpp
actuarial::Reserving actuarial::operators::bornhuetter_ferguson(pluss::Series expected_ultimate, pluss::Series percent_reported);
```

## What It Does

The Bornhuetter-Ferguson method does not project the reported claims; it adds to them the part of an **a priori** ultimate that is still to come. For origin `i` with expected ultimate {math}`U_i` and fraction already reported {math}`p_i`, the reserve is {math}`U_i(1-p_i)` and the ultimate is the latest reported amount plus that reserve. Because the reserve does not depend on the reported amount, an unusually high or low diagonal does not pull the estimate with it, which makes the method the stable choice for immature origins where [`chain_ladder`](chain_ladder.md) would multiply a small amount by a large factor. The fractions reported are commonly {math}`1/F_j` from a chain-ladder development pattern. The result is a [`Reserving`](Reserving.md).

## Parameters

`expected_ultimate`
: One-dimensional, finite, non-negative a priori ultimate of each origin.

`percent_reported`
: One-dimensional fractions in {math}`[0,1]`, one per origin: how much of the ultimate is expected to be reported by now.

## Returns

A `Reserving` with key `"bornhuetter_ferguson"`. Its [`ReserveResult`](ReserveResult.md) fills `ultimate` and `reserve`; `age_to_age` and `cdf` are undefined.

## Errors

Throws `bornhuetter_ferguson: expected_ultimate must be defined`, `must be 1D`, `must be finite` and `must be >= 0`, `bornhuetter_ferguson: percent_reported must be <= 1` (and the corresponding lower-bound error) when built, and `bornhuetter_ferguson: assumptions must match origin count` or `bornhuetter_ferguson: triangle must be defined` when called.

## Practical Examples

### Example 1: a reserve from prior knowledge

Expected ultimates and the share reported, for four origins.

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
  const actuarial::Reserving bf = actuarial::operators::bornhuetter_ferguson(pluss::make_series({250.0, 280.0, 300.0, 330.0}), pluss::make_series({0.96, 0.90, 0.75, 0.50}));
  const actuarial::ReserveResult r = bf(triangle);
  out.writeln("ultimate =", r.ultimate.items<double>());
  out.writeln("reserve  =", r.reserve.items<double>());
}
```

Output

```text
ultimate = [250.00, 278.00, 290.00, 295.00]
reserve  = [10.00, 28.00, 75.00, 165.00]
```

### Example 2: by hand

Origin 3: latest 130, expected 330, half reported, so reserve 330 x 0.5 = 165 and ultimate 295.

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
  const actuarial::ReserveResult r = actuarial::operators::bornhuetter_ferguson(pluss::make_series({250.0, 280.0, 300.0, 330.0}), pluss::make_series({0.96, 0.90, 0.75, 0.50}))(triangle);
  out.writeln(330.0 * (1.0 - 0.5), 130.0 + 330.0 * (1.0 - 0.5));
  out.writeln(r.reserve.items<double>()[3], r.ultimate.items<double>()[3]);
}
```

Output

```text
165.00 295.00
165.00 295.00
```

### Example 3: fractions from a development pattern

Use the chain-ladder cumulative factors to set the share reported, as `1 / cdf`.

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
  out.print_defaults(PrintOptions{}.fixed(4));
  const pluss::Series cumulative = pluss::make_series({100.0, 180.0, 220.0, 240.0,
                                                       110.0, 200.0, 250.0, 0.0,
                                                       120.0, 215.0, 0.0, 0.0,
                                                       130.0, 0.0, 0.0, 0.0}, {4, 4});
  const actuarial::Triangle triangle = actuarial::triangle::cumulative(cumulative);
  const actuarial::ReserveResult cl = actuarial::operators::chain_ladder()(triangle);
  const pluss::Series pct = pluss::series::full({4}, 1.0).div(cl.cdf);
  out.writeln("percent reported =", pct.items<double>());
}
```

Output

```text
percent reported = [0.4110, 0.7411, 0.9167, 1.0000]
```

### Example 4: stability against the diagonal

Raising the latest reported amount of the youngest origin moves a chain-ladder ultimate a lot and a BF ultimate by the same amount.

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
  const pluss::Series expected = pluss::make_series({250.0, 280.0, 300.0, 330.0});
  const pluss::Series pct = pluss::make_series({0.96, 0.90, 0.75, 0.50});
  const auto make = [](double young) {
    return actuarial::triangle::cumulative(pluss::make_series({100.0, 180.0, 220.0, 240.0, 110.0, 200.0, 250.0, 0.0, 120.0, 215.0, 0.0, 0.0, young, 0.0, 0.0, 0.0}, {4, 4}));
  };
  for (const double young : {130.0, 180.0}) {
    out.writeln("latest", young, " chain ladder ult", actuarial::operators::chain_ladder()(make(young)).ultimate.items<double>()[3], " BF ult", actuarial::operators::bornhuetter_ferguson(expected, pct)(make(young)).ultimate.items<double>()[3]);
  }
}
```

Output

```text
latest 130.00  chain ladder ult 316.26  BF ult 295.00
latest 180.00  chain ladder ult 437.90  BF ult 345.00
```

### Example 5: the undefined factors

BF does not estimate development factors, so those fields are undefined.

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
  const pluss::Series cumulative = pluss::make_series({100.0, 180.0, 220.0, 240.0,
                                                       110.0, 200.0, 250.0, 0.0,
                                                       120.0, 215.0, 0.0, 0.0,
                                                       130.0, 0.0, 0.0, 0.0}, {4, 4});
  const actuarial::Triangle triangle = actuarial::triangle::cumulative(cumulative);
  const actuarial::ReserveResult r = actuarial::operators::bornhuetter_ferguson(pluss::make_series({250.0, 280.0, 300.0, 330.0}), pluss::make_series({0.96, 0.90, 0.75, 0.50}))(triangle);
  out.writeln("age_to_age defined:", r.age_to_age.defined(), " cdf defined:", r.cdf.defined(), " ultimate defined:", r.ultimate.defined());
}
```

Output

```text
age_to_age defined: false  cdf defined: false  ultimate defined: true
```

### Example 6: the errors

Bad assumptions fail when the operator is built, a wrong length or an undefined triangle when it is called.

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
  report([&] { (void)actuarial::operators::bornhuetter_ferguson(pluss::make_series({-250.0}), pluss::make_series({0.5})); });
  report([&] { (void)actuarial::operators::bornhuetter_ferguson(pluss::make_series({250.0}), pluss::make_series({1.2})); });
  report([&] { (void)actuarial::operators::bornhuetter_ferguson(pluss::make_series({1.0, 2.0}).reshape({1, 2}), pluss::make_series({0.5, 0.5})); });
  report([&] { (void)actuarial::operators::bornhuetter_ferguson(pluss::make_series({250.0, 280.0}), pluss::make_series({0.96, 0.90}))(actuarial::triangle::cumulative(pluss::make_series({100.0, 180.0, 220.0, 240.0, 110.0, 200.0, 250.0, 0.0, 120.0, 215.0, 0.0, 0.0, 130.0, 0.0, 0.0, 0.0}, {4, 4}))); });
  report([&] { (void)actuarial::operators::bornhuetter_ferguson(pluss::make_series({1.0}), pluss::make_series({0.5}))(actuarial::Triangle{}); });
}
```

Output

```text
bornhuetter_ferguson: expected_ultimate must be >= 0
bornhuetter_ferguson: percent_reported must be <= 1
bornhuetter_ferguson: expected_ultimate must be 1D
bornhuetter_ferguson: assumptions must match origin count
bornhuetter_ferguson: triangle must be defined
```

## See Also

- [`Reserving`](Reserving.md): the type
- [`chain_ladder`](chain_ladder.md): projects the reported claims

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="chain_ladder.html">
    <span class="label">Previous</span>
    <span class="title">chain_ladder</span>
  </a>
</div>
<!-- apiref:nav:end -->
