<!-- generated from composing_and_combining.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Composing and combining operators

[Back to Operators](../calculus_05_operators.md)

Operators form an algebra. They can be composed (one after another), added with coefficients (in parallel), and combined into the commutator that measures how much two of them fail to commute. Each combination is itself an `Operator`, so the results can be stored, applied, composed again and saved like any other.

```cpp
operators::Operator operators::compose(const Operator &outer, const Operator &inner);
operators::Operator operators::linear_combination(std::initializer_list<Term> terms);                  // Term = pair<double, Operator>
operators::Operator operators::linear_combination(RangeView<Operator> operators, RangeView<double> coefficients);
operators::Operator operators::commutator(const Operator &left, const Operator &right);
```

| Combination | Result for `f` |
|---|---|
| `compose(A, B)` | `A(B(f))`: **the inner operator acts first** |
| `linear_combination({{a, A}, {b, B}})` | `a·A(f) + b·B(f)` |
| `commutator(A, B)` | `A(B(f)) − B(A(f))` |

Composition flattens: composing a composition produces one chain, not nested wrappers. A composition is linear when every part is, and a linear combination of linear operators is linear.

## Composition order

`compose(outer, inner)` reads like function composition. The inner operator is applied to `f`, and the outer one to the result. The order matters for most pairs of operators and not for some: scaling and shifting commute, shifting and multiplying by the coordinate do not.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/auxiliary/output.h>

namespace math = pluss::math;
namespace operators = pluss::operators;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const math::Function square = math::function({"x"}, [](math::Function x) { return x.pow(2); });
  const auto x = pluss::make_series({0.0, 1.0, 2.0});

  const operators::Operator shift = operators::shift("x", 2.0);
  const operators::Operator twice = operators::scale(2.0);
  const operators::Operator times_x = operators::multiply(square.arg("x"));
  const operators::Operator d = operators::differential("x");

  // Scaling and shifting commute.
  out.writeln("scale after shift    ", operators::compose(twice, shift)(square)({x}).pull<double>(), " = 2 (x+2)^2:", x.add(2.0).pow(2).mul(2.0).pull<double>());
  out.writeln("shift after scale    ", operators::compose(shift, twice)(square)({x}).pull<double>());

  // Shifting and multiplying by x do not: the inner operator acts first.
  out.writeln("shift after times_x  ", operators::compose(shift, times_x)(square)({x}).pull<double>(), " = (x+2)^3:", x.add(2.0).pow(3).pull<double>());
  out.writeln("times_x after shift  ", operators::compose(times_x, shift)(square)({x}).pull<double>(), " = x (x+2)^2:", x.mul(x.add(2.0).pow(2)).pull<double>());

  // A chain of three, applied inside out: shift, then scale, then negate.
  const operators::Operator chain = operators::compose(operators::scale(-1.0), operators::compose(twice, shift));
  out.writeln("-(2 (x+2)^2)         ", chain(square)({x}).pull<double>());

  // Differentiation commutes with shifting x.
  out.writeln("d after shift        ", operators::compose(d, shift)(square)({x}).pull<double>(), " = 2 (x+2):", x.add(2.0).mul(2.0).pull<double>());
  out.writeln("shift after d        ", operators::compose(shift, d)(square)({x}).pull<double>());
}
```

Output

```text
scale after shift     [8.0000, 18.0000, 32.0000]  = 2 (x+2)^2: [8.0000, 18.0000, 32.0000]
shift after scale     [8.0000, 18.0000, 32.0000]
shift after times_x   [8.0000, 27.0000, 64.0000]  = (x+2)^3: [8.0000, 27.0000, 64.0000]
times_x after shift   [0.0000, 9.0000, 32.0000]  = x (x+2)^2: [0.0000, 9.0000, 32.0000]
-(2 (x+2)^2)          [-8.0000, -18.0000, -32.0000]
d after shift         [4.0000, 6.0000, 8.0000]  = 2 (x+2): [4.0000, 6.0000, 8.0000]
shift after d         [4.0000, 6.0000, 8.0000]
```

Scaling and shifting give the same function in either order. Shifting and multiplying by the coordinate do not: the inner operator acts first, so `shift` after `times_x` is (x + 2)³ (the product x·x² is formed, then shifted), and `times_x` after `shift` is x (x + 2)² (the shift is made, then the product is formed). The three-operator chain reads from the inside out, and differentiation and shifting commute: the derivative of the shifted square is 2(x + 2), whichever is applied first.

## Linear combinations

A linear combination applies several operators to the same function and adds the results with the given coefficients. The terms are `{coefficient, operator}` pairs. The coefficient list must be as long as the operator list when the two are given as separate ranges, and an empty combination is an error.

A differential operator written this way reads like the equation it stands for. For example, the operator L = d² − 4 maps f to f″ − 4f, and the solutions of L f = 0 are the multiples of e^{2x} and e^{−2x}:

```cpp
#include <pluss/ma.h>
#include <pluss/tils/auxiliary/output.h>

#include <cmath>

namespace math = pluss::math;
namespace operators = pluss::operators;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6));
  const operators::Operator d = operators::differential("x");
  const operators::Operator dd = operators::compose(d, d);
  const operators::Operator L = operators::linear_combination({{1.0, dd}, {-4.0, operators::identity()}});
  out.writeln("L is linear:", L.capabilities().linear ? "yes" : "no");

  const auto x = pluss::make_series({-1.0, 0.0, 0.5, 2.0});
  const math::Function square = math::function({"x"}, [](math::Function v) { return v.pow(2); });
  out.writeln("L(x^2)      ", L(square)({x}).pull<double>(), " = 2 - 4 x^2:", x.pow(2).mul(-4.0).add(2.0).pull<double>());

  const math::Function growth = math::function({"x"}, [](math::Function v) { return (v * 2.0).exp(); });
  const math::Function decay = math::function({"x"}, [](math::Function v) { return (v * -2.0).exp(); });
  out.writeln("L(exp(2x))  ", L(growth)({x}).pull<double>());
  out.writeln("L(exp(-2x)) ", L(decay)({x}).pull<double>());

  // The parallel form with separate ranges.
  std::vector<operators::Operator> parts{d, operators::identity()};
  std::vector<double> weights{1.0, 3.0};
  const operators::Operator mixed = operators::linear_combination(pluss::RangeView<operators::Operator>(parts), pluss::RangeView<double>(weights));
  out.writeln("(d + 3) x^2 ", mixed(square)({x}).pull<double>(), " = 2x + 3x^2:", x.mul(2.0).add(x.pow(2).mul(3.0)).pull<double>());
}
```

Output

```text
L is linear: yes
L(x^2)       [-2.000000, 2.000000, 1.000000, -14.000000]  = 2 - 4 x^2: [-2.000000, 2.000000, 1.000000, -14.000000]
L(exp(2x))   [0.000000, 0.000000, 0.000000, 0.000000]
L(exp(-2x))  [0.000000, 0.000000, 0.000000, 0.000000]
(d + 3) x^2  [1.000000, 0.000000, 1.750000, 16.000000]  = 2x + 3x^2: [1.000000, 0.000000, 1.750000, 16.000000]
```

The operator `L` is linear, as the capabilities report. It maps x² to 2 − 4x², and it annihilates both exponentials e^{2x} and e^{−2x} (to exactly zero at all four points), because their second derivative is four times themselves. The parallel form with separate ranges gives the same kind of operator, here d + 3: it maps x² to 2x + 3x², which the by-hand line confirms.

## The commutator

`commutator(A, B)` is A∘B − B∘A. It is zero exactly when the operators commute, and it is how one asks how far they are from doing so. The classic case is differentiation and multiplication by the coordinate, whose commutator is the identity: (xf)′ − x f′ = f.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/auxiliary/output.h>

namespace math = pluss::math;
namespace operators = pluss::operators;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const math::Function f = math::function({"x"}, [](math::Function x) { return x.pow(3) + x.sin(); });
  const auto x = pluss::make_series({0.5, 1.0, 2.0});

  const operators::Operator d = operators::differential("x");
  const operators::Operator times_x = operators::multiply(f.arg("x"));
  const operators::Operator bracket = operators::commutator(d, times_x);
  out.writeln("[d, x] f            ", bracket(f)({x}).pull<double>());
  out.writeln("f                   ", f({x}).pull<double>());

  // Shifting a function and multiplying by x: [T_1, M_x] f = T_1 f, as the shift moves the factor x too.
  const operators::Operator shift = operators::shift("x", 1.0);
  const math::Function square = math::function({"x"}, [](math::Function v) { return v.pow(2); });
  const operators::Operator shift_bracket = operators::commutator(shift, operators::multiply(square.arg("x")));
  out.writeln("[T, x] f            ", shift_bracket(square)({x}).pull<double>(), " = T f:", x.add(1.0).pow(2).pull<double>());

  // Operators that commute give zero.
  const operators::Operator zero = operators::commutator(operators::scale(2.0), operators::shift("x", 1.0));
  out.writeln("[scale, shift] f    ", zero(square)({x}).pull<double>());
  out.writeln("commutator is linear:", bracket.capabilities().linear ? "yes" : "no");
}
```

Output

```text
[d, x] f             [0.6044, 1.8415, 8.9093]
f                    [0.6044, 1.8415, 8.9093]
[T, x] f             [2.2500, 4.0000, 9.0000]  = T f: [2.2500, 4.0000, 9.0000]
[scale, shift] f     [0.0000, 0.0000, 0.0000]
commutator is linear: yes
```

The commutator of differentiation and multiplication by x returns the function itself, which is the identity [d, x] = 1 applied to f; the two rows are equal. The commutator of a shift with multiplication by x is the shift itself. The commutator of scaling and shifting vanishes, since they commute. The commutator of linear operators is linear, and its capabilities say so.

<!-- apiref:toctree:start -->
<!-- apiref:toctree:end -->
