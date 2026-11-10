<!-- generated from load_operator.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `math::io::load_operator`

Reads an operator back from a stream or a file written by `save`.

## Signature

```cpp
operators::Operator math::io::load_operator(std::istream &input);
operators::Operator math::io::load_operator(std::string_view path);
```

## What It Does

`load_operator` reads an artifact written by [`save`](save.md) for an operator and returns an equivalent `Operator`. Built-in operators and compositions of them are rebuilt from their key, version and parameters. A *custom* operator is rebuilt by the loader you registered for its key and version with [`bind`](../operators/bind.md); if none is registered the load fails with a message that names the key and version. A damaged or foreign artifact is rejected, and a function artifact loaded as an operator is refused. Open streams in binary mode.

## Parameters

`std::istream &input`
: A stream opened in binary mode, positioned at the start of an operator artifact.

`std::string_view path`
: The path of a file written by `save`.

## Returns

The restored `operators::Operator`.

## Errors

- `no operator loader is bound for key '<key>' version <n>` for a custom operator without a loader.
- A damaged, truncated or foreign artifact is rejected; a function artifact is refused.
- A path that cannot be opened raises an error.

## Practical Examples

### Example 1

A built-in pipeline. A composition of `scale`, `shift` and `differential` is saved as a single artifact and comes back as an operator with the same behaviour.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <sstream>

namespace math = pluss::math;
namespace operators = pluss::operators;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const operators::Operator pipeline = operators::compose(operators::scale(0.5), operators::compose(operators::differential("x"), operators::shift("x", 1.0)));
  std::stringstream bytes(std::ios::in | std::ios::out | std::ios::binary);
  math::io::save(pipeline, bytes);
  const operators::Operator restored = math::io::load_operator(bytes);
  const math::Function f = math::function({"x"}, [](math::Function x) { return x * x * x; });
  out.writeln("original(f)(1) =", pipeline(f)(1.0).item<double>(), "  restored(f)(1) =", restored(f)(1.0).item<double>(), "  (0.5 * 3 * 2^2 = 6)");
}
```

Output

```text
original(f)(1) = 6.0000   restored(f)(1) = 6.0000   (0.5 * 3 * 2^2 = 6)
```

### Example 2

A custom operator needs a loader. The operator is defined `serializable` with its parameter as the payload, a loader is bound under its key, and the saved operator comes back through it.

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
    return operators::define("example.load_scale", [factor](const math::Function &f) { return f * factor; },
                             operators::Options{.linear = true, .serializable = true, .payload = std::to_string(factor)});
  };
  operators::bind("example.load_scale", 1, [make](std::string_view payload, std::uint32_t) { return make(std::stod(std::string(payload))); });
  std::stringstream bytes(std::ios::in | std::ios::out | std::ios::binary);
  math::io::save(make(3.0), bytes);
  const operators::Operator restored = math::io::load_operator(bytes);
  const math::Function f = math::function({"x"}, [](math::Function x) { return x + 1.0; });
  out.writeln("restored key", restored.key(), "  restored(f)(2) =", restored(f)(2.0).item<double>(), "  (3 * 3)");
}
```

Output

```text
restored key example.load_scale   restored(f)(2) = 9.0000   (3 * 3)
```

### Example 3

Without a loader the artifact cannot be read: the save succeeds, the load reports the missing binding. A function artifact loaded as an operator is also refused.

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

  const operators::Operator unbound = operators::define("example.never_bound_in_load", [](const math::Function &f) { return f; }, operators::Options{.serializable = true});
  std::stringstream bytes(std::ios::in | std::ios::out | std::ios::binary);
  math::io::save(unbound, bytes);
  try { (void)math::io::load_operator(bytes); } catch (const pluss::Error &e) { out.writeln("no loader:       ", first_line(e)); }

  std::stringstream function_bytes(std::ios::in | std::ios::out | std::ios::binary);
  math::io::save(math::function({"x"}, [](math::Function x) { return x; }), function_bytes);
  try { (void)math::io::load_operator(function_bytes); } catch (const pluss::Error &e) { out.writeln("function artifact:", first_line(e)); }
}
```

Output

```text
no loader:        math::io: no operator loader is bound for key 'example.never_bound_in_load' version 1
function artifact: math::io: invalid artifact magic
```

### Example 4

From a file by path.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <filesystem>
#include <string>

namespace math = pluss::math;
namespace operators = pluss::operators;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const std::string path = (std::filesystem::temp_directory_path() / "calculus_reference_operator.bin").string();
  math::io::save(operators::differential("x", 2), path);
  const operators::Operator restored = math::io::load_operator(path);
  const math::Function f = math::function({"x"}, [](math::Function x) { return x.pow(4); });
  out.writeln("restored second derivative of x^4 at 2:", restored(f)(2.0).item<double>(), "  (12 x^2 = 48)");
  std::filesystem::remove(path);
}
```

Output

```text
restored second derivative of x^4 at 2: 48.0000   (12 x^2 = 48)
```

## See Also

- [`save`](save.md): writes the artifact
- [`bind`](../operators/bind.md): registers a loader for a custom operator
- [`load_function`](load_function.md): reads a function

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="load_function.html">
    <span class="label">Previous</span>
    <span class="title">load_function</span>
  </a>
  <a class="next-link" href="save.html">
    <span class="label">Next</span>
    <span class="title">save</span>
  </a>
</div>
<!-- apiref:nav:end -->
