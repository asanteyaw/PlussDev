<!-- generated from ReserveResult.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `actuarial::ReserveResult`

The four series produced by a reserving estimator: development factors, ultimates and reserves.

## Signature

```cpp
actuarial::ReserveResult actuarial::Reserving::operator()(const actuarial::Triangle &triangle) const;
pluss::Series actuarial::ReserveResult::age_to_age;
pluss::Series actuarial::ReserveResult::cdf;
pluss::Series actuarial::ReserveResult::ultimate;
pluss::Series actuarial::ReserveResult::reserve;
```

Calling a [`Reserving`](Reserving.md) operator returns a `ReserveResult`. The origin-level series (`ultimate`, `reserve`) have one value per origin; the factor series describe the development pattern and are filled only by estimators that produce one. Read the fields by name.

## Fields

### `age_to_age`

`pluss::Series`. The factors that take cumulative claims from one age to the next, `ndevelopment − 1` values for [`chain_ladder`](chain_ladder.md). Undefined for [`bornhuetter_ferguson`](bornhuetter_ferguson.md), which does not estimate them.

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
  const auto cl = actuarial::operators::chain_ladder()(triangle);
  out.writeln("chain ladder:", cl.age_to_age.items<double>());
  const auto bf = actuarial::operators::bornhuetter_ferguson(pluss::make_series({250.0, 280.0, 300.0, 330.0}), pluss::make_series({0.96, 0.90, 0.75, 0.50}))(triangle);
  out.writeln("BF defined:", bf.age_to_age.defined());
}
```

Output

```text
chain ladder: [1.8030, 1.2368, 1.0909]
BF defined: false
```


### `cdf`

`pluss::Series`. The cumulative development factors to ultimate, one per age, ending in 1: each is the product of the age-to-age factors from that age on. Their reciprocals are the share of the ultimate reported at each age. Undefined for [`bornhuetter_ferguson`](bornhuetter_ferguson.md).

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
  const auto cl = actuarial::operators::chain_ladder()(triangle);
  out.writeln("cdf         =", cl.cdf.items<double>());
  out.writeln("1 / cdf     =", pluss::series::full({4}, 1.0).div(cl.cdf).items<double>());
}
```

Output

```text
cdf         = [2.4328, 1.3493, 1.0909, 1.0000]
1 / cdf     = [0.4110, 0.7411, 0.9167, 1.0000]
```


### `ultimate`

`pluss::Series`. The estimated ultimate claims of each origin: the latest reported amount plus the reserve. The oldest origin, at the last age, has an ultimate equal to its latest amount in a chain-ladder run.

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
  const auto r = actuarial::operators::chain_ladder()(triangle);
  out.writeln("ultimate =", r.ultimate.items<double>());
  out.writeln("latest + reserve =", r.reserve.add(pluss::make_series({240.0, 250.0, 215.0, 130.0})).items<double>());
}
```

Output

```text
ultimate = [240.00, 272.73, 290.10, 316.26]
latest + reserve = [240.00, 272.73, 290.10, 316.26]
```


### `reserve`

`pluss::Series`. The estimated unpaid (or unreported) claims of each origin, `ultimate − latest reported`, never below zero. The sum over origins is the total reserve.

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
  const auto r = actuarial::operators::chain_ladder()(triangle);
  out.writeln("reserve =", r.reserve.items<double>());
  out.writeln("total   =", r.reserve.sum().item<double>());
}
```

Output

```text
reserve = [0.00, 22.73, 75.10, 186.26]
total   = 284.09
```


## Practical Examples

### Example 1: a full result

All four series of one chain-ladder run.

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
  out.writeln("age_to_age =", r.age_to_age.items<double>());
  out.writeln("cdf        =", r.cdf.items<double>());
  out.writeln("ultimate   =", r.ultimate.items<double>());
  out.writeln("reserve    =", r.reserve.items<double>());
}
```

Output

```text
age_to_age = [1.803, 1.237, 1.091]
cdf        = [2.433, 1.349, 1.091, 1.000]
ultimate   = [240.000, 272.727, 290.096, 316.264]
reserve    = [0.000, 22.727, 75.096, 186.264]
```

### Example 2: comparing two methods

The reserves of the two estimators side by side, origin by origin.

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
  const auto cl = actuarial::operators::chain_ladder()(triangle).reserve.items<double>();
  const auto bf = actuarial::operators::bornhuetter_ferguson(pluss::make_series({250.0, 280.0, 300.0, 330.0}), pluss::make_series({0.96, 0.90, 0.75, 0.50}))(triangle).reserve.items<double>();
  for (std::size_t i = 0; i < cl.size(); ++i) out.writeln("origin", i, " chain ladder", cl[i], " BF", bf[i]);
}
```

Output

```text
origin 0  chain ladder 0.00  BF 10.00
origin 1  chain ladder 22.73  BF 28.00
origin 2  chain ladder 75.10  BF 75.00
origin 3  chain ladder 186.26  BF 165.00
```

### Example 3: an undefined result

A default-constructed result has undefined fields.

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
  const actuarial::ReserveResult none;
  out.writeln(none.age_to_age.defined(), none.cdf.defined(), none.ultimate.defined(), none.reserve.defined());
}
```

Output

```text
false false false false
```

## Changelog

- **0.1**: initial operator-first actuarial surface.

## See Also

[`Reserving`](Reserving.md) produces it, and [`chain_ladder`](chain_ladder.md) and [`bornhuetter_ferguson`](bornhuetter_ferguson.md) create the operators.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="Triangle.html">
    <span class="label">Previous</span>
    <span class="title">Triangle</span>
  </a>
  <a class="next-link" href="Reserving.html">
    <span class="label">Next</span>
    <span class="title">Reserving</span>
  </a>
</div>
<!-- apiref:nav:end -->
