<!-- generated from defined.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: e0c2167d3749 -->

# `actuarial::Operator<Signature>::defined`

[Back to `actuarial::Operator<Signature>`](../Operator.md)

Tells whether a callable is installed.

## Signature

```cpp
bool actuarial::Operator<Signature>::defined() const noexcept;
```

## What It Does

True for an operator built by [`make_operator`](../make_operator.md) or returned by a library builder, false for a default-constructed one. Calling an undefined operator throws, so test with `defined` when an operator may not have been set.

## Returns

`true` if a callable is installed.

## Practical Examples

### Example 1: built and empty

An operator from `make_operator` is defined; the default value is not.

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
  const actuarial::Operator<double(double)> square = actuarial::make_operator<double(double)>("square", [](double x) { return x * x; });
  out.writeln("built:", square.defined(), " empty:", actuarial::Operator<double(double)>{}.defined());
}
```

Output

```text
built: true  empty: false
```

### Example 2: an optional rule

Use a default rule when no operator was supplied.

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
  const actuarial::Operator<double(double)> square = actuarial::make_operator<double(double)>("square", [](double x) { return x * x; });
  actuarial::Operator<double(double)> rule;
  out.writeln("before:", rule.defined());
  rule = square;
  out.writeln("after: ", rule.defined(), " rule(4) =", rule(4.0));
}
```

Output

```text
before: false
after:  true  rule(4) = 16.000000
```

### Example 3: copies share the behaviour

A copy is defined and calls the same function.

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
  const actuarial::Operator<double(double)> square = actuarial::make_operator<double(double)>("square", [](double x) { return x * x; });
  const actuarial::Operator<double(double)> copy = square;
  out.writeln(copy.defined(), copy.key(), copy(5.0));
}
```

Output

```text
true square 25.000000
```

## See Also

- [`Operator`](../Operator.md): the type
- [`make_operator`](../make_operator.md): builds one

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="next-link" href="key.html">
    <span class="label">Next</span>
    <span class="title">key</span>
  </a>
</div>
<!-- apiref:nav:end -->
