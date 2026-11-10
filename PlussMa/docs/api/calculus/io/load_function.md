<!-- generated from load_function.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `math::io::load_function`

Reads a function back from a stream or a file written by `save`.

## Signature

```cpp
math::Function math::io::load_function(std::istream &input);
math::Function math::io::load_function(std::string_view path);
```

## What It Does

`load_function` reads an artifact written by [`save`](save.md) and returns an equivalent `Function`: the same variables, structure, constants and assumptions, and the same values when evaluated. Open streams in binary mode. The artifact carries a header naming its kind and a schema version, and a checksum; a damaged, truncated or foreign file, one of a different schema, or an operator artifact is rejected with an error, and no partly built object is ever returned.

A loaded function has its own variable placeholders, as if it had come from a separate `math::function(...)` call. It can be evaluated, differentiated, integrated, transformed by operators and saved again, but it cannot be added to or multiplied with the function it was saved from or with any other independent function (they belong to different traces); combine them with [`compose`](../functions/Function/compose.md).

## Parameters

`std::istream &input`
: A stream opened in binary mode, positioned at the start of an artifact.

`std::string_view path`
: The path of a file written by `save`.

## Returns

The restored `math::Function`.

## Errors

- A damaged, truncated, empty or foreign artifact is rejected (checksum, header and schema are checked).
- An operator artifact loaded as a function is rejected.
- A path that cannot be opened raises an error.

## Practical Examples

### Example 1

A round trip through a file. The loaded function evaluates identically and its provenance is `Structural`.

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
  out.print_defaults(PrintOptions{}.fixed(6));
  const math::Function f = math::function({"x", "y"}, [](math::Function x, math::Function y) { return (x * y).exp() - x; });
  const std::string path = (std::filesystem::temp_directory_path() / "calculus_reference_load.bin").string();
  math::io::save(f, path);
  const math::Function loaded = math::io::load_function(path);
  const auto x = pluss::make_series({0.5, 1.0}), y = pluss::make_series({1.0, 2.0});
  out.writeln("original:", f({x, y}).items<double>());
  out.writeln("loaded:  ", loaded({x, y}).items<double>(), "  Structural:", loaded.provenance() == math::Provenance::Structural);
  std::filesystem::remove(path);
}
```

Output

```text
original: [1.148721, 6.389056]
loaded:   [1.148721, 6.389056]   Structural: true
```

### Example 2

A loaded function is a new trace. It can be differentiated and evaluated, but combining it directly with the function it came from is refused; `compose` substitutes one into the other.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <sstream>
#include <string>

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
  out.print_defaults(PrintOptions{}.fixed(4));
  const auto first_line = [](const pluss::Error &e) {
    const std::string text(e.what());
    return text.substr(0, text.find('\n'));
  };

  const math::Function f = math::function({"x"}, [](math::Function x) { return x * x + 1.0; });
  const math::Function loaded = through_a_stream(f);
  out.writeln("derivative of the loaded function at 3:", operators::differential("x")(loaded)(3.0).item<double>());
  try { (void)(f + loaded); } catch (const pluss::Error &e) { out.writeln("f + loaded:", first_line(e)); }
  const math::Function composed = loaded.compose({f});
  out.writeln("loaded(f(x)) at 2 =", composed(2.0).item<double>(), "  ((2^2 + 1)^2 + 1 = 26)");
}
```

Output

```text
derivative of the loaded function at 3: 6.0000
f + loaded: math::Function: expressions from different variable traces cannot be combined directly
loaded(f(x)) at 2 = 26.0000   ((2^2 + 1)^2 + 1 = 26)
```

### Example 3

Damaged input is rejected, never half-loaded. An intact artifact loads; a flipped last byte (checksum), a truncated stream, an empty stream, plain text and an operator artifact each fail with a message.

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

  std::ostringstream output(std::ios::binary);
  math::io::save(math::function({"x"}, [](math::Function x) { return x * x; }), output);
  const std::string good = output.str();
  const auto attempt = [&](const char *label, const std::string &bytes) {
    std::istringstream in(bytes, std::ios::binary);
    try { (void)math::io::load_function(in); out.writeln(label, "-> loaded"); } catch (const pluss::Error &e) { out.writeln(label, "->", first_line(e)); }
  };
  std::string flipped = good;
  flipped.back() ^= 0x1;
  attempt("intact              ", good);
  attempt("last byte altered   ", flipped);
  attempt("truncated           ", good.substr(0, good.size() / 2));
  attempt("empty               ", std::string());
  attempt("plain text          ", "this is not an artifact");
  std::ostringstream op_bytes(std::ios::binary);
  math::io::save(operators::scale(2.0), op_bytes);
  attempt("an operator artifact", op_bytes.str());
}
```

Output

```text
intact               -> loaded
last byte altered    -> math::io: checksum mismatch; artifact is corrupted
truncated            -> math::io: corrupted length-prefixed field
empty                -> math::io: artifact is truncated or corrupted
plain text           -> math::io: corrupted length-prefixed field
an operator artifact -> math::io: invalid artifact magic
```

### Example 4

A path that does not exist is an error too.

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

  try { (void)math::io::load_function("/this/path/does/not/exist.bin"); } catch (const pluss::Error &e) { out.writeln(first_line(e)); }
}
```

Output

```text
math::io::load: could not open input path
```

## See Also

- [`save`](save.md): writes the artifact
- [`load_operator`](load_operator.md): reads an operator
- [`compose`](../functions/Function/compose.md): combine a loaded function with another

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="next-link" href="load_operator.html">
    <span class="label">Next</span>
    <span class="title">load_operator</span>
  </a>
</div>
<!-- apiref:nav:end -->
