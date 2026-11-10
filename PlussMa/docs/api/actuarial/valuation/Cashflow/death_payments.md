<!-- generated from death_payments.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: aab176b2c5bf -->

# `actuarial::Cashflow::death_payments`

[Back to `actuarial::Cashflow`](../Cashflow.md)

The payments contingent on dying, for years 1 to the horizon.

## Signature

```cpp
const pluss::Series &actuarial::Cashflow::death_payments() const noexcept;
```

## What It Does

Returns the `n` payments {math}`d_1,\dots,d_n`: entry `k` is paid at the end of year `k` if the life dies during that year. A term insurance or an endowment has the death benefit in every entry; a pure endowment and an annuity have zeros. The reference is to the cashflow's own series and lives as long as the cashflow.

## Returns

A const reference to a one-dimensional `Series` of `horizon` values; empty for an undefined cashflow.

## Practical Examples

### Example 1: a term insurance

A death benefit of 10 in each of three years.

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
  out.writeln(actuarial::cashflow::death(3, 10.0).death_payments().items<double>());
}
```

Output

```text
[10.0, 10.0, 10.0]
```

### Example 2: benefits that vary

A decreasing death benefit built with `make`: 100, 80 and 60.

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
  const actuarial::Cashflow c = actuarial::cashflow::make(pluss::make_series({0.0, 0.0, 0.0, 0.0}), pluss::make_series({100.0, 80.0, 60.0}));
  out.writeln(c.death_payments().items<double>());
}
```

Output

```text
[100.0, 80.0, 60.0]
```

### Example 3: no death payments

An annuity pays nothing on death.

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
  out.writeln("annuity death payments:", actuarial::cashflow::annuity(3).death_payments().items<double>());
}
```

Output

```text
annuity death payments: [0.0, 0.0, 0.0]
```

### Example 4: the length

There are `horizon` death payments.

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
  out.writeln("horizon", actuarial::cashflow::death(6).horizon(), " death entries", actuarial::cashflow::death(6).death_payments().numel());
}
```

Output

```text
horizon 6  death entries 6
```

## See Also

- [`Cashflow`](../Cashflow.md): the type
- [`survival_payments`](survival_payments.md): the other schedule

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="next-link" href="defined.html">
    <span class="label">Next</span>
    <span class="title">defined</span>
  </a>
</div>
<!-- apiref:nav:end -->
