<!-- generated from chain_ladder.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `actuarial::operators::chain_ladder`

Creates a volume-weighted chain-ladder reserving estimator.

## Signature

```cpp
actuarial::Reserving actuarial::operators::chain_ladder(pluss::Series cdf = pluss::series::undefined());
```

## What It Does

The chain-ladder method assumes that every origin period develops by the same proportions. The **age-to-age factor** from age `j` to `j + 1` is the volume-weighted ratio {math}`f_j=\sum_i C_{i,j+1}\,/\,\sum_i C_{i,j}`, summed over the origins observed at both ages. The **cumulative development factor** to ultimate is the product {math}`F_j=\prod_{k\ge j}f_k`, with 1 at the last age. An origin whose latest reported amount is {math}`C_i` at age `j` has ultimate {math}`C_iF_j` and reserve {math}`C_i(F_j-1)`, which is never negative since the factors are at least 1. Call it with no argument to estimate the factors from the triangle; pass `cdf` to impose selected factors, for instance after smoothing or from a benchmark, in which case the triangle only supplies the latest diagonal and the reported `age_to_age` are the ratios {math}`F_j/F_{j+1}` implied by the pattern. The result is a [`Reserving`](Reserving.md).

## Parameters

`cdf`
: Optional one-dimensional cumulative development factors, one per development age, each at least 1, non-increasing and ending in 1. Default: estimated from the triangle.

## Returns

A `Reserving` with key `"chain_ladder"`. Its [`ReserveResult`](ReserveResult.md) fills all four fields: the age-to-age factors, the cumulative factors, the ultimates and the reserves.

## Errors

Throws `chain_ladder: cdf length must match development count`, `chain_ladder: final cdf value must equal 1`, `chain_ladder: cdf must be non-increasing` and `chain_ladder: cdf values must be >= 1` for a supplied pattern, `chain_ladder: triangle must be defined`, and `chain_ladder: at least two development periods are required` for a triangle with a single column.

## Practical Examples

### Example 1: estimating the factors

A four-by-four triangle: factors, cumulative factors, ultimates and reserves.

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
  const actuarial::ReserveResult r = actuarial::operators::chain_ladder()(triangle);
  out.writeln("age to age =", r.age_to_age.items<double>());
  out.writeln("cdf        =", r.cdf.items<double>());
  out.writeln("ultimate   =", r.ultimate.items<double>());
  out.writeln("reserve    =", r.reserve.items<double>());
}
```

Output

```text
age to age = [1.8030, 1.2368, 1.0909]
cdf        = [2.4328, 1.3493, 1.0909, 1.0000]
ultimate   = [240.0000, 272.7273, 290.0957, 316.2636]
reserve    = [0.0000, 22.7273, 75.0957, 186.2636]
```

### Example 2: the factor by hand

The first factor is the sum of the second column over the first, for the three origins observed at both ages.

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
  out.writeln("f0 =", (180.0 + 200.0 + 215.0) / (100.0 + 110.0 + 120.0), " f1 =", (220.0 + 250.0) / (180.0 + 200.0), " f2 =", 240.0 / 220.0);
  out.writeln(actuarial::operators::chain_ladder()(triangle).age_to_age.items<double>());
}
```

Output

```text
f0 = 1.803030  f1 = 1.236842  f2 = 1.090909
[1.803030, 1.236842, 1.090909]
```

### Example 3: selected factors

Impose a pattern: the triangle only supplies the latest diagonal, and the reported factors are those the pattern implies.

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
  const actuarial::Reserving selected = actuarial::operators::chain_ladder(pluss::make_series({1.8, 1.2, 1.1, 1.0}));
  const actuarial::ReserveResult r = selected(triangle);
  out.writeln("ultimate =", r.ultimate.items<double>());
  out.writeln("reserve  =", r.reserve.items<double>());
  out.writeln("implied age to age =", r.age_to_age.items<double>());
}
```

Output

```text
ultimate = [240.0000, 275.0000, 258.0000, 234.0000]
reserve  = [0.0000, 25.0000, 43.0000, 104.0000]
implied age to age = [1.5000, 1.0909, 1.1000]
```

### Example 4: the total reserve

Sum the origin reserves for the figure carried on the balance sheet.

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
  const actuarial::ReserveResult r = actuarial::operators::chain_ladder()(triangle);
  out.writeln("total reserve =", r.reserve.sum().item<double>(), " total ultimate =", r.ultimate.sum().item<double>());
}
```

Output

```text
total reserve = 284.09  total ultimate = 1119.09
```

### Example 5: a fully developed origin

The oldest origin is at the last age, so its factor is 1 and its reserve is zero.

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
  const actuarial::ReserveResult r = actuarial::operators::chain_ladder()(triangle);
  out.writeln(r.ultimate.items<double>()[0], r.reserve.items<double>()[0]);
}
```

Output

```text
240.00 0.00
```

### Example 6: the errors

A bad pattern, an undefined triangle and a triangle with one age.

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
  report([&] { (void)actuarial::operators::chain_ladder(pluss::make_series({1.8, 1.2, 1.0}))(actuarial::triangle::cumulative(pluss::make_series({100.0, 180.0, 220.0, 240.0, 110.0, 200.0, 250.0, 0.0, 120.0, 215.0, 0.0, 0.0, 130.0, 0.0, 0.0, 0.0}, {4, 4}))); });
  report([&] { (void)actuarial::operators::chain_ladder(pluss::make_series({1.8, 1.5, 1.2, 1.1}))(actuarial::triangle::cumulative(pluss::make_series({100.0, 180.0, 220.0, 240.0, 110.0, 200.0, 250.0, 0.0, 120.0, 215.0, 0.0, 0.0, 130.0, 0.0, 0.0, 0.0}, {4, 4}))); });
  report([&] { (void)actuarial::operators::chain_ladder(pluss::make_series({1.0, 1.2, 1.5, 1.8}))(actuarial::triangle::cumulative(pluss::make_series({100.0, 180.0, 220.0, 240.0, 110.0, 200.0, 250.0, 0.0, 120.0, 215.0, 0.0, 0.0, 130.0, 0.0, 0.0, 0.0}, {4, 4}))); });
  report([&] { (void)actuarial::operators::chain_ladder()(actuarial::Triangle{}); });
  report([&] { (void)actuarial::operators::chain_ladder()(actuarial::triangle::cumulative(pluss::make_series({1.0, 2.0, 3.0}, {3, 1}))); });
}
```

Output

```text
chain_ladder: cdf length must match development count
chain_ladder: final cdf value must equal 1
chain_ladder: cdf must be non-increasing
chain_ladder: triangle must be defined
chain_ladder: at least two development periods are required
```

## See Also

- [`Reserving`](Reserving.md): the type
- [`bornhuetter_ferguson`](bornhuetter_ferguson.md): uses prior knowledge instead of the development pattern

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="cumulative.html">
    <span class="label">Previous</span>
    <span class="title">cumulative</span>
  </a>
  <a class="next-link" href="bornhuetter_ferguson.html">
    <span class="label">Next</span>
    <span class="title">bornhuetter_ferguson</span>
  </a>
</div>
<!-- apiref:nav:end -->
