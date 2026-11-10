<!-- generated from bind.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `operators::bind`

Registers a loader that rebuilds a custom operator when it is read back from a saved artifact.

## Signature

```cpp
using operators::Loader = std::function<operators::Operator(std::string_view payload, std::uint32_t version)>;
bool operators::bind(std::string_view key, std::uint32_t version, operators::Loader loader);
```

## What It Does

Saving an operator writes its key, its version and its payload; it cannot write the C++ callable. To read a custom operator back, the library needs to know how to rebuild it, and `bind` is where you say so. You register a loader for a `(key, version)` pair: a function that receives the saved payload and returns an operator. When `io::load_operator` meets that key and version it calls your loader.

Typically you define the operator with `serializable = true`, put the parameters needed to rebuild it in `Options::payload`, and bind a loader that parses the payload and calls the same factory. Built-in operators are already known and need no binding.

`bind` returns `true` if the pair was newly registered and `false` if a loader for that key and version already exists; an existing loader is never replaced. A loader is registered for the life of the program, and registration is thread-safe.

## Parameters

`std::string_view key`
: The key the saved operators carry.

`std::uint32_t version`
: The version the loader handles. Artifacts saved at other versions need their own loader.

`operators::Loader loader`
: A callable receiving the saved payload and version and returning the rebuilt `Operator`.

## Returns

`true` if the loader was registered; `false` if a loader for that key and version already exists (the existing one is kept).

## Errors

- Loading an artifact whose key and version have no loader raises `no operator loader is bound for key '<key>' version <n>`.

## Practical Examples

### Example 1

A complete round trip. A savable `scale_by` operator keeps its factor in the payload; the bound loader parses it back. The restored operator behaves like the original.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <sstream>
#include <string>

namespace math = pluss::math;
namespace operators = pluss::operators;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const auto scale_by = [](double factor) {
    return operators::define("example.scale_by", [factor](const math::Function &f) { return f * factor; },
                             operators::Options{.linear = true, .serializable = true, .payload = std::to_string(factor)});
  };
  operators::bind("example.scale_by", 1, [scale_by](std::string_view payload, std::uint32_t) { return scale_by(std::stod(std::string(payload))); });

  const math::Function f = math::function({"x"}, [](math::Function x) { return x * x; });
  std::stringstream bytes(std::ios::in | std::ios::out | std::ios::binary);
  math::io::save(scale_by(1.5), bytes);
  const operators::Operator restored = math::io::load_operator(bytes);
  out.writeln("restored key", restored.key(), "  restored(f)(4) =", restored(f)(4.0).item<double>(), "  (1.5 * 16 = 24)");
}
```

Output

```text
restored key example.scale_by   restored(f)(4) = 24.0000   (1.5 * 16 = 24)
```

### Example 2

The return value. The first binding of a key and version succeeds; binding the same pair again is refused and the first loader stays. A different version of the same key is a different pair.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace math = pluss::math;
namespace operators = pluss::operators;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const operators::Loader loader = [](std::string_view, std::uint32_t) { return operators::identity(); };
  out.writeln("first bind of (example.once, 1): ", operators::bind("example.once", 1, loader));
  out.writeln("same pair again:                 ", operators::bind("example.once", 1, loader));
  out.writeln("same key, version 2:             ", operators::bind("example.once", 2, loader));
}
```

Output

```text
first bind of (example.once, 1):  true
same pair again:                  false
same key, version 2:              true
```

### Example 3

Without a loader the artifact cannot be read back. Saving succeeds, since the key, version and payload are all written, but loading names the missing binding.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <sstream>
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

  const operators::Operator op = operators::define("example.never_bound", [](const math::Function &f) { return f; },
                                                   operators::Options{.serializable = true});
  std::stringstream bytes(std::ios::in | std::ios::out | std::ios::binary);
  math::io::save(op, bytes);
  out.writeln("saved bytes > 0:", bytes.str().size() > 0);
  try { (void)math::io::load_operator(bytes); } catch (const pluss::Error &e) { out.writeln(first_line(e)); }
}
```

Output

```text
saved bytes > 0: true
math::io: no operator loader is bound for key 'example.never_bound' version 1
```

## See Also

- [`define`](define.md): makes the operator being saved
- [`Options`](Options.md): `serializable` and `payload`
- [`Operator::version`](Operator/version.md): the version a loader is selected by

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="adjoint.html">
    <span class="label">Previous</span>
    <span class="title">adjoint</span>
  </a>
  <a class="next-link" href="commutator.html">
    <span class="label">Next</span>
    <span class="title">commutator</span>
  </a>
</div>
<!-- apiref:nav:end -->
