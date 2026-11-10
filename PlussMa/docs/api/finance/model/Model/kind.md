<!-- generated from kind.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: eae351d237f3 -->

# `module::Model::kind`

[Back to `module::Model`](../Model.md)

The family the model holds, as a `Kind`.

## Signature

```cpp
[[nodiscard]] model::Kind module::Model::kind() const;
```

## What It Does

Returns the [`Kind`](../Kind.md) of the held struct. Use it to branch on the family, or to rebuild a model of the same family from a new vector of parameters; [`name`](name.md) gives the same information as text.

## Returns

The `Kind`.

## Practical Examples

### Example 1

The kind of three models.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace fin = pluss::subsp::math::finance;
namespace mod = fin::module;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(0));
  for (const auto &m : {fin::model::make(fin::model::Kind::BlackScholes, {0.2}), fin::model::make(fin::model::Kind::Heston, {1.5, 0.04, 0.3, -0.7, 0.04}), fin::model::make(fin::model::Kind::RoughBergomi, {0.04, 1.5, -0.1, -0.7})})
    out.writeln(m.name(), ": kind", static_cast<int>(m.kind()));
}
```

Output

```text
BlackScholes : kind 0
Heston : kind 2
RoughBergomi : kind 5
```

### Example 2

Rebuilding the same family from a new vector, the pattern of a calibration.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace fin = pluss::subsp::math::finance;
namespace mod = fin::module;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(3));
  const mod::Model start = fin::model::make(fin::model::Kind::Heston, {1.5, 0.04, 0.3, -0.7, 0.04});
  const mod::Model next = fin::model::make(start.kind(), {2.0, 0.05, 0.4, -0.6, 0.05});
  out.writeln(start.name(), "->", next.name(), next.parameters().items<double>());
}
```

Output

```text
Heston -> Heston [2.000, 0.050, 0.400, -0.600, 0.050]
```

### Example 3

A model assigned from a struct reports that struct's kind.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace fin = pluss::subsp::math::finance;
namespace mod = fin::module;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  mod::Model m;
  m = fin::model::Sabr{.alpha0 = pluss::make_series({0.2}), .beta = pluss::make_series({0.5}), .nu = pluss::make_series({0.4}), .rho = pluss::make_series({-0.3})};
  out.writeln(m.name(), m.kind() == fin::model::Kind::Sabr);
}
```

Output

```text
Sabr true
```

## See Also

- [`Model`](../Model.md): the class
- [`name`](name.md): as text

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="holds.html">
    <span class="label">Previous</span>
    <span class="title">holds</span>
  </a>
  <a class="next-link" href="make.html">
    <span class="label">Next</span>
    <span class="title">make</span>
  </a>
</div>
<!-- apiref:nav:end -->
