<!-- generated from Options.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `operators::Options`

What you declare about an operator you define with `operators::define`.

`Options` is the third argument of [`define`](define.md). It carries the operator's version and the flags that become its [`Capabilities`](Capabilities.md), plus an opaque `payload` string that is saved with the operator and handed back to its loader. The defaults are the cautious ones: structural, not linear, not savable, no known adjoint.

Declare only what is true. The library trusts the flags: it uses `linear` when it combines operators and `serializable` to decide whether `io::save` accepts the operator. The contracts that can be checked are checked when `define` is called: contradictory flags, a savable operator without a key, and a zero version are errors.

## Fields

| Field | Default | Meaning |
|---|---|---|
| `version` | `1` | Version of the definition. Saved with the operator and used to choose its loader. Must be positive. |
| `structural` | `true` | The callable builds the result from the recorded graph of its input. |
| `evaluated_only` | `false` | The callable works on values only. Cannot be `true` together with `structural`. |
| `linear` | `false` | The callable is linear in its input. |
| `adjoint_known` | `false` | The adjoint is known. Declaring it requires passing the adjoint operator to `define`; passing one sets this flag by itself. |
| `serializable` | `false` | The operator can be saved. Requires a non-empty key. |
| `platform_pure` | `true` | The callable does not move data between platforms. |
| `payload` | empty | Text saved with the operator and passed to its loader on load. |

## Guidance

Prefer the defaults unless you can state the property. If an operator is linear, say so: combinations of linear operators then report `linear` and the adjoint machinery can build on them. Mark an operator `serializable` only when its key is stable across releases and its parameters can be reconstructed from `payload` by a loader you bind. Bump `version` when the meaning of the payload changes.

## Practical Examples

### Example 1

Declaring flags. The same callable defined twice, once with the defaults and once declared linear with a version and its adjoint; `capabilities()`, `key()` and `version()` report what was declared.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace math = pluss::math;
namespace operators = pluss::operators;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const auto doubler = [](const math::Function &f) { return f * 2.0; };
  const operators::Operator cautious = operators::define("example.double", doubler);
  const operators::Operator declared = operators::define("example.double", doubler, operators::Options{.version = 3, .linear = true}, operators::scale(2.0));
  out.writeln("cautious: version", cautious.version(), " linear", cautious.capabilities().linear, " adjoint_known", cautious.capabilities().adjoint_known);
  out.writeln("declared: version", declared.version(), " linear", declared.capabilities().linear, " adjoint_known", declared.capabilities().adjoint_known);
}
```

Output

```text
cautious: version 1  linear false  adjoint_known false
declared: version 3  linear true  adjoint_known true
```

### Example 2

A savable operator needs a key, the `serializable` flag, a payload that records its parameter, and a loader bound under the key. Saved and loaded, it behaves the same.

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
  const auto make = [](double factor) {
    return operators::define("example.options_scale", [factor](const math::Function &f) { return f * factor; },
                             operators::Options{.linear = true, .serializable = true, .payload = std::to_string(factor)});
  };
  operators::bind("example.options_scale", 1, [make](std::string_view payload, std::uint32_t) { return make(std::stod(std::string(payload))); });

  const math::Function f = math::function({"x"}, [](math::Function x) { return x + 1.0; });
  std::stringstream bytes(std::ios::in | std::ios::out | std::ios::binary);
  math::io::save(make(4.0), bytes);
  const operators::Operator restored = math::io::load_operator(bytes);
  out.writeln("original(f)(2) =", make(4.0)(f)(2.0).item<double>(), "  restored(f)(2) =", restored(f)(2.0).item<double>(), "  key", restored.key());
}
```

Output

```text
original(f)(2) = 12.0000   restored(f)(2) = 12.0000   key example.options_scale
```

### Example 3

The contracts `define` checks: both `structural` and `evaluated_only`, a `serializable` operator with no key, version zero, and `adjoint_known` without an adjoint operator are each refused with a message that names the problem.

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

  const auto pass_through = [](const math::Function &f) { return f; };
  try { (void)operators::define("a", pass_through, operators::Options{.structural = true, .evaluated_only = true}); } catch (const pluss::Error &e) { out.writeln("both flags:        ", first_line(e)); }
  try { (void)operators::define({}, pass_through, operators::Options{.serializable = true}); } catch (const pluss::Error &e) { out.writeln("savable, no key:   ", first_line(e)); }
  try { (void)operators::define("b", pass_through, operators::Options{.version = 0}); } catch (const pluss::Error &e) { out.writeln("version zero:      ", first_line(e)); }
  try { (void)operators::define("c", pass_through, operators::Options{.adjoint_known = true}); } catch (const pluss::Error &e) { out.writeln("adjoint, none:    ", first_line(e)); }
}
```

Output

```text
both flags:         operators::define: structural and evaluated_only cannot both be true
savable, no key:    operators::define: a serializable custom operator requires a stable key
version zero:       operators::define: version must be positive
adjoint, none:     operators::define: adjoint_known requires the adjoint operator to be passed to define
```

## See Also

- [`define`](define.md): builds an operator from a callable and these options
- [`Capabilities`](Capabilities.md): the flags as reported
- [`bind`](bind.md): register the loader

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="Operator.html">
    <span class="label">Previous</span>
    <span class="title">Operator</span>
  </a>
  <a class="next-link" href="adjoint.html">
    <span class="label">Next</span>
    <span class="title">adjoint</span>
  </a>
</div>
<!-- apiref:nav:end -->
