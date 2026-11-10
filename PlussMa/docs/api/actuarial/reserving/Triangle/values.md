<!-- generated from values.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: d97cea685452 -->

# `actuarial::Triangle::values`

[Back to `actuarial::Triangle`](../Triangle.md)

The cumulative amounts as a matrix.

## Signature

```cpp
const pluss::Series &actuarial::Triangle::values() const noexcept;
```

## What It Does

Returns the `norigins × ndevelopment` matrix of cumulative amounts, with zero in every unobserved cell. The reference stays valid as long as the triangle does and costs nothing; copy the series if the triangle is going away. Read the latest reported amounts from the last observed cell of each row, or combine `values` with [`observed`](observed.md) to work only on real data.

## Returns

A const reference to a two-dimensional `Series`; undefined for an undefined triangle.

## Practical Examples

### Example 1: the matrix

Row by row.

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
  out.print_defaults(PrintOptions{}.fixed(0));
  const pluss::Series cumulative = pluss::make_series({100.0, 180.0, 220.0, 240.0,
                                                       110.0, 200.0, 250.0, 0.0,
                                                       120.0, 215.0, 0.0, 0.0,
                                                       130.0, 0.0, 0.0, 0.0}, {4, 4});
  const actuarial::Triangle triangle = actuarial::triangle::cumulative(cumulative);
  const auto v = triangle.values().reshape({16}).items<double>();
  for (int r = 0; r < 4; ++r) out.writeln(v[4*r], v[4*r+1], v[4*r+2], v[4*r+3]);
}
```

Output

```text
100 180 220 240
110 200 250 0
120 215 0 0
130 0 0 0
```

### Example 2: incremental claims

Differences along a row give the amounts reported in each period.

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
  out.print_defaults(PrintOptions{}.fixed(0));
  const pluss::Series cumulative = pluss::make_series({100.0, 180.0, 220.0, 240.0,
                                                       110.0, 200.0, 250.0, 0.0,
                                                       120.0, 215.0, 0.0, 0.0,
                                                       130.0, 0.0, 0.0, 0.0}, {4, 4});
  const actuarial::Triangle triangle = actuarial::triangle::cumulative(cumulative);
  const pluss::Series inc = triangle.values().slice(1, 1, 4, 1).sub(triangle.values().slice(1, 0, 3, 1));
  out.writeln("first row increments =", inc.select(0, 0).items<double>());
}
```

Output

```text
first row increments = [80, 40, 20]
```

### Example 3: only the real data

Multiplying by the mask leaves the same matrix, since the unobserved cells are already zero.

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
  out.print_defaults(PrintOptions{}.fixed(0));
  const pluss::Series cumulative = pluss::make_series({100.0, 180.0, 220.0, 240.0,
                                                       110.0, 200.0, 250.0, 0.0,
                                                       120.0, 215.0, 0.0, 0.0,
                                                       130.0, 0.0, 0.0, 0.0}, {4, 4});
  const actuarial::Triangle triangle = actuarial::triangle::cumulative(cumulative);
  out.writeln(triangle.values().mul(triangle.observed().to(pluss::ScalarType::Float64)).sum().item<double>(), triangle.values().sum().item<double>());
}
```

Output

```text
1765 1765
```

### Example 4: the shape

Rows are origins and columns are ages.

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
  out.writeln(triangle.values().size(0), triangle.values().size(1));
}
```

Output

```text
4 4
```

## See Also

- [`Triangle`](../Triangle.md): the type
- [`cumulative`](../cumulative.md): builds one

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="origins.html">
    <span class="label">Previous</span>
    <span class="title">origins</span>
  </a>
</div>
<!-- apiref:nav:end -->
