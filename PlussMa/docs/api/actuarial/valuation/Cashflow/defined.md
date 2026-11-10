<!-- generated from defined.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: fd4b2048f75b -->

# `actuarial::Cashflow::defined`

[Back to `actuarial::Cashflow`](../Cashflow.md)

Tells whether the cashflow was built by a factory.

## Signature

```cpp
bool actuarial::Cashflow::defined() const noexcept;
```

## What It Does

True for a `Cashflow` made by `make`, `annuity`, `death`, `survival` or `endowment`, false for a default-constructed one. A valuation refuses an undefined cashflow, so call it before valuing a cashflow that may be empty.

## Returns

`true` if the cashflow was built by a factory.

## Practical Examples

### Example 1: built and empty

A factory-built cashflow is defined; the default value is not.

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
  out.writeln("annuity:", actuarial::cashflow::annuity(3).defined(), " make:", actuarial::cashflow::make(pluss::make_series({1.0, 1.0}), pluss::make_series({0.0})).defined(), " default:", actuarial::Cashflow{}.defined());
}
```

Output

```text
annuity: true  make: true  default: false
```

### Example 2: a zero-year cashflow is defined

A cashflow of zero years is valid and defined; it is just empty of death payments.

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
  const actuarial::Cashflow c = actuarial::cashflow::annuity(0);
  out.writeln("defined:", c.defined(), " horizon:", c.horizon(), " death payments:", c.death_payments().numel());
}
```

Output

```text
defined: true  horizon: 0  death payments: 0
```

### Example 3: guarding a valuation

Value only a defined cashflow.

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
  const actuarial::Mortality mortality = actuarial::mortality::from_qx(qx);
  const actuarial::Basis basis{mortality, actuarial::discount::flat(0.04)};
  const actuarial::Valuation value = actuarial::operators::value(basis);
  const std::vector<actuarial::Cashflow> book = {actuarial::cashflow::annuity(5), actuarial::Cashflow{}, actuarial::cashflow::death(5)};
  for (const auto &c : book) {
    if (c.defined()) out.writeln("value", value(c).item<double>());
    else out.writeln("skipped an undefined cashflow");
  }
}
```

Output

```text
value 4.520664
skipped an undefined cashflow
value 0.067934
```

## See Also

- [`Cashflow`](../Cashflow.md): the type

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="death_payments.html">
    <span class="label">Previous</span>
    <span class="title">death_payments</span>
  </a>
  <a class="next-link" href="horizon.html">
    <span class="label">Next</span>
    <span class="title">horizon</span>
  </a>
</div>
<!-- apiref:nav:end -->
