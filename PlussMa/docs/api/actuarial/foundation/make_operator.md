<!-- generated from make_operator.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `actuarial::make_operator`

Creates an operator from a key and a callable.

## Signature

```cpp
template <typename Signature, typename Callable>
actuarial::Operator<Signature> actuarial::make_operator(std::string key, Callable &&callable);
```

## What It Does

Wraps any callable that accepts the arguments of `Signature` and returns something convertible to its result into an [`Operator`](Operator.md) with the given key. `Signature` is written explicitly, so the compiler checks the callable against it when the operator is made, not when it is used. Use it for a rule the library does not ship and that should be reusable, copyable and nameable like a library one; for a one-off computation call the lambda directly. The callable is copied into the operator, so captures should be values or long-lived references.

## Parameters

`Signature`
: A function type such as `double(double)` or `pluss::Series(const pluss::Series &)`.

`key`
: The descriptive name returned by `Operator::key`.

`callable`
: A function object invocable with the arguments of `Signature`.

## Returns

A defined `Operator<Signature>` that owns the callable and can be copied.

## Errors

A callable that cannot be called with the signature's arguments is a compile error. The returned operator throws what the callable throws.

## Practical Examples

### Example 1: a scalar rule

A simple rule with a key.

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
  out.writeln(square.key(), square(7.0));
}
```

Output

```text
square 49.000000
```

### Example 2: a rule on a Series

A loss rule: a cap on each claim.

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
  const auto cap = actuarial::make_operator<pluss::Series(const pluss::Series &)>("cap_500", [](const pluss::Series &x) { return x.clamp_max(500.0); });
  out.writeln(cap(pluss::make_series({100.0, 700.0, 450.0, 2000.0})).items<double>());
}
```

Output

```text
[100.000000, 500.000000, 450.000000, 500.000000]
```

### Example 3: a rule with captured state

A parameter fixed when the operator is made.

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
  const double rate = 0.05;
  const auto grow = actuarial::make_operator<double(double)>("grow", [rate](double x) { return x * (1.0 + rate); });
  out.writeln(grow(100.0));
}
```

Output

```text
105.000000
```

### Example 4: copies are independent values

A copy behaves the same and keeps the key.

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
  out.writeln(copy.key(), copy(3.0), square(3.0));
}
```

Output

```text
square 9.000000 9.000000
```

### Example 5: the errors

The callable's exceptions propagate through the operator.

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
  const auto positive = actuarial::make_operator<double(double)>("positive", [](double x) { if (x <= 0) throw std::invalid_argument("x must be positive"); return std::sqrt(x); });
  out.writeln(positive(4.0));
  report([&] { (void)positive(-1.0); });
}
```

Output

```text
2.000000
x must be positive
```

## See Also

- [`Operator`](Operator.md): the type returned

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="Operator.html">
    <span class="label">Previous</span>
    <span class="title">Operator</span>
  </a>
</div>
<!-- apiref:nav:end -->
