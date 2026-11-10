<!-- generated from endowment_reserve.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `actuarial::endowment_reserve`

The net premium reserve of an endowment insurance at every duration.

## Signature

```cpp
pluss::Series actuarial::endowment_reserve(const pluss::Series &qx, double interest_rate);
```

## What It Does

Returns, for `k = 0, ..., n`, the prospective reserve {math}`{}_kV=A_k-P\,\ddot a_k`: the expected present value at time `k` of the future benefit less that of the future level premiums ([`endowment_premium`](endowment_premium.md)), each read from the [`commutation_table`](commutation_table.md) as {math}`A_k=(M_k+D_n)/D_k` and {math}`\ddot a_k=N_k/D_k`. It starts at zero by the equivalence principle, rises as the benefit comes closer, and ends at the benefit of 1. The result has `n + 1` values.

## Parameters

`qx`
: One-dimensional, finite series of one-year death probabilities in {math}`[0,1]`; the horizon is its length.

`interest_rate`
: The annual effective rate {math}`i`, finite and greater than −1. A zero or negative rate is valid.

## Returns

A one-dimensional series of `qx.numel() + 1` reserves per unit of benefit, from 0 to 1.

## Errors

Throws `endowment_reserve: denominator must be > 0` for an empty horizon, and the `commutation_table: …` errors for invalid inputs.

## Practical Examples

### Example 1: the reserve each year

Five years of mortality at 4%.

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
  const pluss::Series qx = pluss::make_series({0.010, 0.012, 0.015, 0.019, 0.024});
  out.writeln(actuarial::endowment_reserve(qx, 0.04).items<double>());
}
```

Output

```text
[0.000000, 0.181873, 0.371663, 0.570136, 0.778794, 1.000000]
```

### Example 2: the endpoints

Zero at issue and the full benefit at maturity.

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
  out.print_defaults(PrintOptions{}.fixed(8));
  const pluss::Series qx = pluss::make_series({0.010, 0.012, 0.015, 0.019, 0.024});
  const auto v = actuarial::endowment_reserve(qx, 0.04).items<double>();
  out.writeln("first =", v.front(), " last =", v.back());
}
```

Output

```text
first = 0.00000000  last = 1.00000000
```

### Example 3: by hand at duration 2

The future benefit less the future premiums, from the commutation columns.

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
  out.print_defaults(PrintOptions{}.fixed(8));
  const pluss::Series qx = pluss::make_series({0.010, 0.012, 0.015, 0.019, 0.024});
  const actuarial::CommutationTable t = actuarial::commutation_table(qx, 0.04, 1.0);
  const double dn = t.D.items<double>().back();
  const double a2 = (t.N.select(0, 2) / t.D.select(0, 2)).item<double>();
  const double A2 = ((t.M.select(0, 2) + dn) / t.D.select(0, 2)).item<double>();
  out.writeln(A2 - actuarial::endowment_premium(qx, 0.04).item<double>() * a2, actuarial::endowment_reserve(qx, 0.04).items<double>()[2]);
}
```

Output

```text
0.37166304 0.37166304
```

### Example 4: no interest and no mortality

Saving 1 over three years leaves a reserve of one third, two thirds, then 1.

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
  out.writeln(actuarial::endowment_reserve(pluss::make_series({0.0, 0.0, 0.0}), 0.0).items<double>());
}
```

Output

```text
[0.000000, 0.333333, 0.666667, 1.000000]
```

### Example 5: the effect of the rate

At each duration a higher rate means a smaller reserve is needed.

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
  const pluss::Series qx = pluss::make_series({0.010, 0.012, 0.015, 0.019, 0.024});
  for (const double i : {0.0, 0.04, 0.08})
    out.writeln("i =", i, " reserve at 3 =", actuarial::endowment_reserve(qx, i).items<double>()[3]);
}
```

Output

```text
i = 0.000000  reserve at 3 = 0.593784
i = 0.040000  reserve at 3 = 0.570136
i = 0.080000  reserve at 3 = 0.547227
```

### Example 6: the errors

An empty horizon and invalid inputs.

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
  report([&] { (void)actuarial::endowment_reserve(pluss::series::zeros({0}), 0.04); });
  report([&] { (void)actuarial::endowment_reserve(pluss::make_series({0.1, 0.2}), -1.0); });
  report([&] { (void)actuarial::endowment_reserve(pluss::make_series({1.5}), 0.04); });
}
```

Output

```text
endowment_reserve: denominator must be > 0
commutation_table: interest_rate must be > -1
commutation_table: qx values must lie in [0, 1]
```

## See Also

- [`endowment_premium`](endowment_premium.md): the premium that funds it
- [`commutation_table`](commutation_table.md): the columns it is read from

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="endowment_premium.html">
    <span class="label">Previous</span>
    <span class="title">endowment_premium</span>
  </a>
</div>
<!-- apiref:nav:end -->
