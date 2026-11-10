<!-- generated from TensorSignature.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `operators::geometry::TensorSignature`

How many contravariant (upper) and covariant (lower) indices a tensor field has.

A tensor field is a `math::Function` whose output axes are the tensor's indices. The geometry operators need to know which of those axes are upper (contravariant) and which are lower (covariant), because the connection correction has the opposite sign for the two kinds. `TensorSignature` is that statement: a pair of counts, written `{contravariant, covariant}`. The convention is fixed throughout the library: **contravariant axes come first, then covariant axes.**

| Signature | Field | Example |
|---|---|---|
| `{0, 0}` | scalar | a temperature, `T` |
| `{1, 0}` | vector | a velocity, `Vⁱ` |
| `{0, 1}` | covector (one-form) | a gradient, `ω_j` |
| `{0, 2}` | rank-2 covariant tensor | the metric, `g_ij` |
| `{1, 1}` | mixed tensor | the identity, `δⁱ_j` |

The field's output rank must equal `contravariant + covariant`; an operator given a field whose rank differs is refused.

## Fields

| Field | Type | Default | Meaning |
|---|---|---|---|
| `contravariant` | `std::int64_t` | `0` | The number of upper indices; these are the leading output axes. |
| `covariant` | `std::int64_t` | `0` | The number of lower indices; these follow the contravariant axes. |

## Guidance

Use the signature that matches how the field transforms, not merely how many axes it has. A wrong signature gives a wrong derivative without an error when the rank happens to match.

## Practical Examples

### Example 1

The output rank follows the signature: a scalar has rank 0, a vector 1, a covector 1, the metric 2, a mixed tensor 2. The same rank can belong to different signatures (`{1, 0}` and `{0, 1}`), which is why the signature has to be stated.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <tuple>
#include <vector>

namespace math = pluss::math;
namespace operators = pluss::operators;
using namespace pluss::tils;

namespace geometry = pluss::operators::geometry;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const math::Function metric = math::function({"theta", "phi"}, [](math::Function theta, math::Function) {
    const math::Function one = theta * 0.0 + 1.0, zero = theta * 0.0, s = theta.sin() * theta.sin();
    return math::stack({math::stack({one, zero}), math::stack({zero, s})});   // diag(1, sin^2 theta)
  });
  const math::Function gamma = operators::geometry::levi_civita(metric);
  const auto point = {pluss::make_series(1.0), pluss::make_series(0.3)};   // theta = 1, phi = 0.3

  const math::Function scalar = math::function({"theta", "phi"}, [](math::Function theta, math::Function phi) { return theta * phi; });
  const math::Function vector = math::function({"theta", "phi"}, [](math::Function theta, math::Function) { return math::stack({theta, theta * 0.0}); });
  for (const auto &[label, field, signature] : std::vector<std::tuple<const char *, math::Function, operators::geometry::TensorSignature>>{
           {"scalar {0, 0}", scalar, {0, 0}}, {"vector {1, 0}", vector, {1, 0}}, {"covector {0, 1}", vector, {0, 1}}, {"metric {0, 2}", metric, {0, 2}}}) {
    out.writeln(label, ": rank", field.rank(), " = ", signature.contravariant + signature.covariant);
  }
}
```

Output

```text
scalar {0, 0} : rank 0  =  0
vector {1, 0} : rank 1  =  1
covector {0, 1} : rank 1  =  1
metric {0, 2} : rank 2  =  2
```

### Example 2

The same components, read as a vector or as a covector, are differentiated differently. For `(0, 1)` on the sphere: the vector gets `+Γ^θ_φφ = −sin θ cos θ` and the covector gets `−Γ^φ_θφ = −cot θ`.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace math = pluss::math;
namespace operators = pluss::operators;
using namespace pluss::tils;

namespace geometry = pluss::operators::geometry;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5));
  const math::Function metric = math::function({"theta", "phi"}, [](math::Function theta, math::Function) {
    const math::Function one = theta * 0.0 + 1.0, zero = theta * 0.0, s = theta.sin() * theta.sin();
    return math::stack({math::stack({one, zero}), math::stack({zero, s})});   // diag(1, sin^2 theta)
  });
  const math::Function gamma = operators::geometry::levi_civita(metric);
  const auto point = {pluss::make_series(1.0), pluss::make_series(0.3)};   // theta = 1, phi = 0.3

  const math::Function field = math::function({"theta", "phi"}, [](math::Function theta, math::Function) { return math::stack({theta * 0.0, theta * 0.0 + 1.0}); });
  out.writeln("as a vector  {1, 0}:", operators::geometry::covariant("phi", gamma, {1, 0})(field)(point).items<double>());
  out.writeln("as a covector {0, 1}:", operators::geometry::covariant("phi", gamma, {0, 1})(field)(point).items<double>());
}
```

Output

```text
as a vector  {1, 0}: [-0.45465, 0.00000]
as a covector {0, 1}: [-0.64209, 0.00000]
```

### Example 3

The default signature of `absolute_differential` is a vector, `{1, 0}`. Writing it out gives the same operator; the signature `{0, 1}` changes the result.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace math = pluss::math;
namespace operators = pluss::operators;
using namespace pluss::tils;

namespace geometry = pluss::operators::geometry;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5));
  const math::Function metric = math::function({"theta", "phi"}, [](math::Function theta, math::Function) {
    const math::Function one = theta * 0.0 + 1.0, zero = theta * 0.0, s = theta.sin() * theta.sin();
    return math::stack({math::stack({one, zero}), math::stack({zero, s})});   // diag(1, sin^2 theta)
  });
  const math::Function gamma = operators::geometry::levi_civita(metric);
  const auto point = {pluss::make_series(1.0), pluss::make_series(0.3)};   // theta = 1, phi = 0.3

  const math::Function latitude = math::function({"t"}, [](math::Function t) { return math::stack({t * 0.0 + 1.0, t}); });
  const math::Function field = math::function({"t"}, [](math::Function t) { return math::stack({t * 0.0, t * 0.0 + 1.0}); });
  out.writeln("default:   ", operators::geometry::absolute_differential(latitude, gamma, "t")(field)(0.7).items<double>());
  out.writeln("{1, 0}:    ", operators::geometry::absolute_differential(latitude, gamma, "t", {1, 0})(field)(0.7).items<double>());
  out.writeln("{0, 1}:    ", operators::geometry::absolute_differential(latitude, gamma, "t", {0, 1})(field)(0.7).items<double>());
}
```

Output

```text
default:    [-0.45465, 0.00000]
{1, 0}:     [-0.45465, 0.00000]
{0, 1}:     [-0.64209, 0.00000]
```

## See Also

- [`covariant`](covariant.md): takes a signature
- [`absolute_differential`](absolute_differential.md): takes a signature
- [`levi_civita`](levi_civita.md): the connection the signature is used with

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="next-link" href="absolute_differential.html">
    <span class="label">Next</span>
    <span class="title">absolute_differential</span>
  </a>
</div>
<!-- apiref:nav:end -->
