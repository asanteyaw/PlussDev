<!-- generated from survival_payments.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 321a19dc39e5 -->

# `actuarial::Cashflow::survival_payments`

[Back to `actuarial::Cashflow`](../Cashflow.md)

The payments contingent on being alive, for times 0 to the horizon.

## Signature

```cpp
const pluss::Series &actuarial::Cashflow::survival_payments() const noexcept;
```

## What It Does

Returns the `n + 1` payments {math}`s_0,\dots,s_n`: entry `t` is paid at time `t` if the life is alive at time `t`. For an annuity-due the first `n` entries are the payment amount and the last is zero; for an annuity-immediate the first is zero; for a pure endowment only the last entry is non-zero. Negative entries are premiums. The reference is to the cashflow's own series and lives as long as the cashflow.

## Returns

A const reference to a one-dimensional `Series` of `horizon + 1` values; empty for an undefined cashflow.

## Practical Examples

### Example 1: an annuity-due and an annuity-immediate

The same three payments, one at time 0 and one at time 3.

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
  out.print_defaults(PrintOptions{}.fixed(1));
  out.writeln("due      ", actuarial::cashflow::annuity(3).survival_payments().items<double>());
  out.writeln("immediate", actuarial::cashflow::annuity(3, 1.0, actuarial::PaymentTiming::End).survival_payments().items<double>());
}
```

Output

```text
due       [1.0, 1.0, 1.0, 0.0]
immediate [0.0, 1.0, 1.0, 1.0]
```

### Example 2: a pure endowment and an endowment

The survival benefit sits at the last time.

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
  out.print_defaults(PrintOptions{}.fixed(1));
  out.writeln("pure endowment of 7 ", actuarial::cashflow::survival(3, 7.0).survival_payments().items<double>());
  out.writeln("endowment (10 / 4)  ", actuarial::cashflow::endowment(3, 10.0, 4.0).survival_payments().items<double>());
}
```

Output

```text
pure endowment of 7  [0.0, 0.0, 0.0, 7.0]
endowment (10 / 4)   [0.0, 0.0, 0.0, 4.0]
```

### Example 3: premiums as negative payments

A level premium of 5 at times 0 and 1, and a benefit of 100 at time 2 if alive.

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
  out.print_defaults(PrintOptions{}.fixed(1));
  const actuarial::Cashflow c = actuarial::cashflow::make(pluss::make_series({-5.0, -5.0, 100.0}), pluss::make_series({0.0, 0.0}));
  out.writeln(c.survival_payments().items<double>());
}
```

Output

```text
[-5.0, -5.0, 100.0]
```

### Example 4: the length

There is always one more survival payment than death payments.

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
  const actuarial::Cashflow c = actuarial::cashflow::endowment(4);
  out.writeln("survival entries", c.survival_payments().numel(), " death entries", c.death_payments().numel(), " horizon", c.horizon());
}
```

Output

```text
survival entries 5  death entries 4  horizon 4
```

## See Also

- [`Cashflow`](../Cashflow.md): the type
- [`death_payments`](death_payments.md): the other schedule

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="horizon.html">
    <span class="label">Previous</span>
    <span class="title">horizon</span>
  </a>
</div>
<!-- apiref:nav:end -->
