<!-- generated from defined.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: e9e47ab1c816 -->

# `math::Function::defined`

[Back to `math::Function`](../Function.md)

Reports whether a `Function` holds a formula, or is the empty default-constructed value.

## Signature

```cpp
bool math::Function::defined() const noexcept;
explicit math::Function::operator bool() const noexcept;
```

## What It Does

A default-constructed `Function` holds nothing: it is the "no function" value, the way an empty `std::function` or a null pointer is. `defined()` is `false` for it and `true` for every function built by `math::function`, `math::stack`, an operator, or arithmetic on other functions. The `explicit operator bool` is the same test spelled so that it reads naturally in an `if`.

Call it whenever a `Function` might legitimately be absent: a lookup that can miss, a slot in a struct that is filled later, the result of an optional step. Every other method of `Function`, and every operator applied to it, requires a defined function and fails with an error otherwise, so testing first lets you report the problem in your own words (or skip the step) instead of catching that error.

`defined()` says nothing about the formula itself. It does not tell you the function is valid at any particular point, only that there is one.

## Returns

`true` if the function holds a formula, `false` for a default-constructed `Function`.

## Practical Examples

### Example 1

A default-constructed function is the empty value; a function built by `math::function` is defined. `defined()` and the boolean conversion agree.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace math = pluss::math;
namespace operators = pluss::operators;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const math::Function empty;
  const math::Function f = math::function({"x"}, [](math::Function x) { return x * x; });
  out.writeln("empty.defined():", empty.defined(), "  bool(empty):", static_cast<bool>(empty));
  out.writeln("f.defined():    ", f.defined(), "  bool(f):    ", static_cast<bool>(f));
}
```

Output

```text
empty.defined(): false   bool(empty): false
f.defined():     true   bool(f):     true
```

The two spellings always agree. The conversion is `explicit`, so `bool ok = f;` does not compile, but `if (f)` and `static_cast<bool>(f)` do.

### Example 2

A table of named functions in which a lookup can miss. A miss returns an empty `Function`, and the caller tests it with `if (f)` instead of using a separate flag or an exception.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <map>
#include <string>

namespace math = pluss::math;
namespace operators = pluss::operators;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  std::map<std::string, math::Function> library;
  library["square"] = math::function({"x"}, [](math::Function x) { return x * x; });
  library["cube"] = math::function({"x"}, [](math::Function x) { return x * x * x; });

  const auto find = [&](const std::string &name) {
    const auto it = library.find(name);
    return it == library.end() ? math::Function{} : it->second;
  };

  for (const char *name : {"square", "cube", "quartic"}) {
    const math::Function f = find(name);
    if (f) {
      out.writeln(name, "at 3 =", f(3.0).item<double>());
    } else {
      out.writeln(name, "is not in the library");
    }
  }
}
```

Output

```text
square at 3 = 9.0000
cube at 3 = 27.0000
quartic is not in the library
```

`find` returns `math::Function{}` for a name that is not there. Because a `Function` is a cheap handle, returning one by value from a lookup costs a pointer copy.

### Example 3

Every other method requires a defined function. Using an undefined one is an error that names the problem; testing `defined()` first avoids it.

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
  const math::Function empty;
  const auto first_line = [](const pluss::Error &e) {
    const std::string text(e.what());
    return text.substr(0, text.find('\n'));
  };

  try { (void)empty.arity(); } catch (const pluss::Error &e) { out.writeln("arity():       ", first_line(e)); }
  try { (void)empty(1.0); } catch (const pluss::Error &e) { out.writeln("call:          ", first_line(e)); }
  try { (void)operators::differential("x")(empty); } catch (const pluss::Error &e) { out.writeln("differential:  ", first_line(e)); }
}
```

Output

```text
arity():        math::Function: operation requires a defined function
call:           math::Function: operation requires a defined function
differential:   operators::Operator: input Function must be defined
```

The accessors and the call operator report that the function must be defined; an operator applied to it reports the same about its input. All three are `pluss::Error`.

## See Also

- [`arity`](arity.md): the number of variables of a defined function
- [`math::function`](../function_factory.md): builds a defined function

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="cosh.html">
    <span class="label">Previous</span>
    <span class="title">cosh</span>
  </a>
  <a class="next-link" href="det.html">
    <span class="label">Next</span>
    <span class="title">det</span>
  </a>
</div>
<!-- apiref:nav:end -->
