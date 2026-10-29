<!-- generated from saving_and_loading.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Saving and loading

[Back to Operators](../calculus_05_operators.md)

A `Function` or an `Operator` that has structure can be written to a stream or a file and read back, so a derivative that took effort to build, or a pipeline of operators, can be stored and reused in another run. The format is a versioned binary artifact with a checksum; what is read back is a new, equivalent object.

```cpp
void math::io::save(const Function &function, std::ostream &output);
void math::io::save(const Function &function, std::string_view path);
Function math::io::load_function(std::istream &input);
Function math::io::load_function(std::string_view path);

void math::io::save(const operators::Operator &op, std::ostream &output);
void math::io::save(const operators::Operator &op, std::string_view path);
operators::Operator math::io::load_operator(std::istream &input);
operators::Operator math::io::load_operator(std::string_view path);
```

Streams should be opened in binary mode. The `Function` and `Operator` artifacts are different kinds; loading one as the other is an error.

## Saving a function

A saved function keeps its variables, its constants (including array constants of any shape), its structure, and its assumptions. A function that came from `differential`, `gradient`, `antiderivative` or an exact integral is saved like any other. A numerical integral is saved as the integral and is evaluated again after loading. The restored function evaluates to the same values as the original.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/aux/output.h>

#include <filesystem>
#include <sstream>

namespace math = pluss::math;
namespace operators = pluss::operators;
using namespace pluss::tils;

static math::Function through_a_stream(const math::Function &f) {
  std::ostringstream output(std::ios::binary);
  math::io::save(f, output);
  std::istringstream input(output.str(), std::ios::binary);
  return math::io::load_function(input);
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6));
  const math::Function f = math::function({"x", "y"}, [](math::Function x, math::Function y) { return x.pow(2) * (x * y).sin(); });
  const math::Function dfdx = operators::differential("x")(f);
  const auto x = pluss::make_series({0.5, 1.0});
  const auto y = pluss::make_series({1.0, 2.0});

  const math::Function restored = through_a_stream(dfdx);
  out.writeln("original df/dx ", dfdx({x, y}).pull<double>());
  out.writeln("restored df/dx ", restored({x, y}).pull<double>());
  out.writeln("variables:", restored.variables()[0], restored.variables()[1], " arity", restored.arity(), " exact:", restored.provenance() == math::Provenance::Structural ? "yes" : "no");

  // A restored function can be differentiated further.
  out.writeln("d/dy of the restored function ", operators::differential("y")(restored)({x, y}).pull<double>());

  // Array constants and assumptions are kept.
  const auto weights = pluss::make_series({{1.0, 2.0}, {3.0, 4.0}});
  const math::Function with_constant = math::function({"x"}, [&](math::Function v) { return v * math::function(weights); });
  const math::Function constant_back = through_a_stream(with_constant);
  out.writeln("array constant, rank", constant_back.rank(), " shape", constant_back.shape(), " at x = 2:", constant_back({pluss::make_series(2.0, as_f64())}).host<double>().array().data);
  const math::Function assumed = math::function({"x"}, [](math::Function v) { return v.pow(2).sqrt(); }).assume("x", math::Assumption::Positive);
  out.writeln("assumption kept (Positive):", through_a_stream(assumed).assumption("x") == math::Assumption::Positive ? "yes" : "no");

  // The same through a file.
  const std::string path = (std::filesystem::temp_directory_path() / "calculus_guide_function.bin").string();
  math::io::save(dfdx, path);
  out.writeln("from a file    ", math::io::load_function(path)({x, y}).pull<double>());
  std::filesystem::remove(path);
}
```

Output

```text
original df/dx  [0.698821, 0.986301]
restored df/dx  [0.698821, 0.986301]
variables: x y  arity 2  exact: yes
d/dy of the restored function  [0.598259, -3.067035]
array constant, rank 2  shape [2, 2]  at x = 2: [2.000000, 4.000000, 6.000000, 8.000000]
assumption kept (Positive): yes
from a file     [0.698821, 0.986301]
```

The restored derivative gives the same values as the original at every point, has the same two variables and is still `Structural`. Because it is a full function, it can be differentiated again: the derivative with respect to `y` of the restored function is the mixed second derivative. The array constant comes back with its rank and shape (`[2, 2]`), the assumption `Positive` survives, and saving to a file by path and loading from the path gives the same result as the stream. The file is removed at the end because the program wrote it only for this demonstration.

### What cannot be saved

An opaque function cannot be saved, because it contains your callable and a callable is not data: `save` raises `opaque Function callables are not serializable`. The same holds for an autodiff derivative of an opaque function. An undefined function cannot be saved either.

### A restored function is a new trace

A loaded function has its own variable placeholders, just as one made by a second call to `math::function(...)`. It therefore cannot be added to, multiplied by or otherwise combined directly with the function it was saved from, or any other independent function; it can be evaluated, differentiated, integrated, transformed by operators, and saved again.

## Corruption and incompatibility

Every artifact begins with a header that names its kind and a schema version, and ends with a checksum. A damaged file, a truncated one, one from a different schema, or a file that is not an artifact at all is rejected with an error and never produces a partly built object.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/aux/output.h>

#include <functional>
#include <sstream>
#include <string>

namespace math = pluss::math;
namespace operators = pluss::operators;
using namespace pluss::tils;

static void attempt(OutputEngine &out, const char *label, const std::function<void()> &call) {
  try {
    call();
    out.writeln(label, "-> loaded");
  } catch (const pluss::Error &e) {
    const std::string text(e.what());
    out.writeln(label, "->", text.substr(0, text.find('\n')));
  }
}

int main() {
  OutputEngine out;
  const math::Function square = math::function({"x"}, [](math::Function x) { return x.pow(2); });
  std::ostringstream output(std::ios::binary);
  math::io::save(square, output);
  const std::string good = output.str();

  attempt(out, "intact artifact             ", [&] { std::istringstream in(good, std::ios::binary); (void)math::io::load_function(in); });

  std::string flipped = good;
  flipped.back() ^= 0x1;
  attempt(out, "last byte altered           ", [&] { std::istringstream in(flipped, std::ios::binary); (void)math::io::load_function(in); });
  attempt(out, "truncated                   ", [&] { std::istringstream in(good.substr(0, good.size() / 2), std::ios::binary); (void)math::io::load_function(in); });
  attempt(out, "empty stream                ", [&] { std::istringstream in(std::string(), std::ios::binary); (void)math::io::load_function(in); });
  attempt(out, "not an artifact             ", [&] { std::istringstream in(std::string("this is plain text, not an artifact"), std::ios::binary); (void)math::io::load_function(in); });
  attempt(out, "a Function loaded as Operator", [&] { std::istringstream in(good, std::ios::binary); (void)math::io::load_operator(in); });
  attempt(out, "opaque function, saved      ", [&] {
    const math::Function opaque = math::function({"x"}, [](const pluss::Series &v) { return v.sin(); });
    std::ostringstream o(std::ios::binary);
    math::io::save(opaque, o);
  });
  attempt(out, "undefined function, saved   ", [&] { std::ostringstream o(std::ios::binary); math::io::save(math::Function{}, o); });
  attempt(out, "a path that does not exist  ", [&] { (void)math::io::load_function("/this/path/does/not/exist.bin"); });
}
```

Output

```text
intact artifact              -> loaded
last byte altered            -> math::io: checksum mismatch; artifact is corrupted
truncated                    -> math::io: corrupted length-prefixed field
empty stream                 -> math::io: artifact is truncated or corrupted
not an artifact              -> math::io: corrupted length-prefixed field
a Function loaded as Operator -> math::io: invalid artifact magic
opaque function, saved       -> math::io::save: opaque Function callables are not serializable
undefined function, saved    -> math::Function: operation requires a defined function
a path that does not exist   -> math::io::load: could not open input path
```

An intact artifact loads. A single altered byte is caught by the checksum, and truncation, an empty stream and plain text are all rejected as corrupted, with no partial result. Loading a `Function` artifact as an `Operator` is refused because the header names the wrong kind. The last three rows are the failures to save or open.

## Saving operators

An operator is saved as a small description (its key, its version, its parameters) and rebuilt on loading; it is not saved with any function. Saving works for the operators whose description is complete:

| Saves | Does not save |
|---|---|
| `identity`, `scale`, `shift` | `multiply` (holds a coefficient function) |
| `differential`, `magnitude_differential`, `gradient`, `jacobian`, `hessian` (with their `DiffOptions`) | `covariant`, `geometry::absolute_differential`, `geometry::covariant` (hold a connection function) |
| `antiderivative`, `integral` (both forms) | `linear_combination`, `commutator` |
| `stochastic_integral`, `quadratic_variation` | `generator`, `stochastic_differential`, `convention_change`, `covariation` (hold functions) |
| `compose` of operators that save | an operator defined without `serializable` |

The restored operator behaves exactly like the original:

```cpp
#include <pluss/ma.h>
#include <pluss/tils/aux/output.h>

#include <sstream>

namespace math = pluss::math;
namespace operators = pluss::operators;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6));
  // Two derivatives, then a scale: a small pipeline with its options.
  const operators::Operator plan = operators::compose(operators::scale(2.5), operators::differential("x", 2, {.max_nodes = 2048}));
  std::ostringstream output(std::ios::binary);
  math::io::save(plan, output);
  std::istringstream input(output.str(), std::ios::binary);
  const operators::Operator restored = math::io::load_operator(input);

  const math::Function quartic = math::function({"x"}, [](math::Function x) { return x.pow(4); });
  const auto x = pluss::make_series({1.0, 2.0, 3.0});
  out.writeln("restored key", restored.key(), " version", restored.version());
  out.writeln("original pipeline  ", plan(quartic)({x}).pull<double>(), " = 30 x^2:", x.pow(2).mul(30.0).pull<double>());
  out.writeln("restored pipeline  ", restored(quartic)({x}).pull<double>());

  // Operators that hold a function are refused, with a message that names the operator.
  try {
    std::ostringstream o(std::ios::binary);
    math::io::save(operators::multiply(quartic.arg("x")), o);
  } catch (const pluss::Error &e) {
    const std::string text(e.what());
    out.writeln("multiply:", text.substr(0, text.find('\n')));
  }
}
```

Output

```text
restored key compose  version 1
original pipeline   [30.000000, 120.000000, 270.000000]  = 30 x^2: [30.000000, 120.000000, 270.000000]
restored pipeline   [30.000000, 120.000000, 270.000000]
multiply: math::io::save(Operator): operator 'multiply' is not serializable
```

The restored pipeline is the composition of the scale and the second derivative, with the node budget it was saved with, and gives 30x² for the quartic, as the original does. The operator that holds a function is refused, with a message that names the operator.

## Custom operators

A custom operator can be saved if it is declared `serializable`, has a key, and a **loader** for that key is registered with `operators::bind`. The loader is a function from the saved payload text and version to an `Operator`. It is how a program that did not save the operator tells the library how to rebuild one, and it is what lets different versions of an operator coexist.

```cpp
bool operators::bind(std::string_view key, std::uint32_t version, Loader loader);   // Loader = function<Operator(string_view payload, uint32_t version)>
```

`bind` returns `true` the first time a key and version are registered and `false` if they already were (the first loader stays). Register loaders at program start, before loading; loading an operator whose key and version have no loader is an error that names them.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/aux/output.h>

#include <sstream>
#include <string>

namespace math = pluss::math;
namespace operators = pluss::operators;
using namespace pluss::tils;

// The operator "example.scale_by": multiplies a function by a number kept in the payload.
static operators::Operator scale_by(double factor) {
  return operators::define("example.scale_by", [factor](const math::Function &f) { return f * factor; },
                           {.version = 1, .linear = true, .serializable = true, .payload = std::to_string(factor)});
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const math::Function quartic = math::function({"x"}, [](math::Function x) { return x.pow(4); });
  const auto x = pluss::make_series({1.0, 2.0, 3.0});

  // Saving works as soon as the operator is declared serializable ...
  std::ostringstream output(std::ios::binary);
  math::io::save(scale_by(3.0), output);

  // ... but loading needs a loader for the key. Without one, the load is refused.
  try {
    std::istringstream input(output.str(), std::ios::binary);
    (void)math::io::load_operator(input);
  } catch (const pluss::Error &e) {
    const std::string text(e.what());
    out.writeln("before bind:", text.substr(0, text.find('\n')));
  }

  const bool first = operators::bind("example.scale_by", 1, [](std::string_view payload, std::uint32_t) { return scale_by(std::stod(std::string(payload))); });
  const bool second = operators::bind("example.scale_by", 1, [](std::string_view, std::uint32_t) { return scale_by(1.0); });
  out.writeln("bind returned", first, "then", second, "(the first loader is kept)");

  std::istringstream input(output.str(), std::ios::binary);
  const operators::Operator restored = math::io::load_operator(input);
  out.writeln("restored: key", restored.key(), " version", restored.version(), " serializable", restored.capabilities().serializable);
  out.writeln("restored(f)", restored(quartic)({x}).pull<double>(), " = 3 x^4:", x.pow(4).mul(3.0).pull<double>());
}
```

Output

```text
before bind: math::io: no operator loader is bound for key 'example.scale_by' version 1
bind returned true then false (the first loader is kept)
restored: key example.scale_by  version 1  serializable true
restored(f) [3.0000, 48.0000, 243.0000]  = 3 x^4: [3.0000, 48.0000, 243.0000]
```

Saving succeeded because the operator declared itself serializable and carries a payload, the factor as text. The first load fails because nothing yet says how to rebuild an operator called `example.scale_by`. After `bind`, the same bytes load into a working operator; the second `bind` for the same key and version returns false and leaves the first loader in place. The payload is whatever text the operator's author chooses; the loader parses it. A loader for a changed definition is registered under a new version number, and artifacts saved with the old version keep loading through the old loader.

<!-- apiref:toctree:start -->
<!-- apiref:toctree:end -->
