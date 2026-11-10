<!-- generated from defined.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: af93e9ea1078 -->

# `operators::Operator::defined`

[Back to `operators::Operator`](../Operator.md)

Reports whether an `Operator` holds an operation, or is the empty default-constructed value.

## Signature

```cpp
bool operators::Operator::defined() const noexcept;
explicit operators::Operator::operator bool() const noexcept;
```

## What It Does

A default-constructed `Operator` holds nothing, like an empty `std::function`. `defined()` is `false` for it and `true` for every operator returned by a factory, by `define`, or by combining operators. The `explicit operator bool` is the same test spelled for an `if`.

Call it when an operator may legitimately be absent: a pipeline stage that is optional, a slot in a configuration object that is filled later, a lookup that can miss. Applying an undefined operator, or asking it for its capabilities, key or version, is an error; testing first lets the code handle the absence itself.

## Returns

`true` if the operator holds an operation, `false` for a default-constructed one.

## Practical Examples

### Example 1

A default-constructed operator is empty; any factory result is defined. The two spellings agree.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace math = pluss::math;
namespace operators = pluss::operators;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const operators::Operator empty;
  const operators::Operator d = operators::differential("x");
  out.writeln("empty:", empty.defined(), static_cast<bool>(empty));
  out.writeln("differential:", d.defined(), static_cast<bool>(d));
}
```

Output

```text
empty: false false
differential: true true
```

The boolean conversion is `explicit`: write `if (op)` or `static_cast<bool>(op)`, not `bool ok = op;`.

### Example 2

An optional pipeline stage. A stage is applied only if it was configured; an empty `Operator` means "skip".

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace math = pluss::math;
namespace operators = pluss::operators;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const math::Function f = math::function({"x"}, [](math::Function x) { return x * x * x; });
  const auto run = [&](const operators::Operator &stage) {
    const math::Function g = stage ? stage(f) : f;
    return g(2.0).item<double>();
  };
  out.writeln("no stage:      ", run(operators::Operator{}));
  out.writeln("scale(10):     ", run(operators::scale(10.0)));
  out.writeln("differential:  ", run(operators::differential("x")));
}
```

Output

```text
no stage:       8.0000
scale(10):      80.0000
differential:   12.0000
```

### Example 3

Using an undefined operator is an error that says so: applying it, and asking it for its key.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <string>

namespace math = pluss::math;
namespace operators = pluss::operators;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const auto first_line = [](const pluss::Error &e) {
    const std::string text(e.what());
    return text.substr(0, text.find('\n'));
  };

  const operators::Operator empty;
  const math::Function f = math::function({"x"}, [](math::Function x) { return x; });
  try { (void)empty(f); } catch (const pluss::Error &e) { out.writeln("apply:", first_line(e)); }
  try { (void)empty.key(); } catch (const pluss::Error &e) { out.writeln("key:  ", first_line(e)); }
}
```

Output

```text
apply: operators::Operator: operation requires a defined operator
key:   operators::Operator: operation requires a defined operator
```

## See Also

- [`operator()`](call.md): applying a defined operator
- [`math::Function::defined`](../../functions/Function/defined.md): the same test on functions

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="capabilities.html">
    <span class="label">Previous</span>
    <span class="title">capabilities</span>
  </a>
  <a class="next-link" href="key.html">
    <span class="label">Next</span>
    <span class="title">key</span>
  </a>
</div>
<!-- apiref:nav:end -->
