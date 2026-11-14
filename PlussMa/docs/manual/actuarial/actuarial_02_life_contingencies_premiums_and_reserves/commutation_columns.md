<!-- generated from commutation_columns.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Commutation Columns

[Back to Life Contingencies, Premiums and Reserves](../actuarial_02_life_contingencies_premiums_and_reserves.md)

Before computers, actuaries tabulated a few columns once and priced every policy from them. The columns survive because they answer a different question than the functions of the previous pages: they give the present value at **every duration** of the table in one pass, so a value at the age of 64 costs a division and not a new sum.

Starting from a radix `l_0` of lives and the mortality `qx` over `n` years, the table has these columns, with `v = 1/(1+i)`:

| Column | Definition | Length | Meaning |
|---|---|---|---|
| `lx` | {math}`l_0 \prod_{j<k}(1-q_j)` | n + 1 | lives alive at the start of year `k` |
| `dx` | {math}`l_k - l_{k+1}=l_k q_k` | n | deaths during year `k + 1` |
| `D` | {math}`D_k = v^k\,l_k` | n + 1 | lives at year `k`, discounted to time 0 |
| `N` | {math}`N_k=\sum_{j=k}^{n} D_j` | n + 1 | the tail sum of `D`, zero at `n` as the table ends there |
| `C` | {math}`C_k = v^{k+1}\,d_k` | n | deaths in year `k + 1`, discounted to time 0 |
| `M` | {math}`M_k=\sum_{j=k}^{n} C_j` | n + 1 | the tail sum of `C`, zero at `n` |

The present values of the previous pages are ratios of these columns. For a life at duration `k` of a table that ends at `n`:

```{math}
\ddot a_{k:\overline{n-k}|}=\frac{N_k-N_n}{D_k},\qquad
A^1_{k:\overline{n-k}|}=\frac{M_k-M_n}{D_k},\qquad
{}_{n-k}E_k=\frac{D_n}{D_k}.
```

Because the table stops at `n`, its `N` and `M` are zero at `n` and the formulas above read `N_k / D_k` and `M_k / D_k`.

```cpp
actuarial::CommutationTable actuarial::commutation_table(const pluss::Series &qx, double interest_rate, double radix = 100000.0);
pluss::Series actuarial::lx_qx(const pluss::Series &qx, double radix = 100000.0);
```

| Argument | Meaning |
|---|---|
| `qx` | one-dimensional, values in [0, 1], one per year of the table |
| `interest_rate` | the annual effective rate; must be above −1 |
| `radix` | the number of lives at the start, `l_0`; must be positive |

The `CommutationTable` holds the six columns as `Series`: `lx`, `D`, `N`, `M` with `n + 1` entries, and `dx`, `C` with `n`.

## The table

The table below uses the same ten-year Makeham mortality and 4% interest, and a radix of 100,000 lives.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace actuarial = pluss::ma::actuarial;
namespace life = actuarial::life_contingencies;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(3));
  const pluss::Series ages = pluss::make_series({60.0, 61.0, 62.0, 63.0, 64.0, 65.0, 66.0, 67.0, 68.0, 69.0});
  const pluss::Series qx = life::makeham_qx(ages, 0.0002, 0.0001, 1.08);
  const double i = 0.04;
  const actuarial::CommutationTable table = actuarial::commutation_table(qx, i);
  out.writeln("lx =", table.lx.items<double>());
  out.writeln("dx =", table.dx.items<double>());
  out.writeln("D  =", table.D.items<double>());
  out.writeln("N  =", table.N.items<double>());
  out.writeln("C  =", table.C.items<double>());
  out.writeln("M  =", table.M.items<double>());
}
```

Output

```text
lx = [100000.000, 98972.742, 97876.719, 96708.190, 95463.371, 94138.463, 92729.692, 91233.349, 89645.842, 87963.754, 86183.911]
dx = [1027.258, 1096.023, 1168.529, 1244.819, 1324.908, 1408.771, 1496.343, 1587.507, 1682.088, 1779.843]
D  = [100000.000, 95166.098, 90492.529, 85973.229, 81602.489, 77374.954, 73285.623, 69329.847, 65503.339, 61802.167, 58222.762]
N  = [800530.274, 700530.274, 605364.176, 514871.647, 428898.418, 347295.929, 269920.975, 196635.352, 127305.505, 61802.167, 0.000]
C  = [987.748, 1013.335, 1038.818, 1064.077, 1088.978, 1113.372, 1137.098, 1159.976, 1181.813, 1202.398]
M  = [10987.612, 9999.864, 8986.529, 7947.711, 6883.634, 5794.657, 4681.285, 3544.187, 2384.211, 1202.398, 0.000]
```

`lx` falls from 100,000 to 86,184 over ten years, `D` falls faster because of the discounting, and `N` and `M` are tail sums that end at zero.

## The present values from the columns

At duration 0 the ratios `N_0/D_0` and `M_0/D_0` are the ten-year annuity-due and term insurance of the first page, and `D_10/D_0` is the pure endowment. At duration 4 the same columns give the values for a life that has survived four years, with six years to run, and these equal the functions applied to the remaining probabilities `qx[4..10)`: the columns have done in a division what the function does in a sum.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace actuarial = pluss::ma::actuarial;
namespace life = actuarial::life_contingencies;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6));
  const pluss::Series ages = pluss::make_series({60.0, 61.0, 62.0, 63.0, 64.0, 65.0, 66.0, 67.0, 68.0, 69.0});
  const pluss::Series qx = life::makeham_qx(ages, 0.0002, 0.0001, 1.08);
  const double i = 0.04;
  const actuarial::CommutationTable table = actuarial::commutation_table(qx, i);
  const auto D = table.D.items<double>(), N = table.N.items<double>(), M = table.M.items<double>();
  const pluss::Series remaining = qx.slice(0, 4, 10);

  out.writeln("duration 0: annuity-due", N[0] / D[0], " function", actuarial::annuity_due(qx, i).item<double>());
  out.writeln("duration 0: term       ", M[0] / D[0], " function", actuarial::term_insurance(qx, i).item<double>());
  out.writeln("duration 0: pure       ", D[10] / D[0], " function", actuarial::pure_endowment(qx, i).item<double>());
  out.writeln("duration 4: annuity-due", N[4] / D[4], " function", actuarial::annuity_due(remaining, i).item<double>());
  out.writeln("duration 4: term       ", M[4] / D[4], " function", actuarial::term_insurance(remaining, i).item<double>());
  out.writeln("duration 4: pure       ", D[10] / D[4], " function", actuarial::pure_endowment(remaining, i).item<double>());
}
```

Output

```text
duration 0: annuity-due 8.005303  function 8.005303
duration 0: term        0.109876  function 0.109876
duration 0: pure        0.582228  function 0.582228
duration 4: annuity-due 5.255948  function 5.255948
duration 4: term        0.084356  function 0.084356
duration 4: pure        0.713492  function 0.713492
```

## A shorter term from the same table

The tail sums are what make a shorter term cheap: the annuity-due for years 4 to 7 of the table, a life of 64 for four years, is `(N_4 − N_8)/D_4`, and it needs no new table.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace actuarial = pluss::ma::actuarial;
namespace life = actuarial::life_contingencies;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6));
  const pluss::Series ages = pluss::make_series({60.0, 61.0, 62.0, 63.0, 64.0, 65.0, 66.0, 67.0, 68.0, 69.0});
  const pluss::Series qx = life::makeham_qx(ages, 0.0002, 0.0001, 1.08);
  const double i = 0.04;
  const actuarial::CommutationTable table = actuarial::commutation_table(qx, i);
  const auto D = table.D.items<double>(), N = table.N.items<double>(), M = table.M.items<double>();
  const pluss::Series four_years = qx.slice(0, 4, 8);
  out.writeln("annuity-due, 4 years from duration 4:", (N[4] - N[8]) / D[4], " function", actuarial::annuity_due(four_years, i).item<double>());
  out.writeln("term insurance, same years:          ", (M[4] - M[8]) / D[4], " function", actuarial::term_insurance(four_years, i).item<double>());
}
```

Output

```text
annuity-due, 4 years from duration 4: 3.695879  function 3.695879
term insurance, same years:           0.055138  function 0.055138
```

## The radix is only a scale

The radix sets the number of lives and nothing else: `lx`, `dx`, `D`, `N`, `C` and `M` all scale with it, and every ratio, and so every present value, is unchanged. A radix that is not positive is refused, and so is a rate that has no discount factor.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace actuarial = pluss::ma::actuarial;
namespace life = actuarial::life_contingencies;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6));
  const pluss::Series ages = pluss::make_series({60.0, 61.0, 62.0, 63.0, 64.0, 65.0, 66.0, 67.0, 68.0, 69.0});
  const pluss::Series qx = life::makeham_qx(ages, 0.0002, 0.0001, 1.08);
  const double i = 0.04;
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
  const auto small = actuarial::commutation_table(qx, i, 1000.0);
  const auto large = actuarial::commutation_table(qx, i, 100000.0);
  out.writeln("l_0 of each table       =", small.lx.items<double>()[0], large.lx.items<double>()[0]);
  out.writeln("N_0 / D_0 for radix 1e3 =", small.N.items<double>()[0] / small.D.items<double>()[0], " for radix 1e5 =", large.N.items<double>()[0] / large.D.items<double>()[0]);
  report([&] { (void)actuarial::commutation_table(qx, i, 0.0); });
  report([&] { (void)actuarial::commutation_table(qx, -1.0); });
}
```

Output

```text
l_0 of each table       = 1000.000000 100000.000000
N_0 / D_0 for radix 1e3 = 8.005303  for radix 1e5 = 8.005303
commutation_table: radix must be > 0
commutation_table: interest_rate must be > -1
```

## Where to go next

The columns are the usual route to premiums and reserves at every duration ([Net Premiums](net_premiums.md), [Reserves](reserves.md)), and `lx_qx(qx, radix)` returns just the first column.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="the_valuation_operator.html">
    <span class="label">Previous</span>
    <span class="title">The valuation operator</span>
  </a>
  <a class="next-link" href="net_premiums.html">
    <span class="label">Next</span>
    <span class="title">Net premiums</span>
  </a>
</div>
<!-- apiref:nav:end -->
