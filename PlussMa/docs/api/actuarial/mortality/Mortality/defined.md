<!-- generated from defined.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: e53b3a00cc21 -->

# `actuarial::Mortality::defined`

[Back to `actuarial::Mortality`](../Mortality.md)

Tells whether the law was built by a factory.

## Signature

```cpp
bool actuarial::Mortality::defined() const noexcept;
```

## What It Does

True for a `Mortality` made by one of the factories, false for a default-constructed one. `px`, `force`, `survival`, `lifetime_pmf` and `life_table` throw for an undefined law (`horizon` and `qx` do not), and `operators::value` refuses it, so call `defined` before using a law that may be empty. A law built from an empty vector of probabilities is defined, with horizon 0.

## Returns

`true` if the law was built by a factory.

## Practical Examples

### Example 1: built and empty

A law from `qx` is defined; the default value is not.

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
  out.writeln("from_qx:", actuarial::mortality::from_qx(pluss::make_series({0.01, 0.02})).defined(), " default:", actuarial::Mortality{}.defined());
}
```

Output

```text
from_qx: true  default: false
```

### Example 2: an empty vector is a defined law

A law with no years is valid and has horizon 0.

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
  const actuarial::Mortality empty = actuarial::mortality::from_qx(pluss::series::zeros({0}));
  out.writeln("defined:", empty.defined(), " horizon:", empty.horizon());
}
```

Output

```text
defined: true  horizon: 0
```

### Example 3: guarding a stored law

Use a law only if it was set.

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
  actuarial::Mortality slot;
  out.writeln("before:", slot.defined());
  slot = actuarial::mortality::from_qx(pluss::make_series({0.01, 0.02, 0.03}));
  out.writeln("after: ", slot.defined(), " survival", slot.survival().items<double>());
}
```

Output

```text
before: false
after:  true  survival [1.000000, 0.990000, 0.970200, 0.941094]
```

## See Also

- [`Mortality`](../Mortality.md): the type

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="next-link" href="force.html">
    <span class="label">Next</span>
    <span class="title">force</span>
  </a>
</div>
<!-- apiref:nav:end -->
