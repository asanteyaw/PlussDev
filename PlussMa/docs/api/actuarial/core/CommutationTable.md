<!-- generated from CommutationTable.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `actuarial::CommutationTable`

The six columns of a life table at a rate of interest, from which contingent values are read by division.

## Signature

```cpp
actuarial::CommutationTable actuarial::commutation_table(const pluss::Series &qx, double interest_rate, double radix = 100000.0);
pluss::Series actuarial::CommutationTable::lx;
pluss::Series actuarial::CommutationTable::dx;
pluss::Series actuarial::CommutationTable::D;
pluss::Series actuarial::CommutationTable::N;
pluss::Series actuarial::CommutationTable::C;
pluss::Series actuarial::CommutationTable::M;
```

[`commutation_table`](commutation_table.md) returns a `CommutationTable` of one-dimensional series over a horizon of `n` years. The columns are discounted sums from each age to the end of the horizon, so a benefit starting at age `k` is valued by one division: {math}`\ddot a_k=N_k/D_k` for an annuity-due, {math}`A^1_k=M_k/D_k` for a term insurance and {math}`A_k=(M_k+D_n)/D_k` for an endowment insurance. Read the fields directly.

## Fields

### `lx`

`pluss::Series`. The expected lives alive at each age, `n + 1` values from the radix down. {math}`l_k=l_0\,{}_kp_x`.

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
  out.writeln(t.lx.items<double>());
}
```

Output

```text
[1000.000, 990.000, 978.120, 963.448, 945.143, 922.459]
```


### `dx`

`pluss::Series`. The expected deaths in each year, `n` values: {math}`d_k=l_k-l_{k+1}`. They sum to the lives lost over the horizon.

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
  out.writeln(t.dx.items<double>());
  out.writeln(t.dx.sum().item<double>(), t.lx.items<double>().front() - t.lx.items<double>().back());
}
```

Output

```text
[10.000, 11.880, 14.672, 18.306, 22.683]
77.541 77.541
```


### `D`

`pluss::Series`. The discounted survivors, `n + 1` values: {math}`D_k=v^kl_k`. The denominator of every ratio, and the weight of a payment of 1 made to a life aged `k`.

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
  out.writeln(t.D.items<double>());
  out.writeln("D1 = v * l1 =", t.lx.items<double>()[1] / 1.04);
}
```

Output

```text
[1000.000, 951.923, 904.327, 856.502, 807.912, 758.194]
D1 = v * l1 = 951.923
```


### `N`

`pluss::Series`. The cumulative sum of `D` from each age to the end of the horizon, `n + 1` values with {math}`N_n=0`: {math}`N_k=\sum_{j=k}^{n-1}D_j`. Divided by `D`, it is the annuity-due.

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
  const pluss::Series qx = pluss::make_series({0.010, 0.012, 0.015, 0.019, 0.024});
  const actuarial::CommutationTable t = actuarial::commutation_table(qx, 0.04, 1000.0);
  out.writeln(t.N.items<double>());
  out.writeln("N0 / D0 =", (t.N.select(0, 0) / t.D.select(0, 0)).item<double>(), " annuity_due =", actuarial::annuity_due(qx, 0.04).item<double>());
}
```

Output

```text
[4520.6639, 3520.6639, 2568.7408, 1664.4139, 807.9119, 0.0000]
N0 / D0 = 4.5207  annuity_due = 4.5207
```


### `C`

`pluss::Series`. The discounted deaths, `n` values: {math}`C_k=v^{k+1}d_k`, the value today of the deaths of year `k + 1`, each paying 1 at the end of its year.

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
  const pluss::Series qx = pluss::make_series({0.010, 0.012, 0.015, 0.019, 0.024});
  const actuarial::CommutationTable t = actuarial::commutation_table(qx, 0.04, 1000.0);
  out.writeln(t.C.items<double>());
  out.writeln("C0 = v * d0 =", t.dx.items<double>()[0] / 1.04);
}
```

Output

```text
[9.6154, 10.9837, 13.0432, 15.6476, 18.6441]
C0 = v * d0 = 9.6154
```


### `M`

`pluss::Series`. The cumulative sum of `C`, `n + 1` values with {math}`M_n=0`: {math}`M_k=\sum_{j\ge k}C_j`. Divided by `D`, it is the term insurance.

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
  const pluss::Series qx = pluss::make_series({0.010, 0.012, 0.015, 0.019, 0.024});
  const actuarial::CommutationTable t = actuarial::commutation_table(qx, 0.04, 1000.0);
  out.writeln(t.M.items<double>());
  out.writeln("M0 / D0 =", (t.M.select(0, 0) / t.D.select(0, 0)).item<double>(), " term_insurance =", actuarial::term_insurance(qx, 0.04).item<double>());
}
```

Output

```text
[67.9340, 58.3187, 47.3349, 34.2918, 18.6441, 0.0000]
M0 / D0 = 0.0679  term_insurance = 0.0679
```


## Practical Examples

### Example 1: the endowment insurance from the columns

The endowment insurance adds the discounted survivors at maturity to the term insurance.

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
  const double dn = t.D.items<double>().back();
  out.writeln(((t.M.select(0, 0) + dn) / t.D.select(0, 0)).item<double>(), actuarial::endowment_insurance(qx, 0.04).item<double>());
}
```

Output

```text
0.82612831 0.82612831
```

### Example 2: column lengths

Some columns have `n + 1` entries and some `n`, following the age grid and the years between.

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
  const actuarial::CommutationTable t = actuarial::commutation_table(qx, 0.04, 1000.0);
  out.writeln("lx", t.lx.numel(), " dx", t.dx.numel(), " D", t.D.numel(), " N", t.N.numel(), " C", t.C.numel(), " M", t.M.numel());
}
```

Output

```text
lx 6  dx 5  D 6  N 6  C 5  M 6
```

### Example 3: an undefined table

A default-constructed table has undefined columns.

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
  const actuarial::CommutationTable none;
  out.writeln(none.lx.defined(), none.dx.defined(), none.D.defined(), none.N.defined(), none.C.defined(), none.M.defined());
}
```

Output

```text
false false false false false false
```

## Changelog

- **0.1**: initial operator-first actuarial surface.

## See Also

[`commutation_table`](commutation_table.md) builds it, and [`endowment_premium`](endowment_premium.md) and [`endowment_reserve`](endowment_reserve.md) are read from it.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="commutation_table.html">
    <span class="label">Previous</span>
    <span class="title">commutation_table</span>
  </a>
  <a class="next-link" href="endowment_premium.html">
    <span class="label">Next</span>
    <span class="title">endowment_premium</span>
  </a>
</div>
<!-- apiref:nav:end -->
