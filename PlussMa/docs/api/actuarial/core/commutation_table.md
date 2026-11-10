<!-- generated from commutation_table.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `actuarial::commutation_table`

The commutation columns of a life table at a rate of interest.

## Signature

```cpp
actuarial::CommutationTable actuarial::commutation_table(const pluss::Series &qx, double interest_rate, double radix = 100000.0);
```

## What It Does

Builds the six classical columns from which contingent values are read by division: the survivors {math}`l_k`, the deaths {math}`d_k`, and the discounted columns {math}`D_k=v^kl_k`, {math}`N_k=\sum_{j\ge k}D_j`, {math}`C_k=v^{k+1}d_k` and {math}`M_k=\sum_{j\ge k}C_j`. Because every column is a discounted sum from age `k` on, the value of a benefit starting at any age is a ratio of two entries: {math}`\ddot a_k=N_k/D_k` and {math}`A_k=M_k/D_k`. Use it when many ages are priced from one table or the textbook formulas are wanted; for a single value the direct functions ([`annuity_due`](annuity_due.md) and the others) are shorter. The sums run over the horizon only, so {math}`N_n=M_n=0`. The result is a [`CommutationTable`](CommutationTable.md).

## Parameters

`qx`
: One-dimensional, finite series of one-year death probabilities in {math}`[0,1]`; the horizon is its length.

`interest_rate`
: The annual effective rate {math}`i`, finite and greater than −1. A zero or negative rate is valid.

`radix`
: Starting number of lives, finite and greater than 0. Default 100000.

## Returns

A `CommutationTable` with the columns `lx`, `D`, `N`, `M` of length `n + 1` and `dx`, `C` of length `n`.

## Errors

Throws `commutation_table: interest_rate must be finite`, `commutation_table: interest_rate must be > -1`, `commutation_table: radix must be finite`, `commutation_table: radix must be > 0`, and `commutation_table: qx must be defined`, `must be 1D`, `must be finite` or `values must lie in [0, 1]`.

## Practical Examples

### Example 1: the six columns

A cohort of 1,000 over five years at 4%.

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
  const pluss::Series qx = pluss::make_series({0.010, 0.012, 0.015, 0.019, 0.024});
  const actuarial::CommutationTable t = actuarial::commutation_table(qx, 0.04, 1000.0);
  out.writeln("lx =", t.lx.items<double>());
  out.writeln("dx =", t.dx.items<double>());
  out.writeln("D  =", t.D.items<double>());
  out.writeln("N  =", t.N.items<double>());
  out.writeln("C  =", t.C.items<double>());
  out.writeln("M  =", t.M.items<double>());
}
```

Output

```text
lx = [1000.000, 990.000, 978.120, 963.448, 945.143, 922.459]
dx = [10.000, 11.880, 14.672, 18.306, 22.683]
D  = [1000.000, 951.923, 904.327, 856.502, 807.912, 758.194]
N  = [4520.664, 3520.664, 2568.741, 1664.414, 807.912, 0.000]
C  = [9.615, 10.984, 13.043, 15.648, 18.644]
M  = [67.934, 58.319, 47.335, 34.292, 18.644, 0.000]
```

### Example 2: reading values by division

The annuity-due and term insurance are N0 / D0 and M0 / D0.

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
  const actuarial::CommutationTable t = actuarial::commutation_table(qx, 0.04, 1000.0);
  out.writeln("N0 / D0 =", (t.N.select(0, 0) / t.D.select(0, 0)).item<double>(), " annuity_due =", actuarial::annuity_due(qx, 0.04).item<double>());
  out.writeln("M0 / D0 =", (t.M.select(0, 0) / t.D.select(0, 0)).item<double>(), " term_insurance =", actuarial::term_insurance(qx, 0.04).item<double>());
}
```

Output

```text
N0 / D0 = 4.52066387  annuity_due = 4.52066387
M0 / D0 = 0.06793404  term_insurance = 0.06793404
```

### Example 3: values at a later age

The ratio at age `k` is the value of a benefit starting then.

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
  const actuarial::CommutationTable t = actuarial::commutation_table(qx, 0.04);
  out.writeln("annuity-due at k = 2:", (t.N.select(0, 2) / t.D.select(0, 2)).item<double>());
  out.writeln("direct, from the last 3 years:", actuarial::annuity_due(qx.slice(0, 2, 5, 1), 0.04).item<double>());
}
```

Output

```text
annuity-due at k = 2: 2.84050018
direct, from the last 3 years: 2.84050018
```

### Example 4: the radix only scales

Ratios do not depend on the starting cohort.

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
  const auto a = actuarial::commutation_table(qx, 0.04, 1.0);
  const auto b = actuarial::commutation_table(qx, 0.04, 100000.0);
  out.writeln((a.N.select(0, 0) / a.D.select(0, 0)).item<double>(), (b.N.select(0, 0) / b.D.select(0, 0)).item<double>());
}
```

Output

```text
4.52066387 4.52066387
```

### Example 5: no years

An empty horizon has a one-entry table and empty deaths.

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
  const actuarial::CommutationTable t = actuarial::commutation_table(pluss::series::zeros({0}), 0.04);
  out.writeln("lx =", t.lx.items<double>(), " dx entries:", t.dx.numel(), " N =", t.N.items<double>());
}
```

Output

```text
lx = [100000.00]  dx entries: 0  N = [0.00]
```

### Example 6: the errors

Invalid rates, radices and probabilities are refused.

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
  report([&] { (void)actuarial::commutation_table(pluss::make_series({0.1}), -1.0); });
  report([&] { (void)actuarial::commutation_table(pluss::make_series({0.1}), std::nan("")); });
  report([&] { (void)actuarial::commutation_table(pluss::make_series({0.1}), 0.04, -1.0); });
  report([&] { (void)actuarial::commutation_table(pluss::make_series({1.5}), 0.04); });
  report([&] { (void)actuarial::commutation_table(pluss::make_series({0.1, 0.2}).reshape({1, 2}), 0.04); });
  report([&] { (void)actuarial::commutation_table(pluss::series::undefined(), 0.04); });
}
```

Output

```text
commutation_table: interest_rate must be > -1
commutation_table: interest_rate must be finite
commutation_table: radix must be > 0
commutation_table: qx values must lie in [0, 1]
commutation_table: qx must be 1D
commutation_table: qx must be defined
```

## See Also

- [`CommutationTable`](CommutationTable.md): the result and its columns
- [`endowment_premium`](endowment_premium.md), [`endowment_reserve`](endowment_reserve.md): built from it

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="lx_qx.html">
    <span class="label">Previous</span>
    <span class="title">lx_qx</span>
  </a>
  <a class="next-link" href="CommutationTable.html">
    <span class="label">Next</span>
    <span class="title">CommutationTable</span>
  </a>
</div>
<!-- apiref:nav:end -->
