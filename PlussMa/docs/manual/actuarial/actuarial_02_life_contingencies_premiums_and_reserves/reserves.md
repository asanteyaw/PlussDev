<!-- generated from reserves.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Reserves

[Back to Life Contingencies, Premiums and Reserves](../actuarial_02_life_contingencies_premiums_and_reserves.md)

At issue, premiums and benefits are in balance. As the policy ages they are not: the benefits are nearer, fewer premiums remain to be collected, and the insurer must hold the difference. The **prospective reserve** at duration `t` is the expected present value, at that time, of the benefits still to come less the premiums still to be paid, given that the life is alive at `t`:

```{math}
{}_tV \;=\; A_{x+t:\overline{n-t}|} \;-\; P\,\ddot a_{x+t:\overline{n-t}|}.
```

By the equivalence principle {math}`{}_0V = 0`, and for an endowment insurance {math}`{}_nV = 1`, the survival benefit that is about to be paid. The reserve is the policy's accumulated liability, and it moves according to a recursion: a reserve and the premium collected at the start of the year, accumulated at interest, must pay the death claims of the year and the reserve at the end for the survivors,

```{math}
({}_tV+P)(1+i)\;=\;q_{x+t}\cdot 1\;+\;p_{x+t}\,{}_{t+1}V .
```

```cpp
pluss::Series actuarial::endowment_reserve(const pluss::Series &qx, double interest_rate);
```

| Argument | Meaning |
|---|---|
| `qx` | the annual mortality over the term, one value per year, in [0, 1] |
| `interest_rate` | the annual effective rate, above −1 |

`endowment_reserve` returns the reserves {math}`{}_tV` for `t = 0, ..., n` for a unit endowment insurance with the level net premium of [Net Premiums](net_premiums.md), on the start-of-year convention: one more value than `qx`, from 0 at issue to 1 at maturity. Reserves for other benefits are valuations of the remaining benefits and premiums on the mortality that remains (below).

## The reserve of an endowment insurance

The reserve starts at zero, grows with the years and reaches the benefit at maturity. At duration 4 it is 0.3434: on a unit of cover, the insurer holds about 34 cents for each policy still in force.

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
  const pluss::Series reserve = actuarial::endowment_reserve(qx, i);
  out.writeln("reserve at t = 0..10 =", reserve.items<double>());
  out.writeln("premium              =", actuarial::endowment_premium(qx, i).item<double>());
}
```

Output

```text
reserve at t = 0..10 = [0.000000, 0.080468, 0.164347, 0.251903, 0.343442, 0.439312, 0.539913, 0.645706, 0.757224, 0.875083, 1.000000]
premium              = 0.086456
```

## The reserve from the definition

The definition needs the benefits and premiums still to come, valued on the mortality that remains. For a policy that has run four years, the remaining mortality is `qx[4..10)` and the remaining term is six years. The program builds the valuation of the six remaining years, takes the benefits less the premium times the annuity, and compares with the function.

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
  const double premium = actuarial::endowment_premium(qx, i).item<double>();
  const auto reserve_at = [&](int t) {
    const pluss::Series remaining = qx.slice(0, t, 10);
    const actuarial::Valuation value = actuarial::operators::value({actuarial::mortality::from_qx(remaining), actuarial::discount::flat(i)});
    const std::int64_t years = 10 - t;
    return value(actuarial::cashflow::endowment(years)).item<double>() - premium * value(actuarial::cashflow::annuity(years)).item<double>();
  };
  const auto table = actuarial::endowment_reserve(qx, i).items<double>();
  for (const int t : {0, 2, 4, 6, 8}) out.writeln("t =", t, " from the definition", reserve_at(t), " function", table[static_cast<std::size_t>(t)]);
}
```

Output

```text
t = 0  from the definition -0.000000  function 0.000000
t = 2  from the definition 0.164347  function 0.164347
t = 4  from the definition 0.343442  function 0.343442
t = 6  from the definition 0.539913  function 0.539913
t = 8  from the definition 0.757224  function 0.757224
```

## The recursion

The recursion links consecutive reserves with no sum at all. The program checks it at every duration: the left side is the reserve plus the premium accumulated for a year, and the right side is the death claims plus the reserve held for the survivors.

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
  const double premium = actuarial::endowment_premium(qx, i).item<double>();
  const auto reserve = actuarial::endowment_reserve(qx, i).items<double>();
  const auto q = qx.items<double>();
  for (std::size_t t = 0; t < q.size(); t += 3) {
    out.writeln("t =", t, " (V + P)(1 + i) =", (reserve[t] + premium) * (1.0 + i), " q + p V(t+1) =", q[t] + (1.0 - q[t]) * reserve[t + 1]);
  }
}
```

Output

```text
t = 0  (V + P)(1 + i) = 0.089914  q + p V(t+1) = 0.089914
t = 3  (V + P)(1 + i) = 0.351893  q + p V(t+1) = 0.351893
t = 6  (V + P)(1 + i) = 0.651423  q + p V(t+1) = 0.651423
t = 9  (V + P)(1 + i) = 1.000000  q + p V(t+1) = 1.000000
```

## Reserves for other benefits

For a ten-year term insurance the same definition applies with the benefit `cashflow::death` and the term premium, and the reserve at maturity is zero because no benefit is left. Unlike the endowment reserve it does not rise to the benefit: it rises while the cover still has years to run and the premium is level, then falls back towards zero as the remaining cover shortens, as the output below shows. The program values the reserve at durations 0, 3, 6 and 9 from the remaining mortality, with the equivalence premium of the term insurance.

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
  const actuarial::Valuation full = actuarial::operators::value({actuarial::mortality::from_qx(qx), actuarial::discount::flat(i)});
  const double premium = full(actuarial::cashflow::death(10)).item<double>() / full(actuarial::cashflow::annuity(10)).item<double>();
  out.writeln("term premium =", premium);
  for (int t = 0; t < 10; t += 3) {
    const actuarial::Valuation value = actuarial::operators::value({actuarial::mortality::from_qx(qx.slice(0, t, 10)), actuarial::discount::flat(i)});
    const std::int64_t years = 10 - t;
    out.writeln("t =", t, " term reserve =", value(actuarial::cashflow::death(years)).item<double>() - premium * value(actuarial::cashflow::annuity(years)).item<double>());
  }
}
```

Output

```text
term premium = 0.013725
t = 0  term reserve = 0.000000
t = 3  term reserve = 0.010246
t = 6  term reserve = 0.013325
t = 9  term reserve = 0.005730
```

## The reserve scales with the benefit

Reserves are linear in the benefit: a policy of 250,000 has 250,000 times the reserve of a unit policy, and the premium scales the same way. The program reserves a policy of 250,000 at duration 4.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace actuarial = pluss::ma::actuarial;
namespace life = actuarial::life_contingencies;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const pluss::Series ages = pluss::make_series({60.0, 61.0, 62.0, 63.0, 64.0, 65.0, 66.0, 67.0, 68.0, 69.0});
  const pluss::Series qx = life::makeham_qx(ages, 0.0002, 0.0001, 1.08);
  const double i = 0.04;
  const double sum_assured = 250000.0;
  const auto unit = actuarial::endowment_reserve(qx, i).items<double>();
  out.writeln("unit reserve at t = 4 =", unit[4], " reserve for", sum_assured, "=", sum_assured * unit[4]);
  out.writeln("premium for", sum_assured, "=", sum_assured * actuarial::endowment_premium(qx, i).item<double>());
}
```

Output

```text
unit reserve at t = 4 = 0.3434  reserve for 250000.0000 = 85860.4319
premium for 250000.0000 = 21613.9153
```

## Invalid inputs fail explicitly

The checks are those of the other functions: one-dimensional `qx` with values in `[0, 1]`, and a rate above −1. The reserve is computed from the commutation columns, so the messages come from `commutation_table`.

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
  report([&] { (void)actuarial::endowment_reserve(pluss::make_series({0.1, 1.5}), i); });
  report([&] { (void)actuarial::endowment_reserve(qx, -1.0); });
}
```

Output

```text
commutation_table: qx values must lie in [0, 1]
commutation_table: interest_rate must be > -1
```

## Where to go next

Reserves for losses that have already happened, and the pricing of insurance contracts from experience, are the subject of Chapter 4; the chapters in between treat the loss side of insurance.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="net_premiums.html">
    <span class="label">Previous</span>
    <span class="title">Net premiums</span>
  </a>
</div>
<!-- apiref:nav:end -->
