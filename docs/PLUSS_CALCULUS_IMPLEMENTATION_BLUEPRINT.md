# Pluss Function And Operator Implementation Blueprint

## Status

This document is the normative implementation contract for reusable
mathematical functions and function operators in Pluss.

The public mental model contains exactly two core classes:

```text
pluss::math::Function
pluss::operators::Operator
```

Everything else is a factory function, option structure, enum, or private
implementation type.

The implementation must not reintroduce public specialized classes for
variables, expressions, gradients, integrals, differential operators, or
function output ranks.

## Namespace Contract

`pluss::math` already aliases `pluss::subsp::math`. It owns reusable
mathematical functions and their evaluation:

```text
pluss::math::Function
pluss::math::function(...)
```

The canonical operator implementation namespace is:

```text
pluss::subsp::math::operators
```

It is exposed publicly as:

```cpp
namespace pluss {
namespace operators = ::pluss::subsp::math::operators;
}
```

Only supported public contracts belong directly in
`pluss::subsp::math::operators`. Internal nodes, registries, caches, concepts,
and kernels belong under `pluss::subsp::math::operators::detail`.

Users may shorten the public namespaces locally:

```cpp
namespace math = pluss::math;
namespace operators = pluss::operators;
```

There are no root aliases such as `pluss::Function`, `pluss::Operator`,
`pluss::Differential`, or `pluss::Integral`.

## Non-Negotiable Public Surface

The following names must not become public classes:

```text
math::Variable
math::Expression
math::FunctionVector
math::FunctionMatrix

operators::Differential
operators::Partial
operators::Gradient
operators::Jacobian
operators::Hessian
operators::Integral
operators::Shift
operators::Scale
operators::Multiplication
operators::Builder
```

Standard operator factories all return the same class:

```cpp
const operators::Operator Dx = operators::differential("x");
const operators::Operator Dx2 = operators::differential("x", 2);
const operators::Operator grad = operators::gradient({"x", "y"});
const operators::Operator hess = operators::hessian({"x", "y"});
const operators::Operator Ix = operators::integral("x", 0.0);
const operators::Operator shifted = operators::shift("x", 1.0);
```

Applying any operator returns the same function class:

```cpp
const math::Function dfdx = Dx(f);
const math::Function d2fdx2 = Dx2(f);
const math::Function gradient_f = grad(f);
const math::Function hessian_f = hess(f);
const math::Function integral_f = Ix(f);
const math::Function shifted_f = shifted(f);
```

`math::Function` carries arbitrary Series tensor output shape internally.
Evaluation returns a `Series` with the corresponding output and batch axes.

## Mathematical Model

A function transforms values:

```text
f : values -> values
```

An operator transforms functions:

```text
L : functions -> functions
```

Differentiation, integration, shifting, multiplication, scaling, Fourier
transformation, convolution, projection, and expectation can all be expressed
as operators on functions.

An evaluated `Series::calc` operation remains different:

```text
operators::differential("x")(f)  -> reusable structural Function
x.calc.diff(callback)             -> derivative values at x
```

The new operator system must not become cosmetic syntax that simply reruns
eager autodiff whenever a transformed function is evaluated.

## Function Construction

`math::Function` is a concrete public value type. It is not an abstract base
class and users do not derive from it. Internally it is a small immutable
handle to a private DAG representation, comparable to how `Series` is a
concrete handle to its numerical payload.

Function construction uses the free `math::function(...)` factory rather than
a public templated constructor. This is conceptually the same pattern as a
Series factory: validate and normalize flexible inputs in a free factory, then
return one stable concrete value type.

```cpp
const pluss::Series values = pluss::make_series({1.0, 2.0, 3.0});

const pluss::math::Function square = pluss::math::function(
    {"x"},
    [](pluss::math::Function x) {
      return x.pow(2);
    });
```

The factory is named `function(...)`, not a static factory method such as
`Function::make(...)`. Its job is to create projection Functions, trace the
lambda once, validate arity and names, seal the private graph, and return a
normal `Function` object.

`function(...)` is the only public Function factory name. Its overload family
may construct:

```text
a named fixed-arity lambda Function
a named runtime-arity span-lambda Function
a zero-variable scalar constant Function
a zero-variable Series constant Function
```

These are overloads of one factory, not additional public names such as
`variable(...)`, `expression(...)`, `constant(...)`, or `vector_function(...)`.
Deserialization belongs to the repository's IO surface rather than becoming a
second math factory.

### Normal Lambda Form

Users construct functions with named variables and a lambda. Public variable
and expression classes are unnecessary.

For:

```text
f(x, y) = x^2 sin(xy) exp(cos(x))
```

the public construction is:

```cpp
#include <pluss/ma.h>

namespace math = pluss::math;

const math::Function f = math::function(
    {"x", "y"},
    [](math::Function x, math::Function y) {
      return x.pow(2)
          * (x * y).sin()
          * x.cos().exp();
    });
```

During construction, the lambda arguments are lightweight projection
functions. Their mathematical methods build private expression nodes. The
lambda is traced once; it is not rerun for every data value.

The `Function` class is a small immutable handle, so passing these projection
functions by value is intentional.

### Callable Forms And Ownership

`math::function(...)` accepts normal C++ callables regardless of whether the
callable is named before the call or written inline. The public overload is a
forwarding template conceptually equivalent to:

```cpp
template <class Callable>
math::Function math::function(NameRange names, Callable&& callable);
```

Consequently, all of these are valid construction forms:

```cpp
// Named lvalue lambda.
const auto formula = [](math::Function x, math::Function y) {
  return x.pow(2) * (x * y).sin();
};
const math::Function f1 = math::function({"x", "y"}, formula);

// Inline rvalue lambda.
const math::Function f2 = math::function(
    {"x", "y"},
    [](math::Function x, math::Function y) {
      return x.pow(2) * (x * y).sin();
    });

// User-defined function object.
struct Formula final {
  math::Function operator()(math::Function x, math::Function y) const {
    return x.pow(2) * (x * y).sin();
  }
};
const Formula user_formula{};
const math::Function f3 = math::function({"x", "y"}, user_formula);

// Free function or function pointer.
const math::Function f4 = math::function({"x", "y"}, &named_formula);
```

An appropriately typed `std::function` and a move-only callable are accepted
as well. Construction invokes the callable once with projection Functions and
stores the resulting immutable graph; it does not retain the callable and does
not rerun host C++ every time the resulting Function is evaluated. Captured
scalars or Series become constants in that graph.

This tracing rule has one important consequence: ordinary host control flow is
resolved once while the graph is built. A branch whose condition depends on a
Function value must use a structural conditional supplied by the math surface;
it cannot use a C++ `if` that expects a concrete `bool`.

### Why `Function` Remains Public

The lambda removes the need for public `Variable` and `Expression` classes,
but it cannot remove `Function`. A transformed callable needs a stable named
object carrying:

- its private expression DAG
- variable names, IDs, and ordering
- arbitrary Series tensor output shape
- domain and assumption metadata
- exact, special-function, or numerical form metadata
- simplification state
- serialization schema
- evaluation plan and caches

Returning unnamed template closures would force `auto`, prevent a stable ABI,
complicate heterogeneous storage, and undermine serialization.

### Arbitrary Variable Count

There is no `x/y/z/w` limit. Internally, a `Function` stores a dynamic variable
table:

```text
names: dynamic ordered strings
IDs: compact integer IDs
roots: one or more private DAG nodes
```

The normal lambda overload infers arity from a non-generic lambda and verifies
that it matches the supplied names:

```cpp
const math::Function f = math::function(
    {"x", "y", "z", "temperature"},
    [](math::Function x,
       math::Function y,
       math::Function z,
       math::Function temperature) {
      return x * y + z / temperature;
    });
```

For a runtime-sized variable set, an overload accepts a span:

```cpp
const std::vector<std::string> names = load_variable_names();

const math::Function f = math::function(
    names,
    [](std::span<const math::Function> variables) {
      return variables[0] + variables[1];
    });
```

The practical limits are graph memory and useful model size, not an API arity
cap.

### Evaluation

Named bindings preserve clarity for arbitrary-dimensional functions:

```cpp
const pluss::Series values = f({
    {"x", pluss::make_series({1.0, 2.0, 3.0})},
    {"y", pluss::make_series(0.5)},
});
```

The evaluator validates missing, duplicate, and unknown names. Series
broadcasting determines the resulting batch shape.

An ordered overload may exist for performance-sensitive code when the caller
already has the function's canonical variable order.

## Function Method Surface

`math::Function` mirrors only the deterministic, side-effect-free Series
operations that have structural mathematical meaning:

```cpp
const math::Function f = math::function(
    {"x"},
    [](math::Function x) {
      return x.pow(2).add(x.exp()).log();
    });
```

The numerical implementation is never duplicated:

```text
Function::log() records a private Log node.
Series::log() evaluates the existing CPU or MPS kernel.
Evaluating a Log node delegates to Series::log().
```

There is no duplicate `math::log(series)` public API.

The initial structural subset includes:

```text
+ - * / unary-
pow
exp log log1p expm1
sqrt
sin cos tan
sinh cosh tanh
abs
```

Mutation, random generation, device transfer, host extraction, persistence,
display, and metadata operations do not belong on `Function`.

Reductions, matrix operations, indexing, and shape transforms are added only
when their tensor-function and derivative-axis semantics are defined. They are
not copied mechanically from Series.

One internal primitive registry defines:

```text
stable operation ID
Function method generation metadata
Series evaluation dispatch
arity and argument kinds
local derivative rules
domain constraints
safe simplifications
serialization support
```

## Operator Class

`operators::Operator` is an immutable value type representing a transformation
from `math::Function` to `math::Function`:

```cpp
const math::Function output = op(input);
```

Scalar-valued functionals still fit this model by returning a zero-variable,
scalar-output `Function`. Definite integration over one variable returns a
function of every remaining free variable.

Internally, `Operator` uses type erasure over a private abstract concept. The
public API does not expose inheritance, cloning, `unique_ptr`, or concrete
operator subclasses.

In other words, the public `Operator` itself is concrete. The abstract class
exists only as an implementation mechanism under `operators::detail`:

```text
public:   operators::Operator                concrete value handle
private:  operators::detail::OperatorConcept abstract type-erasure interface
private:  operators::detail::*Model          concrete implementations
```

Built-in factories such as `differential(...)` and `integral(...)` return the
same concrete `Operator` type as `define(...)`. Users never choose, name, or
inherit from the private implementations.

The private concept provides operations such as:

```text
apply(Function) -> Function
metadata
capabilities
serialization
compatibility
```

## Standard Operator Factories

The initial factories are:

```text
identity()
differential(variable, order = 1)
gradient(variables = all)
jacobian(variables = all)
hessian(variables = all)
antiderivative(variable)
integral(variable, lower)
integral(variable, lower, upper)
shift(variable, amount)
multiply(Function coefficient)
scale(double coefficient)
compose(outer, inner)
linear_combination(...)
commutator(left, right)
```

Every factory returns `operators::Operator`.

The initial public operator inventory has fourteen factory names. Thirteen
produce built-in operators:

```text
identity
differential
gradient
jacobian
hessian
antiderivative
integral
shift
multiply
scale
compose
linear_combination
commutator
```

The fourteenth factory, `define`, creates a user operator:

```text
define
```

Overloads for derivative order, variable subsets, bounds, and options do not
create additional factory names. New built-in names require a demonstrated
use case and an explicit public-surface review.

Future additions may include Fourier, Laplace, convolution, expectation,
projection, restriction, extension, finite-difference, and integral-kernel
operators. The geometric, stochastic, measure-theoretic, variational,
fractional and transform families are specified in the *Extension Programme*
below; `absolute_differential` there is the tensor absolute differential and
the `|f^(n)|` operator of the initial inventory is renamed
`magnitude_differential`. Their inclusion must be driven by real requirements rather than an
attempt to populate the namespace speculatively.

## Differentiation

### First Derivative

```cpp
const operators::Operator Dx =
    operators::differential("x");

const math::Function dfdx = Dx(f);
```

For the running example, the structural result is equivalent to:

```text
exp(cos(x)) *
(
  2x sin(xy)
  + x^2 y cos(xy)
  - x^2 sin(x) sin(xy)
)
```

A valid factored form is:

```text
x exp(cos(x)) *
(
  (2 - x sin(x)) sin(xy)
  + xy cos(xy)
)
```

### Higher Derivatives

The canonical higher-order API is direct:

```cpp
const operators::Operator Dx2 =
    operators::differential("x", 2);

const math::Function d2fdx2 = Dx2(f);
```

This is preferred over composing a first derivative with itself because it:

- communicates derivative order directly
- permits one `(node, variable, order)` cache lookup
- avoids unnecessary intermediate operator composition
- enables optimized higher-order transformation
- scales naturally to arbitrary derivative orders

Composition remains for genuinely distinct operators:

```cpp
const operators::Operator shifted_derivative =
    operators::compose(
        operators::shift("x", 1.0),
        operators::differential("x"));
```

### Gradient, Jacobian And Hessian

These are factories, not classes:

```cpp
const operators::Operator gradient =
    operators::gradient({"x", "y"});

const operators::Operator hessian =
    operators::hessian({"x", "y"});

const math::Function gradient_f = gradient(f);
const math::Function hessian_f = hessian(f);
```

`gradient_f` and `hessian_f` are ordinary `Function` objects carrying vector
and matrix output shape. Their evaluation returns correspondingly shaped
Series.

An empty variable selector means all variables in canonical order:

```cpp
const operators::Operator full_gradient = operators::gradient();
```

### Differentiation Scalability

The engine defines rules for primitive operations, not every possible function
composition.

For:

```text
f(x) = sin(exp(x^2 + log(x)))
```

it recursively applies:

```text
d(sin(u)) = cos(u) du
d(exp(v)) = exp(v) dv
d(a + b)  = da + db
d(x^2)    = 2x
d(log(x)) = 1/x
```

For a primitive operation:

```text
z = op(u1, ..., un)
```

the generic chain rule is:

```text
dz/dx = sum_i (partial op / partial ui) * (dui/dx)
```

Adding a primitive requires only its local derivative rules, evaluation
dispatch, domain, simplifications, and serialization ID.

Transformations are memoized by:

```text
(node identity, variable ID, derivative order)
```

## Integration

Integration cannot be complete through local composition rules. Some
elementary-looking functions have no elementary antiderivative.

The integration strategy is ordered:

```text
1. exact primitive rules
2. bounded structural patterns
3. supported special functions
4. unresolved Integral node evaluated numerically
```

### Anchored Integral

```cpp
const operators::Operator Ix =
    operators::integral("x", 0.0);

const math::Function F = Ix(f);
```

For the running example, `F` represents:

```text
F(u, y) = integral from 0 to u of
          x^2 sin(xy) exp(cos(x)) dx
```

No useful elementary antiderivative exists for general `y`, so `F` retains an
internal integral node. Evaluating it delegates to the existing Series-native
quadrature implementation.

### Definite Integral

```cpp
const operators::Operator Iab =
    operators::integral("x", 0.0, 2.0);

const math::Function area_by_y = Iab(f);
```

Integrating out `x` still leaves a function of `y`. Only after the remaining
variables are bound does evaluation return a Series:

```cpp
const pluss::Series areas = area_by_y({
    {"y", pluss::make_series({0.25, 0.5, 1.0})},
});
```

### Exact Symmetric Simplification

The running integrand is odd in `x`:

```text
x^2          even
sin(xy)      odd in x
exp(cos(x))  even
product      odd
```

Therefore its integral over any finite symmetric interval is exactly zero.
The structural integrator may detect this parity without quadrature.

Over the whole real line, the original function is not Lebesgue-integrable:
the polynomial amplitude is not damped. Its symmetric Cauchy principal value
is zero, but that is not a Lebesgue integral. The API must never silently
substitute one notion for the other.

An explicitly damped related function can be absolutely integrable:

```cpp
const math::Function damped = math::function(
    {"x", "y"},
    [](math::Function x, math::Function y) {
      return x.pow(2)
          * (x * y).sin()
          * x.cos().exp()
          * (-0.5 * x.pow(2)).exp();
    });
```

The library must not add damping automatically because that changes the
function supplied by the user.

### Fundamental Theorem

An unresolved anchored integral can still differentiate structurally:

```text
F(x) = integral from a to x of f(t) dt
F'(x) = f(x)
```

Variable bounds use the Leibniz rule. This avoids numerically differentiating
quadrature results when the structural theorem applies.

## Operator Algebra

### Identity, Shift, Scale And Multiplication

```cpp
const operators::Operator identity = operators::identity();
const operators::Operator shift_x = operators::shift("x", 1.0);
const operators::Operator twice = operators::scale(2.0);
const operators::Operator multiply_x =
    operators::multiply(f.arg("x"));
```

### Composition

For:

```cpp
const operators::Operator composed =
    operators::compose(outer, inner);
```

the meaning is:

```text
composed(f) = outer(inner(f))
```

Nested composition is flattened internally. Capability metadata is preserved
conservatively.

### Laplacian

```cpp
const operators::Operator laplacian =
    operators::linear_combination({
        {1.0, operators::differential("x", 2)},
        {1.0, operators::differential("y", 2)},
    });

const math::Function laplacian_f = laplacian(f);
```

This uses direct higher-order differential factories rather than repeated
self-composition.

### Commutator

```cpp
const operators::Operator commutator =
    operators::commutator(
        operators::differential("x"),
        operators::multiply(f.arg("x")));
```

For the multiplication-by-`x` operator, `[D_x, M_x]f = f`.

## User-Defined Operators

Custom operators use a factory and return the same `Operator` class as built-in
operators:

```cpp
const operators::Operator Dx =
    operators::differential("x");

const operators::Operator euler_x = operators::define(
    "example.euler_x",
    [Dx](const math::Function& input) {
      return input.arg("x") * Dx(input);
    },
    operators::Options{
        .version = 1,
        .linear = true,
        .platform_pure = true,
    });

const math::Function euler_f = euler_x(f);
```

No public builder class or inheritance is required.

An in-process custom operator requires only an application callback. A
persistable operator additionally requires:

```text
stable key
schema version
save hook
load hook
compatibility gate
```

Capability metadata may report:

```text
structural
evaluated-only
linear
adjoint-known
serializable
platform-pure
```

Capabilities are promises, not decorative labels. Composition combines them
conservatively.

## Internal Function Representation

`math::Function` owns a private immutable DAG. Initial node kinds include:

```text
Projection
Constant
SeriesConstant
Add Subtract Multiply Divide Negate
Power
Log Exp Sqrt
Sin Cos Tan
Sinh Cosh Tanh
Abs
Piecewise
Integral
Opaque
```

Nodes contain stable operation IDs, immutable children, payload metadata,
domain information, structural hashes, and source/debug metadata.

The public class never exposes node types.

Evaluation traverses a cached topological plan. Shared nodes are evaluated
once per call. Every numerical operation delegates to existing Series methods,
preserving CPU/MPS execution, broadcasting, dtype promotion, and autograd.

## Internal Operator Representation

`operators::Operator` is a small immutable handle over a private type-erased
concept. The private concept supports:

```text
apply(Function) -> Function
metadata
capabilities
serialization hooks
compatibility checks
```

Built-in operators may use compact tagged descriptors instead of heap-backed
callbacks. User operators use the same public value type.

## Simplification

The first simplifier performs safe local rewrites only:

```text
x + 0 -> x
x - 0 -> x
x * 0 -> 0
x * 1 -> x
x / 1 -> x
x - x -> 0
-(-x) -> x
constant arithmetic -> folded constant
```

It preserves common subexpressions and performs bounded parity analysis for
integrals.

Domain-sensitive identities such as `sqrt(x^2) -> x` or
`log(xy) -> log(x) + log(y)` are forbidden without sufficient assumptions.

## Domains And Non-Smooth Functions

Functions may carry domain and assumption metadata. Examples include:

```text
log(x)  requires x > 0 over the reals
1/x     excludes zero
sqrt(x) requires x >= 0 over the reals
abs(x)  is non-differentiable at zero
```

Non-smooth policies include:

```text
Strict
Piecewise
Subgradient
AutodiffConvention
```

The default must be strict or piecewise, never an undocumented subgradient.

## Opaque Callables And `Series::calc`

An arbitrary `std::function<Series(const Series&)>` cannot be inspected
structurally by portable C++.

The policy is:

- `math::function(...)` traces supported Function operations structurally.
- Typed Series callbacks continue to use `Series::calc`.
- Opaque Function nodes reject structural differentiation by default.
- Autodiff or numerical fallback must be requested explicitly.
- Function metadata reports whether a result is structural or fallback-based.

`Series::calc` remains:

- an evaluated derivative and integration surface
- an independent verifier of structural derivatives
- an explicit fallback engine for opaque operations

It does not become the implementation of structural transformation.

## Placement Among Existing Calculus Surfaces

The repository already contains several APIs with calculus-related names, but
they operate at different abstraction levels. The new system must clarify and
coordinate those levels rather than delete useful operations merely because
their names are similar.

### Sampled-Data Layer

`Series::diff(n, dim)` computes discrete forward differences of values that
already exist. `Series::antidiff()` integrates already-sampled values by
trapezoidal, cumulative, or stochastic path rules. These transform numerical
data into numerical data; neither reconstructs or transforms the unknown
function that generated the samples.

They remain Series operations:

```text
sampled Series -> sampled Series
```

### Autodiff Execution Layer

`Series::reverse(...)` is the reverse-mode execution terminal. It propagates a
sensitivity through an already-recorded numerical graph and accumulates or
retains gradients according to its options. It is infrastructure for eager
autodiff, not a school-style derivative operator and not a Function factory.

### Evaluated Calculus Layer

`Series::calc.diff`, `grad`, `jac`, and `hess` accept a callable and evaluate
its derivative information at the receiver. `Series::calc.antidiff` evaluates
a callable integral ending at the receiver. Their contract is:

```text
callable + evaluation point -> numerical Series
```

These methods remain the concise API when the caller wants values now rather
than a reusable transformed Function.

### Kernel Layer

`pluss::subsp::math::diff` owns lower-level derivative transforms including
`nth_diff`, JVP, VJP, Jacobian, Hessian, and HVP machinery.
`pluss::subsp::math::integ` owns numerical integration algorithms. These are
the execution engines beneath higher surfaces, not competing school-facing
APIs. `Series::calc` already delegates ordinary derivatives to the diff engine;
the same single-engine policy must be preserved and extended.

### Structural Operator Layer

`math::Function` plus `operators::Operator` adds the missing contract:

```text
Function -> transformed reusable Function
```

Known nodes are transformed structurally by analytic rules. Evaluation of the
resulting graph delegates to existing Series operations such as `Series::sin`,
`Series::log`, and `Series::exp`. Explicit opaque fallbacks may delegate to
`Series::calc` and therefore to the low-level diff or integration engines, but
fallback provenance must remain visible.

The intended hierarchy is therefore:

```text
operators + math::Function       structural public layer
Series::calc                     evaluated public convenience layer
Series::diff / antidiff          sampled-data public layer
Series::reverse                  autodiff execution terminal
subsp::math::diff / integ        shared lower engines
```

### Consolidation Rule

Future cleanup should consolidate implementation and documentation, not merge
different mathematical contracts. There must be one derivative engine and one
numerical integration engine beneath the public surfaces. True forwarding
duplicates may be retired or internalized after an API audit; sampled
differences, sampled integration, point-evaluated calculus, reverse-mode
execution, and structural function transformation must remain distinct because
they answer different questions.

## Mandatory Reuse Audit

No tranche may introduce a new mechanism until its implementation task has
been checked against the existing PlussQit, PlussMa, PlussTils, MSharp, and IO
infrastructure. Reuse is required when the existing contract matches. Reuse is
rejected when it would invert dependencies, introduce host-only execution into
a platform-native path, or force a semantically different abstraction to carry
the new contract.

The initial source audit establishes the following decisions.

| Existing facility | Decision | Calculus use |
| --- | --- | --- |
| `Series` mathematical methods and native dispatch | Reuse directly | Function evaluation calls the existing `Series` operation; no numerical kernel is duplicated. |
| `pluss::subsp::math::diff` | Reuse as the sole eager derivative engine | `Series::calc`, parity tests, and explicit opaque fallbacks share `nth_diff`, JVP, VJP, Jacobian, Hessian, and HVP machinery. |
| `pluss::subsp::math::integ` | Reuse as the sole quadrature engine | Unresolved and explicitly numerical integrals delegate to Quadra, Aleatoria, or Integra rather than adding another quadrature implementation. |
| `Series::antidiff()` | Reuse for sampled data only | Sampled trapezoidal and stochastic path integration remains on Series and is not rebuilt in Function. |
| `Series::reverse()` and `SeriesTape` | Reuse for eager execution, not structural storage | Evaluation and fallback autodiff may use the tape; the persistent Function DAG must not alias an ephemeral, value-bound reverse tape. |
| `pluss::msharp::intrusive_ptr` and `RefCounted` | Reuse | `Function` and immutable graph storage use the established cheap-handle ownership machinery instead of another reference-counted pointer implementation. |
| `RangeView`, `msharp::Lister`, and public `StackArray` adapters | Reuse according to lifetime | `RangeView` remains the borrowed RHS view for immediately consumed lists. `msharp::Lister` is the existing owning counterpart (there is no `ListView` type), while public `StackArray` is the ergonomic API adapter that accepts brace lists and lvalue ranges. Persistent Function metadata is copied into owned storage. |
| `pluss::dispense` | Reuse as optional runtime invocation | A registered Function method can be selected by name with positional or keyword arguments. It is not used inside the hot graph evaluator. |
| PlussTils `suppose(...).then/transform` | Interoperate, do not depend on it | Since `Operator` is callable, users may compose a Function through `suppose`; Function construction itself only needs `std::invoke`. |
| PlussTils `let` | Do not use internally | It is a payload-restricted immutable binding. `Function` is already immutable and should not gain a wrapper merely to imitate that property. |
| PlussTils `transmit` and `map_axis` | Do not use for numerical evaluation | They coordinate host ranges and host workers. Function evaluation must preserve whole-Series vectorization and the input platform. |
| PlussMa `integ::analytica` | Reuse selectively | Existing closed-form definite formulas provide constant-folding and verification opportunities; they are not a substitute for structural antiderivative nodes. |
| Procedure or PlussMic persistence code | Reuse design rules, not upward dependencies | Schema versions, compatibility checks, checksums, and corruption tests are required, but the math core cannot depend upward on PlussAI or PlussMic. Shared persistence machinery may be lowered only as a separate justified refactor. |

### Runtime Method Invocation

The MATLAB-like facility remembered during design already exists as
`pluss::dispense`:

```cpp
const std::any result = pluss::dispense(object, "method_name", arguments);
```

It supports registered member names plus positional and keyword binding. The
Function primitive registry must be the single source of method metadata; any
`dispense` registration for Function is generated from that registry rather
than maintained as a second hand-written operation list.

Runtime name dispatch is an optional boundary feature. Graph construction and
evaluation use typed operation IDs and typed evaluators because invoking every
node through `std::any` would weaken diagnostics and add avoidable overhead.

### PlussTils Dependency Boundary

PlussTils currently depends on PlussQit. PlussMa and the mathematical core must
not depend upward on PlussTils merely to reuse a small fluent wrapper. Such a
dependency would reverse the established layering and could create a cycle.

The intended interoperability remains natural without coupling:

```cpp
const math::Function derivative =
    pluss::tils::suppose(f)
        .transform(operators::differential("x"))
        .value();
```

No special adapter is needed because `operators::Operator` is a normal
callable value type.

### Components That Are Genuinely New

The audit found no existing persistent structural Function graph. The
following components therefore must be implemented, while still building on
the reused facilities above:

- immutable structural nodes and graph ownership
- variable identity and binding metadata
- primitive metadata combining evaluation, derivative, domain, rewrite, and
  serialization rules
- graph interning, memoization, and topological execution planning
- structural differentiation and bounded simplification
- unresolved integral nodes and structural integral metadata
- Function and Operator codecs

`SeriesTape` cannot replace this graph: it records operations on concrete
Series values for reverse execution, whereas `math::Function` must survive
without bound values, be serializable, and be transformable before evaluation.

### Reuse Gate Per Tranche

Before editing code in each tranche, its change record must list:

1. existing facilities inspected
2. facilities reused directly
3. facilities rejected and the semantic or dependency reason
4. genuinely new components being introduced

Tests must also prove delegation where practical. Examples include structural
evaluation matching direct Series methods, opaque differentiation matching
`Series::calc`, and unresolved integration reporting the diagnostics returned
by the existing integration engine.

## Distribution And CGF Integration

Distribution families with analytic cumulant generating functions may expose
them as `math::Function`:

```cpp
const math::Function K = distribution.cgf_function();

const operators::Operator Dt1 =
    operators::differential("t");

const operators::Operator Dt2 =
    operators::differential("t", 2);

const math::Function K1 = Dt1(K);
const math::Function K2 = Dt2(K);

const pluss::Series mean = K1({
    {"t", pluss::make_series(0.0)},
});

const pluss::Series variance = K2({
    {"t", pluss::make_series(0.0)},
});
```

Existing evaluated `cgf(t)` and `cumulant(order)` methods remain available.
Numerical CGFs and differentiation-under-the-integral policies must report
their fallback status honestly.

## Implementation Phases

### Phase 1: `math::Function`

Implement:

- immutable Function handle and private DAG
- normal named-lambda construction
- arbitrary-arity span construction
- scalar, vector, and matrix output metadata
- named and ordered evaluation
- initial pure mathematical methods
- primitive registry
- Series-native CPU/MPS evaluation
- graph validation and diagnostics

Acceptance gates:

- no public `Variable` or `Expression`
- normal lambda examples compile without `auto`
- arbitrary variable names and counts work
- evaluation matches direct Series expressions
- no observation-level host loops
- no duplicated numerical kernels

### Phase 2: `operators::Operator`

Implement:

- immutable Operator value type
- private type-erased concept
- standard metadata and capabilities
- identity, shift, scale, multiplication, composition
- `operators::define(...)`
- typed application `Function -> Function`
- composition flattening and validation

Acceptance gates:

- no public specialized operator subclasses
- every standard factory returns `operators::Operator`
- every operator application returns `math::Function`
- custom operators require no inheritance

### Phase 3: Differential Operators

Implement:

- `differential(variable, order)`
- `gradient(variables)`
- `jacobian(variables)`
- `hessian(variables)`
- arbitrary variable lookup
- derivative rule registry
- higher-order memoization
- vector/matrix Function outputs

Acceptance gates:

- direct higher-order differentiation is canonical
- analytic references pass
- structural results agree with `Series::calc` on smooth functions
- gradient, Jacobian, and Hessian factories still return `Operator`
- unsupported primitives fail explicitly

### Phase 4: Simplification And Domains

Implement:

- safe local rewrites
- common-subexpression preservation
- bounded factoring
- parity analysis
- domain assumptions
- non-smooth policies
- graph-growth safeguards

### Phase 5: Integral Operators

Implement:

- exact primitive antiderivatives
- bounded structural integration rules
- supported special-function forms
- anchored and definite integral operators
- unresolved Integral nodes
- Fundamental Theorem and Leibniz rules
- existing vectorized quadrature delegation
- convergence and exactness metadata

Acceptance gates:

- every integral factory returns `Operator`
- every transformed integral remains `Function`
- integrating out one variable preserves remaining variables
- unsupported closed forms remain honest Integral nodes
- Lebesgue integral and principal value are never conflated

### Phase 6: `Series::calc` Interoperability

Implement:

- derivative parity verification
- explicit autodiff fallback policy
- explicit numerical differentiation policy
- structural/fallback provenance
- no second differentiation tape
- no second quadrature engine

### Phase 7: Distribution And CGF Integration

Implement analytic CGF functions family by family, derive structural cumulants,
and verify them against existing distribution formulas and sampled moments.

### Phase 8: Production Hardening

Implement:

- stable serialization schema
- operation and operator IDs
- custom operator registry and codecs
- compatibility gates
- corrupted artifact tests
- execution-plan caching
- derivative caching
- thread safety
- CPU/MPS parity
- benchmarks
- handwritten API reference
- compiled examples with verified outputs

## Execution Tranches

The eight architectural phases are delivered in ten gated implementation
tranches. Splitting the foundational phases prevents a large unverified core
from accumulating before platform and contract tests exist.

### Tranche 1: Contract Skeleton

- public namespaces and umbrella includes
- concrete `Function` and `Operator` declarations
- compile-time return-type contract tests
- forbidden-public-type header audit
- CMake targets and empty smoke tests

#### Tranche 1 Reuse Record

The contract pass inspected `RangeView`, `msharp::Lister`, `StackArray`,
`intrusive_ptr`, `RefCounted`, existing PlussMa CMake composition, and the
public `pluss/ma.h` umbrella.

- `StackArray<std::string_view>` is reused by value for variable-name inputs.
  It accepts initializer lists and ordinary lvalue ranges while its underlying
  string-aware `Lister` owns copied names, so a returned Function cannot retain
  a dangling view into caller storage.
- The audit corrected `BasicLister<std::string_view>` copy and move operations
  so each destination rebuilds its cached views against its own string storage.
  A lifetime regression test destroys the original source before reading the
  copied and moved public adapter.
- `RangeView<T>` is reused for operator and coefficient lists that are consumed
  immediately by an operator factory and are not retained as borrowed memory.
- `intrusive_ptr` and `RefCounted` are reused for the two cheap public handles;
  no second reference-counting mechanism is introduced.
- A proposed `ListView` was rejected because no such facility exists. The
  established owning facility is `msharp::Lister`, exposed at public API
  boundaries through `StackArray`.
- The only genuinely new tranche-one components are the concrete public
  `Function` and `Operator` handles, their canonical namespace aliases, the
  Calculus build target, and contract/audit tests. No graph, primitive,
  differentiation, or integration implementation is introduced in this
  tranche.

### Tranche 2: Function DAG And Factory

- private immutable DAG
- `math::function(...)` overload family
- projection and constant nodes
- arbitrary names and variable counts
- scalar, vector, and matrix output metadata
- arity, duplicate-name, and binding validation

#### Tranche 2 Reuse Record

The Function pass inspected the Note formatter's callable traits, Series shape
and cloning contracts, `StackArray`, `msharp::Lister`, `intrusive_ptr`,
`RefCounted`, and the repository error machinery.

- `intrusive_ptr` and `RefCounted` own immutable trace, node, and Function
  implementation objects. No parallel graph-pointer implementation exists.
- `StackArray<std::string_view>` and its owning string `Lister` remain the
  variable-name boundary. The sealed trace owns names independently of the
  caller's initializer list or lvalue range.
- `Series::sizes()` and `Series::dim()` provide scalar, vector, and matrix
  output metadata. Series constants are captured with `Series::clone()` so
  later in-place mutation of the caller's handle cannot alter the Function.
- Existing `PLUSS_CHECK` and `pluss::Error` contracts report empty names,
  duplicate names, arity mismatches, undefined results, unsupported output
  ranks, and foreign projection bindings consistently with the rest of Pluss.
- The Note formatter's callable-trait implementation was not reused directly:
  it is private to Note formatting and couples invocation to Note argument
  conversion. A calculus-private trait layer supports fixed callables and the
  explicit `std::span<const Function>` runtime-arity form without introducing
  a new public utility.
- The genuinely new components are the private immutable projection and
  constant nodes, trace ownership IDs, sealed Function implementation, and
  one-shot lambda tracing. Primitive mathematical nodes and numerical
  evaluation remain tranche-three work.

### Tranche 3: Primitive Registry And Evaluation

- initial mathematical Function methods
- one primitive registry
- topological execution plan
- Series-method delegation
- broadcasting and dtype behavior
- CPU/MPS parity

#### Tranche 3 Reuse Record

The evaluation pass inspected the existing `Series` arithmetic, elementary
function, broadcasting, dtype, platform-transfer, cloning, and scalar-factory
contracts before adding calculus execution machinery.

- One private registry owns stable IDs, names, arities, and evaluators for the
  initial 18 primitives: addition, subtraction, multiplication, division,
  power, negation, exponential, logarithm, `log1p`, `expm1`, square root,
  circular and hyperbolic sine/cosine/tangent, and absolute value.
- Every primitive evaluator delegates directly to the corresponding `Series`
  method. The calculus layer contains no element-wise numerical kernels and no
  host loop over observations.
- A sealed `Function` compiles its immutable DAG once into a topologically
  ordered execution plan. Shared nodes retain one plan entry, so evaluation
  does not recompute a common subexpression.
- Evaluation accepts ordered Series arguments or named bindings. Named inputs
  are reordered once through the compiled binding index and reject unknown,
  duplicate, missing, undefined, or mixed-platform values before execution.
- `Series` owns runtime broadcasting and dtype promotion. Structural Function
  metadata applies the same trailing-axis broadcasting rule early so
  incompatible constant output shapes fail while the Function is built.
- Scalar constants use `codem::make_scalar_like` when a variable input is
  present, preserving that input's dtype and platform. Captured Series
  constants are cloned at construction and again at evaluation, preventing
  mutation through either the caller's original handle or a returned result.
- CPU behavior is verified against direct `Series` expressions, including the
  motivating multivariable formula, all registered primitives, named and
  ordered bindings, Float32 broadcasting, constant isolation, graph sharing,
  and failure contracts. The same test verifies MPS routing and CPU/MPS parity
  when the configured runtime exposes MPS; it is explicitly skipped otherwise.
- The only genuinely new tranche-three components are the primitive metadata
  registry, immutable primitive nodes, compiled execution steps, and named
  binding router. Numerical behavior remains owned by `Series`.

### Tranche 4: Operator Core And Algebra

- concrete type-erased `Operator`
- identity, shift, multiply, scale
- compose, linear combination, commutator
- capability metadata
- `define(...)` for custom operators
- composition flattening

#### Tranche 4 Reuse Record

The operator pass reused the immutable Function DAG, intrusive ownership,
primitive registry, compiled Function execution plans, and existing Series
runtime rather than introducing an operator-specific numerical engine.

- Public `operators::Operator` remains one concrete, copyable value handle.
  Its abstract concept, leaf implementation, and flattened-composition
  implementation are private calculus details; users neither inherit from nor
  name concrete operator subclasses.
- `identity`, `shift`, `multiply`, `scale`, `compose`,
  `linear_combination`, and `commutator` all transform `math::Function`
  objects structurally. Applying an operator performs no Series evaluation and
  no host loop over observations.
- `Function::arg(name)` returns a coordinate projection tied to the Function's
  existing trace. This makes multiplication operators precise without
  constructing a second, incompatible variable trace.
- Shift substitutes the selected projection in one pass over the Function's
  cached topological plan. Rebuilt nodes continue to share transformed child
  nodes, and all arithmetic remains represented by the tranche-three primitive
  registry.
- Composition stores application order from inner to outer and flattens nested
  compositions. Identity components are removed, avoiding nested callback
  traversal and redundant application steps.
- Linear combinations own copied operators and coefficients. The implementation
  loops over algebraic operator terms only; resulting Functions combine through
  existing graph primitives and retain Series platform behavior at evaluation.
- Capability metadata is combined conservatively. Structural, linear,
  adjoint-known, serializable, and platform-pure promises survive only when
  every required component provides them; evaluated-only propagates if any
  component requires it.
- `define(...)` accepts named or anonymous functions, lambdas, and move-only
  callable state through shared erased storage. Invalid callbacks, undefined
  inputs or outputs, contradictory capabilities, and premature serialization
  claims fail explicitly. Custom serialization remains reserved for tranche
  eight, where real codec hooks can support the promise.
- CPU tests verify operator meaning, composition order, flattening, coordinate
  multiplication, parallel linear combinations, commutators, custom metadata,
  move-only state, and failure contracts. The complete suite also runs against
  the native MPS runtime from `build_cpp_env`; all CPU and Metal assertions
  pass without skips.
- The genuinely new tranche-four components are the private operator concept,
  leaf/composition models, conservative capability router, and structural
  variable substitution. Numerical kernels remain owned entirely by Series.

### Tranche 5: Differential Engine

- direct first and higher derivatives
- primitive derivative rules
- `(node, variable, order)` memoization
- arbitrary variable selection
- structural-versus-`Series::calc` parity

#### Tranche 5 Reuse Record

The differential pass transforms the immutable Function DAG directly. It does
not invoke eager autodiff to manufacture a derivative value and does not add a
second element-wise execution engine.

- `operators::differential(variable, order)` is the sole public differential
  factory. It returns the existing concrete `Operator` handle, validates a
  positive order, and resolves the requested coordinate against the Function's
  owned variable trace when applied.
- Rules cover every primitive introduced in tranche three: arithmetic,
  variable and constant powers, negation, exponential and logarithmic forms,
  square root, circular and hyperbolic functions, and absolute value away from
  its non-differentiable origin. The explicit non-smooth policy remains tranche
  seven work; this pass does not silently invent one at zero.
- Higher derivatives are requested directly through
  `differential(variable, order)`. One differentiator memoizes by
  `(node identity, variable identity, order)`, preserving shared derivative
  subgraphs instead of repeatedly expanding a common source expression.
- Derivatives of projections and constants use two private graph primitives,
  `zeros_like` and `ones_like`. Their evaluators delegate to the canonical
  `pluss::series::zeros_like` and `pluss::series::ones_like` factories, so
  runtime shape, dtype, and CPU/MPS platform follow the relevant Series input.
  They are implementation nodes, not additions to the public Function method
  surface.
- All generated arithmetic remains expressed through the existing primitive
  registry and compiled execution plan. There is no host loop over Series
  observations and no calculus-owned numerical kernel.
- Arbitrary named partial derivatives retain the original Function arity and
  binding order. Differentiating with respect to an absent variable fails at
  operator application rather than producing a misleading constant zero.
- Independent parity tests compare first and second structural derivatives to
  `Series::calc.diff`. This is verification across two calculus layers, not an
  implementation dependency: structural differentiation never delegates to
  the eager calculator.
- Tests cover the motivating two-variable expression, direct third and fifth
  derivatives, all unary rules, binary and variable-power rules, runtime zero
  and one shapes, shared-node identity, invalid variables and orders, and
  native MPS execution. The complete native suite passes 38 cases and 169
  assertions with no MPS skips.
- The genuinely new tranche-five component is the private memoized graph
  transformer. Allocation, broadcasting, dtype promotion, platform execution,
  and elementary numerical behavior remain owned by Series.

### Tranche 6: Multivariable Outputs

- gradient, Jacobian, and Hessian factories
- vector and matrix output shape inside `Function`
- derivative-axis ordering
- selected and all-variable modes
- batched evaluation

#### Tranche 6 Reuse Record

The multivariable pass keeps the two-type public model intact. Gradient,
Jacobian, and Hessian are `Operator` factories, and their transformed outputs
remain ordinary `Function` values; no public vector-function, matrix-function,
gradient, Jacobian, or Hessian class was introduced.

- `operators::gradient`, `operators::jacobian`, and `operators::hessian`
  accept an optional ordered variable selector. An empty selector means every
  Function variable in canonical declaration order. Named subsets preserve the
  caller's order and reject empty, duplicate, or unknown names.
- Derivative axes are trailing. For a batch shape `B`, output shape `O`, and
  selected-variable count `N`, evaluation follows `gradient -> [B..., N]`,
  `jacobian -> [B..., O..., N]`, and `hessian -> [B..., N, N]`. Static Function
  metadata records only the mathematical output axes; runtime Series batch axes
  remain unconstrained and are preserved ahead of them.
- Gradient and Hessian require scalar-valued input Functions. Jacobian accepts
  scalar- or vector-valued Functions. Transformations that would exceed the
  framework's scalar/vector/matrix Function output contract fail before
  evaluation.
- Native C++ containers are not tensor values on this surface.
  `math::function(...)` continues to accept only Function, Series, or arithmetic
  results. Vector, matrix, and higher-dimensional runtime values remain owned
  and represented by Series; private structural stacking exists only to
  coordinate multivariable operator outputs.
- One private variadic stack primitive represents structural vector and matrix
  assembly. Its evaluator delegates to `Series::pile` on the active platform;
  calculus owns no concatenation kernel and performs no host traversal over
  observations.
- Execution steps now own a dynamic graph-edge list rather than a two-input
  array. This supports arbitrary variable and output component counts while
  retaining one topologically compiled plan and one evaluation per shared node.
- The tranche-five differentiator was generalized so one memo table serves all
  selected variables and derivative orders during each multivariable transform.
  Stack differentiation maps structurally over component nodes and preserves
  shared source subgraphs.
- Tests cover canonical and reordered selectors, direct Hessian references,
  Jacobian/Hessian agreement, four-variable and subset modes,
  multidimensional batches, rank and selector
  errors, shared graphs, and native MPS execution. The complete native suite
  passes 46 cases and 205 assertions with no MPS skips.
- The only genuinely new execution primitive is structural stack. Numerical
  stacking, dtype handling, batching, and CPU/MPS behavior remain owned by
  Series.

### Tranche 7: Simplification And Domains

- safe local rewrites
- common-subexpression preservation
- bounded factoring and parity analysis
- domain assumptions
- non-smooth policies
- graph-growth safeguards

#### Tranche 7 Reuse Record

- `math::function(...)` accepts only `math::Function`, arithmetic scalars, or
  `Series` results. Native containers are never interpreted as tensors.
  `Series` remains the sole tensor value and constant Functions retain its
  complete rank, including rank three and higher.
- Local rewrites preserve runtime shape. In particular, `x - x`, `0 * x`, and
  `x^0` use `zeros_like` or `ones_like` rather than scalar constants.
- Constant folding, double-negation removal, `log(exp(x))`, assumption-backed
  `sqrt(x^2)`, and assumption-backed `x/x` are memoized graph rewrites. Common
  factor extraction is deliberately bounded to pointer-identical factors; it
  does not perform distributive search or duplicate shared subgraphs.
- `Function::assume` records per-variable real-domain facts. Compatible facts
  narrow to stronger facts, contradictory facts fail, and rewrites requiring a
  fact are not applied without it.
- Real-valued division, logarithm, `log1p`, and square-root domains are checked
  with Series-native comparisons and reductions on the active platform.
- Absolute-value differentiation is explicit. `Strict` rejects zero,
  `Piecewise` leaves the classical branch undefined there, and `Subgradient`
  and `Autodiff` select the canonical zero value.
- Derivative factories carry a configurable node budget. A transformed graph
  that exceeds it fails before execution; an independent absolute graph limit
  protects every compiled Function plan.
- Parity is inferred conservatively from graph primitives. It is used only
  where a proof is available, such as resolving an odd integrand over finite
  symmetric bounds to exact zero.

### Tranche 8: Integral Engine

- exact primitive antiderivatives
- bounded structural patterns
- special-function forms
- anchored and definite integral operators
- unresolved Integral nodes
- Fundamental Theorem and Leibniz rules
- existing quadrature delegation and convergence metadata

#### Tranche 8 Reuse Record

- `operators::antiderivative` implements exact rules for constants,
  projections, sums, differences, constant factors and divisors, powers,
  exponential forms, circular sine/cosine, and hyperbolic sine/cosine.
- Anchored and fixed-bound integral factories substitute exact primitives when
  available. Unsupported compositions become a private structural Integral
  node rather than pretending to have an elementary closed form.
- Integral-node evaluation delegates to the existing
  `pluss::subsp::math::integ::quadra` implementation. No calculus-owned
  quadrature loop or numerical kernel was introduced. Remaining parameters are
  broadcast as Series batches and numerical work stays on CPU or MPS.
- Adaptive tolerance follows the active dtype. Non-convergence is an error
  carrying the observed estimate; it is never silently returned as an exact
  result.
- Differentiating an unresolved anchored integral with respect to its bound
  applies the Fundamental Theorem structurally and recovers the integrand
  without differentiating a quadrature result.
- Exact symmetric odd integrals bypass quadrature. This is a finite ordinary
  integral result, not an implicit Cauchy-principal-value interpretation.
- Native CPU/MPS verification closes tranches seven and eight at 62 Catch2
  cases and 261 assertions, with no platform skips.

### Stochastic Operator Boundary

> Superseded and made concrete by *Extension Programme*, Tranche 12 below. The boundary stated here still holds.

The ten deterministic tranches do not treat stochastic calculus as a mode of
ordinary differentiation or integration. A later stochastic operator family
will reuse the existing `pluss::subsp::math::sde` engines in
`random_processes.h`, including Itô, Stratonovich, theta, Marcus, Malliavin,
and Skorokhod machinery. Its operators will carry path, filtration,
adaptedness, quadratic-variation, and convention contracts explicitly.

This separation prevents an expression such as `integral("x", a, b)` from
silently changing meaning when its argument happens to be random. Structural
`Function` graphs may be consumed by stochastic operators, but stochastic
integration remains path-aware and delegates to the established SDE layer.

### Tranche 9: Existing-System Integration

- explicit `Series::calc` fallback and provenance
- no duplicate tape or quadrature engine
- generic CGF examples built from ordinary `math::Function` graphs
- structural cumulant verification without a distribution-specific calculus API
- umbrella integration with PlussMa

#### Tranche 9 Reuse Record

- Opaque Series callables remain an explicit escape hatch. Structural
  differentiation rejects them by default and delegates to the existing
  `Series::calc` derivative engine only when `Fallback::Autodiff` is selected.
- The calculus layer owns neither an autodiff tape nor a quadrature kernel.
  Evaluated fallback and unresolved integrals delegate to the established
  Series and numerical-integration engines.
- Cumulant-generating functions are validation examples, not a specialised
  calculus API. A CGF is an ordinary `math::Function`; repeated
  `operators::differential` calls recover its cumulants at zero.
- Provenance distinguishes structural, opaque, quadrature and autodiff paths,
  so a caller can audit how a result will be evaluated.

### Tranche 10: Production Closure

- Function and Operator serialization
- custom operator registry and codecs
- compatibility and corrupted-artifact tests
- execution and derivative cache benchmarks
- thread-safety audit
- full CPU/MPS suite
- handwritten API reference
- compiled examples with verified outputs
- full repository build and staging verification

#### Tranche 10 Closure Record

- `math::io` persists structural `Function` graphs and serializable
  `Operator` descriptors through a versioned, checksummed binary format.
  Series-valued constants reuse the existing Series serializer.
- Restored functions receive fresh trace identities. Assumptions, graph
  sharing, variables, constants and unresolved integral nodes survive the
  round trip; opaque callables are rejected because executable code is not a
  portable artifact.
- `operators::bind(key, version, loader)` provides the stable custom-operator
  persistence contract. Built-in operator descriptors and custom codecs are
  version checked, and missing loaders fail loudly.
- Production tests cover round trips, corrupted and truncated artifacts,
  schema mismatches, wrong artifact kinds, custom loaders and concurrent
  evaluation of one immutable function.
- Compiled demonstration and benchmark targets exercise structural
  differentiation, numerical integration, generic CGF cumulants, persistence,
  graph construction cost and repeated evaluation.
- The handwritten calculus manual is generated from executable templates.
  Its programs are compiled and run by the documentation renderer, and their
  captured output is placed directly below each program.
- The final contract suite passes 91 test cases and 485 assertions with native
  MPS enabled. The same suite passes 84 CPU cases and 470 assertions in the
  sandbox, with the seven platform cases explicitly skipped when Metal is not
  available. The public-surface audit and environment-parser regression suite
  also pass.
- `pluss_sdk_stage` completes from `build_cpp_env` and installs the calculus
  public headers and libraries into
  `build_cpp_env/sdk/pluss-sdk-0.1.0-Darwin-arm64`.
- The repository-wide all-target build exposed and corrected three stale
  integration examples: a `HostValue`/`std::vector` assumption, a duplicate
  scalar-conversion helper, and an ambiguous vector/span overload. The live
  `cpp_env` preset still regenerates Series wrappers on each invocation and
  therefore invalidates its entire all-demo build graph; this build-system
  inefficiency is separate from calculus correctness and staging closure.

Every tranche closes only after its Catch2 tests, contract audits, relevant
platform tests, and affected repository builds pass. A tranche is not complete
when only its happy-path demonstration works.

## Extension Programme: Geometric, Stochastic, Measure-Theoretic And Advanced Operators

Status: **approved scope (2026-10-27), implementation not started.** This
programme extends the ten deterministic tranches above. It adds operator
*families* without adding a public class: every factory below still returns
`operators::Operator`, every application still returns `math::Function`, and
the public-surface audit gains the new factory names as an explicit allow-list.
This section is the "demonstrated use case and explicit public-surface review"
that the factory inventory above requires.

**The Analytic Result Rule (user directive, 2026-10-28).** After an operator acts
on a `Function`, the result is an *analytic* `Function` that waits for values to
be supplied, never a numerical or eager object. Eager and numerical calculus on
sampled arrays is the job of `Series::calc` and the other eager contracts; the
separation between the two is deliberate and this programme does not blur it.
Consequences, applied to everything below:

- No operator may return an evaluated-only (opaque callable) Function. The
  capability flag `evaluated_only` stays for user operators made with `define`,
  and for the existing quadrature fallback of an unresolved integral node,
  which is a lazy numerical rule inside an otherwise analytic graph and is
  reported through provenance; nothing new is added to that category.
- Where a mathematical object has a closed or graph form (derivative graphs,
  power-series recurrences, Fourier coefficient integrals, generators), the
  operator builds that graph. Where it does not (sampled-path reverse passes,
  interpolation along jump segments), it is not an operator: it stays on
  `Series::calc`.
- Performance is tracked as we go (graph size per operation, build time, and
  evaluation cost are measured in each tranche's record) so the full
  performance review has little left to find.

User decisions that fix the design (recorded so they are not re-litigated):

| Question | Decision |
| --- | --- |
| Meaning of `absolute_differential` | The **tensor absolute (intrinsic) differential** of Ricci and Levi-Civita along a curve. The existing `|f^(n)|` operator is **renamed** `magnitude_differential`. |
| Form of stochastic operators | **Path-functional `Operator`s on `Function`.** No new public path, filtration, or process class. |
| Meaning of the Lebesgue integral | **Measure-based, simple-function / layer-cake evaluation**, genuinely different from the Riemann operator, not a vocabulary wrapper. |

### The Two-Type Invariant

This programme adds **no public type** to `math::Function` and
`operators::Operator`. The rules it is held to:

1. Every new capability is a factory returning `operators::Operator`, or a
   factory returning `math::Function` (`levi_civita`, `connection` builders).
   A connection, metric, density, CDF, curve, drift, diffusion, kernel or
   indicator is a `math::Function`; it never gets a wrapper.
2. The only other public declarations are **plain-data option structures and
   enums** with no member functions, no inheritance and no invariants
   (`Convention`, `TensorSignature`, `IntegralOptions`, `LebesgueOptions`,
   `FractionalKind`). They are parameters, not concepts a user must learn.
3. Path axis, tensor signature and similar single-integer settings are
   fields of an options structure, not structures of their own.
4. Anything that would need behaviour (a measure, a process, a filtration, a
   manifold, a tensor) is expressed by the existing operator algebra
   (`compose`, `linear_combination`, `multiply`, `commutator`, `adjoint`) or
   stays a private `detail` type behind `OperatorConcept` or a graph node.
5. New factory names are bounded: the audit allow-list below is the complete
   set; any addition requires a blueprint amendment.
6. The public-header audit asserts both directions: forbidden class names
   stay absent, and the count of public classes in `operators` and `math`
   remains exactly two.

### Namespace And Naming Rules For The Extension

The flat `pluss::subsp::math::operators` namespace keeps the factories of the
original inventory. The extension families live in public sub-namespaces so
that the flat namespace does not grow by dozens of names and so that each
family's contract is documented as a unit:

```text
pluss::subsp::math::operators::geometry     absolute_differential, covariant, lie, ...
pluss::subsp::math::operators::stochastic   stochastic_integral, generator, ...
pluss::subsp::math::operators::measure      lebesgue, weighted, atomic, stieltjes, measure_of
pluss::subsp::math::operators::variational  euler_lagrange, functional_derivative, ...
pluss::subsp::math::operators::transform    fourier, laplace, convolution, ...
pluss::subsp::math::operators::fractional   differential, integral
```

Public aliases follow the existing rule (`pluss::operators::stochastic`,
users shorten locally; no `pluss::<family>` root aliases). Option structures and
enums for a family live beside its factories. Concrete model classes stay in
`operators::detail`. The new families add **option structures and enums only**:
`Convention`, `TensorSignature`, `FractionalKind` and per-family `...Options`. None of
these is a class with behaviour, and none may grow virtual functions or
inheritance in public headers.

`operators::absolute_differential` and `operators::covariant` remain reachable
from the flat namespace as forwarding declarations of the `geometry` versions
for one release, then the forwards are removed (see Tranche 11 migration).

### Capability Vocabulary Additions

`Capabilities` gains two promises, combined conservatively by composition like
the existing ones:

```text
path_functional   evaluation requires a shared sampled path axis (stochastic family)
measure_aware     the result depends on an explicit measure, not only on Lebesgue measure
```

Both default to `false`. A composition containing any operator with
`path_functional` is `path_functional`. A `path_functional` operator is never
`structural` in the sense of closed-form symbolic algebra: it is structural in
the sense that it records a graph node, and evaluated-only in the sense that
the node needs sampled data. This distinction is reported, not hidden.

### Tranche 11: Geometric Operators And The Absolute Differential

**Mathematics.** Let `x^k(t)` be a curve in coordinates `x`, `V` a tensor field
of signature `(p, q)` (p contravariant, q covariant indices) and
`Γ^i_{jk}(x)` a connection. The absolute (intrinsic) differential along the
curve is

```text
DV^{i1..ip}_{j1..jq}/dt
    = dV/dt
    + sum_a  Γ^{i_a}_{m k} V^{..m..}_{..} dx^k/dt          (one term per contravariant index)
    - sum_b  Γ^{m}_{j_b k} V^{..}_{..m..} dx^k/dt          (one term per covariant index)
```

with the Levi-Civita connection obtained from a metric `g`:

```text
Γ^i_{jk} = 1/2 g^{il} (d_j g_{lk} + d_k g_{lj} - d_l g_{jk})
```

`DV/dt = 0` is parallel transport; the geodesic equation is `D(dx/dt)/dt = 0`.
The covariant derivative along a coordinate direction is the same formula with
`dx^k/dt` replaced by the unit vector `e_k`. Both therefore share one private
kernel.

**Public surface.**

```cpp
namespace geometry {
struct TensorSignature { std::int64_t contravariant = 0; std::int64_t covariant = 0; };
[[nodiscard]] math::Function levi_civita(const math::Function& metric,
                                         pluss::StackArray<std::string_view> coordinates);
[[nodiscard]] Operator absolute_differential(const math::Function& curve,          // x^k(t), output rank 1
                                             const math::Function& connection,     // Γ^i_{jk}(x), output rank 3
                                             std::string_view parameter,
                                             TensorSignature signature = {1, 0},
                                             DiffOptions options = {});
[[nodiscard]] Operator covariant(std::string_view coordinate,                      // generalised: tensors of any signature
                                 const math::Function& connection,
                                 TensorSignature signature = {0, 0},
                                 DiffOptions options = {});
[[nodiscard]] Operator magnitude_differential(std::string_view variable,          // the old |f^(n)|
                                              std::int64_t order = 1,
                                              DiffOptions options = {});
}
```

`levi_civita` returns a `math::Function`, not an operator: it builds `Γ`
structurally from `metric` by applying `differential` to each metric component
and contracting with the inverse metric.

*Numerics are not new.* Matrix inverse, pseudo-inverse, Cholesky, determinant
and solves already exist as `Series.linearalg` (`pluss::subsp::linearalg`) and
are reused verbatim at evaluation. What `math::Function` lacks is graph nodes:
the primitive registry holds 24 elementwise/stack primitives and no matmul,
inverse or contraction. Tranche 11a therefore adds only **registry
primitives that wrap the existing Series operations**, each with an output
shape rule and a derivative rule: `Matmul`, `Contract` (einsum-style index
contraction), `Transpose`, `Inverse` (`d(A⁻¹) = -A⁻¹ dA A⁻¹`), and `Det`
(`d det A = det A tr(A⁻¹ dA)`). No numerical kernel is written.

#### Tranche 11a Record (complete, 2026-10-27)

Built and verified before any geometric operator:

- Registry primitives (ids 25-30): `Transpose`, `Contract`, `Inverse`, `Det`,
  `Trace`, `Select`. They are the first **attribute-carrying** primitives: a
  node now holds integer attributes (ranks, axes, indices), evaluated through
  `AttributeEvaluator`, persisted for these ids only (older saved Functions
  load unchanged), and rebuilt by the simplifier, `substitute`, `rebind` and
  `shift` through one shape-rule factory.
- `Contract` is a general pairwise tensor contraction (any number of contracted
  axis pairs, outer products when none). `matmul`, dot and matrix-vector are
  contractions of the last axis with the first. It evaluates by
  permute/reshape/matmul on `Series`; the blueprint's earlier `Matmul` and
  `einsum`-style `Contract` are one primitive. `Select` replaces the need for
  index-spec strings: any component extraction is `select(axis, index)`.
- Derivative rules: `Transpose`, `Trace`, `Select` are linear;
  `Contract` is bilinear (product rule); `d(A^-1) = -A^-1 dA A^-1`;
  `d det A = det A tr(A^-1 dA)`. Verified against central differences,
  Jacobi's formula, `A A^-1 = I`, second derivatives, and the sphere metric
  `diag(1, sin^2 theta)` (inverse, determinant, `d det g = 2 sin cos`).
- Public surface: `Function::{matmul, contract, transpose, select, inverse,
  det, trace}` and the free function `math::stack(...)`. Without `stack` a
  matrix-valued Function could not be assembled from scalar Functions at all
  (the existing public API had only constants and the operators' own outputs).
  `stack` appends a trailing output axis, so stacking column vectors gives the
  matrix whose columns they are, the same convention as `jacobian`.
- **Output rank limit raised from 2 to 4.** The previous cap ("scalar, vector
  or matrix") made the rank-3 connection `Gamma^i_{jk}` and the rank-4
  Riemann tensor inexpressible. The check is now a single named constant.
  No existing behaviour changes below rank 3.
- Domain contract: `inverse` raises on a singular matrix at evaluation (as
  division raises on a zero denominator); shape errors (non-square, bad axes,
  mismatched contracted lengths, rank over 4) raise at construction.
- Evaluation reuses `pluss::subsp::linearalg` (`inverse`, `det`) and `Series`
  permute/reshape/matmul; no kernel was written. `Series` itself has no
  `einsum`, `tensordot`, `diagonal` or `trace` today (a torch-parity gap);
  promoting contraction into `Series` is a recommended separate change, after
  which `Contract` should delegate to it.
- Known limit, unchanged: batch axes lead and output axes trail, so a
  hand-assembled Function mixing outputs of different rank with batched
  arguments pairs trailing axes like any two arrays. Tensor operations act on
  the trailing output axes and broadcast leading batch axes of both operands.
  The tests evaluate at single points for that reason; batched use needs
  every stacked component to carry the batch shape.

#### Tranche 11b Record (complete, 2026-10-28)

- `math::Function::compose({g0, g1, ...})` (new, general): `f(g0, g1, ...)` by
  structural substitution on the execution plan; opaque callables and
  unresolved integrals are rejected with an error. Needed to evaluate a
  connection along a curve, and useful on its own (chain rule, change of
  variables).
- `geometry::levi_civita(metric, coordinates = all, DiffOptions)` returns the
  rank-3 connection `Gamma[i][j][k] = Gamma^i_{jk}`, built only from
  `differential`, `inverse`, `transpose` and `contract` (no new primitive).
- `geometry::absolute_differential(curve, connection, parameter, signature,
  DiffOptions)`: along-curve form, input is the tensor along the curve (a
  Function of the parameter). Field-on-manifold input is expressed by
  composing the field with the curve first (`field.compose(curve components)`)
  or by `geometry::covariant`.
- `geometry::covariant(coordinate, connection, signature, DiffOptions)`:
  covariant derivative of a tensor field in one coordinate direction, any
  signature. The earlier scalar `operators::covariant(variable, connection)`
  is unchanged.
- `operators::magnitude_differential` is the renamed `|f^(n)|` operator. The
  old flat `absolute_differential(variable, order, options)` is removed on
  purpose (no forwarding shim: a silently different meaning under the old
  name was the problem). Persistence: key `absolute_differential` at schema
  version 1 loads as `magnitude_differential`; version 2 is reserved for the
  geometric operator, which is not serializable (it holds Functions).
- Verification (`Math/Calculus/test/geometry_operators_test.cpp`, 117
  assertions): sphere and polar Christoffel symbols against closed forms and
  torsion-freeness; latitude-circle acceleration `(-sin cos, 0)`; the
  closed-form parallel-transported field has `DV/dt = 0` and its holonomy
  angle `2 pi cos(theta0)`; equator velocity parallel; metric compatibility
  `d(g V W)/dt = g(DV, W) + g(V, DW)` against central differences;
  `nabla g = 0`, `nabla g^-1 = 0`; symmetric covariant Hessian; Cartesian-
  constant field in polar components is parallel; input validation.
- Guide: the page `calculus_02_derivatives/absolute_and_covariant_differentials`
  now covers the magnitude operator and the geometric pair with two verified
  programs; operators-are-values and saving-and-loading pages updated.
- Not built (gated on a use case, as planned): `lie`, `exterior`, `divergence`,
  `laplace_beltrami`, `curl`, `directional`; curvature tensors (expressible now
  from `levi_civita`, `differential`, `contract`).

**Migration of the existing operators.**

- `absolute_differential(variable, order, options)` (magnitude of the n-th
  derivative) becomes `magnitude_differential` with the same behaviour,
  capabilities and payload. Serialized artifacts that contain key
  `absolute_differential` at **schema version 1** continue to load and
  produce `magnitude_differential`; the geometric operator is written under
  key `absolute_differential` at **schema version 2** so the two can never be
  confused. The loader dispatches on `(key, version)`.
- `covariant(variable, connection)` with scalar sections is the `(0,0)`
  signature case of the generalised operator and keeps its current results
  bit-for-bit. Tensor signatures lift the scalar-only restriction.
- Guide page `calculus_02_derivatives/absolute_and_covariant_differentials`
  is rewritten: the magnitude operator moves to its own short section, the
  geometric operator gets worked examples (sphere, polar coordinates).

**Additional geometry operators, each gated on a use case** (implemented only
when a test problem in this programme or in NatSci needs them; otherwise they
stay listed here):

| Operator | Meaning | Gate |
| --- | --- | --- |
| `geometry::lie(vector_field)` | Lie derivative `L_X` of scalar, vector and covector fields | NatSci fluids transport theorems |
| `geometry::exterior()` | exterior derivative on antisymmetric covariant tensors (forms) | needs a form representation; deferred |
| `geometry::divergence(metric)` | `(1/sqrt|g|) d_i (sqrt|g| V^i)` | NatSci curvilinear grids |
| `geometry::laplace_beltrami(metric)` | `div grad` on a Riemannian manifold; reduces to `laplacian` for the Euclidean metric | NatSci curvilinear grids |
| `geometry::curl()` | 3-D curl, with `divergence`/`laplacian` identities as tests | vector-calculus completion |
| `geometry::directional(direction)` | `v . grad f` | convenience over `gradient` and `multiply` |

**Verification oracles (independent of the implementation):**

- Unit sphere, coordinates `(θ, φ)`, `g = diag(1, sin²θ)`: `levi_civita`
  reproduces the textbook symbols `Γ^θ_{φφ} = -sinθ cosθ`,
  `Γ^φ_{θφ} = cotθ` exactly (symbolic comparison against hand-written
  Functions evaluated on a grid).
- Parallel transport of a tangent vector around a latitude circle returns a
  vector rotated by the enclosed solid angle `2π(1 - cosθ0)`. The change is
  obtained by integrating `DV/dt = 0` with the library's own ODE engine and
  compared with the closed-form holonomy angle.
- Metric compatibility: `d(g_{ij} V^i W^j)/dt = g_{ij}(DV^i/dt W^j + V^i DW^j/dt)`
  to rounding, for random fields, in 2-D and 3-D, which is a property test of
  the sign conventions for contravariant versus covariant indices.
- Geodesic equation: great circles satisfy `D(dx/dt)/dt = 0`; a non-geodesic
  latitude circle does not.
- Flat space in polar coordinates: `levi_civita` of `diag(1, r²)` gives
  `Γ^r_{θθ} = -r`, `Γ^θ_{rθ} = 1/r`, and the absolute differential of the
  Cartesian-constant vector field written in polar components is zero.
- Signature `(0,0)` equals `differential` along the curve (chain rule).
- Torsion-free symmetry `Γ^i_{jk} = Γ^i_{kj}` of the `levi_civita` output.

**Reuse record.** Differentiation and broadcasting reuse the
existing `differential`, `jacobian`, `multiply` and registry primitives;
inverse/solve/Cholesky evaluation reuses `Series.linearalg`;
parallel transport is verified through `pluss::subsp::math::ode`; NatSci
`common::spatial` metric handling is read, not duplicated. No new numerical
kernel is permitted; the new primitives only wrap `Series.linearalg`/`Series` ops.

### Tranche 12: Stochastic Operators

**Boundary (supersedes the earlier "Stochastic Operator Boundary").** A
stochastic operator is an ordinary `Operator`. It is **path-functional**: the
`Function` it returns has the same free variables as its input plus the
requirement that the time and noise variables be bound to Series sampled on a
shared *path axis* (the last axis by default, configurable through the `path_axis` option).
Evaluation of such a `Function` on arguments that do not satisfy the contract
raises `pluss::Error` naming the violated condition. Nothing about the
deterministic `integral(...)` changes meaning when its argument is random; a
user who wants a stochastic integral must say so by calling a function in the
`stochastic` namespace.

**Conventions.** One enum selects the integral's meaning, so a convention is
never inferred:

```cpp
enum class Convention : std::uint8_t {
  Ito,            // left endpoint, non-anticipating (martingale, isometry)
  Stratonovich,   // midpoint/trapezoid, ordinary chain rule
  Klimontovich,   // right endpoint (backward / anti-Ito)
  Theta,          // f evaluated at (1-θ) left + θ right; θ in [0,1]
  Marcus,         // jump-aware, canonical for jump diffusions
};
```

`Theta` with `θ = 0`, `1/2`, `1` reproduces Ito, Stratonovich, Klimontovich and is
the single implementation path for the three; they are named only so that code
reads correctly.

**Public surface.**

```cpp
namespace stochastic {
struct IntegralOptions {
  Convention convention = Convention::Ito;
  double theta = 0.0;               // used when convention == Theta
  bool cumulative = false;          // running integral along the path vs terminal value
  bool check_adapted = true;        // run the sde:: adaptedness probe (Ito only)
  std::int64_t path_axis = -1;          // the shared sampled path axis
};
[[nodiscard]] Operator stochastic_integral(std::string_view time, std::string_view noise,
                                           IntegralOptions options = {});
[[nodiscard]] Operator stochastic_differential(const math::Function& drift,
                                               const math::Function& diffusion,
                                               Convention convention = Convention::Ito);
[[nodiscard]] Operator generator(const math::Function& drift,
                                 const math::Function& diffusion,
                                 std::string_view state);
[[nodiscard]] Operator quadratic_variation(std::string_view noise, std::int64_t path_axis = -1);
[[nodiscard]] Operator covariation(std::string_view first, std::string_view second, std::int64_t path_axis = -1);
[[nodiscard]] Operator malliavin_derivative(std::string_view noise, std::int64_t path_axis = -1);
[[nodiscard]] Operator skorokhod_integral(std::string_view noise, std::int64_t path_axis = -1);
[[nodiscard]] Operator convention_change(Convention from, Convention to,
                                         const math::Function& diffusion, std::string_view state);
}
```

- `stochastic_integral(t, W)` applied to `f(t, W, ...)` gives the integral
  `∫ f(t, W_t, ...) dW_t` as a `Function` evaluated on sampled `t` and `W`.
  Evaluation delegates to the existing `Series::antidiff(w).ito()/strat()`
  paths and `sde::marcus_integral`; no new integration loop is written.
- `stochastic_differential(μ, σ, convention)` is a **purely structural**
  operator. For an SDE `dX = μ dt + σ dW` and a smooth function `g(x)` it
  returns the drift and diffusion coefficients of `g(X)` as a `Function` of
  output shape `[2]`: under Ito, `(g'μ + ½σ²g'', g'σ)` (Ito's formula); under
  Stratonovich, `(g'μ, g'σ)` (ordinary chain rule). Derivatives are produced
  by the existing `differential`, so higher-dimensional state follows from
  `gradient`/`hessian` (matrix form `½ tr(σσᵀ ∇²g)`).
- `generator(μ, σ, x)` is the infinitesimal generator
  `L g = μ·∇g + ½ tr(σσᵀ ∇²g)`, a deterministic, structural, **linear** operator.
  It composes with every other operator (`compose`, `linear_combination`,
  `commutator`), which gives Dynkin's formula and the backward Kolmogorov
  equation `∂u/∂t = L u` for free, and its formal adjoint (Fokker-Planck) is
  supplied as `adjoint_known` where μ and σ are structural.
- `convention_change` applies the Ito/Stratonovich correction
  `μ_Strat = μ_Ito - ½ σ ∂σ/∂x` (and its matrix form), the standard
  conversion, as a structural operator on drift Functions.
- `quadratic_variation` and `covariation` evaluate `[X]_t = Σ (ΔX)²` on a
  sampled path (cumulative or terminal) through the existing
  `StochasticIntegralResult` quadratic-variation helper.
- `malliavin_derivative` and `skorokhod_integral` delegate to
  `Series::calc.malliavin/skorokhod` (the weak-derivative and its adjoint on
  the sampled Wiener increments) and carry the sampling-grid contract.

**Structural rules for path-functional nodes.** A new private node kind
(`PathFunctional`) records the child integrand, the time and noise variable
ids, the convention and θ. Its derivative rules are deliberately narrow:

- Differentiation with respect to a **parameter** (not time, not noise) is
  allowed by pathwise exchange: `∂θ ∫ f dW = ∫ ∂θ f dW`, and the convention is
  preserved. This is valid for integrands smooth in the parameter and is
  documented as a pathwise derivative, the same notion `sde::fluctua_adjoint`
  already uses.
- Differentiation with respect to the **noise or time variable** of the node is
  rejected with an error that names the Malliavin operator as the correct
  tool. It is never silently approximated.
- Linearity in the integrand and additivity in the integral hold; scalar
  constants factor out. Simplification may use `∫ c dW = c (W_T - W_0)` only
  when the integrand node is a constant.
- Composition with deterministic operators on the *output* is allowed
  (`differential("a")(stochastic_integral(...)(f))`), composition that would
  differentiate through the path variable is not.
- The node is not serializable in the first release (the persisted forms are
  the factory descriptors and the integrand Function); the descriptor persists
  `convention`, `theta`, and variable names, so a saved operator reloads.

**Verification oracles.** Each uses an identity or reference independent of the
library's own integrator:

1. `∫₀ᵀ W dW = (W_T² - T)/2` pathwise in the Ito discretisation (exact
   discrete identity `Σ W_i ΔW_i = (W_T² - Σ ΔW_i²)/2`) and `= W_T²/2` for
   Stratonovich; the θ-family interpolates linearly in θ.
2. Ito isometry `E[(∫ f dW)²] = E∫ f² dt` and martingale mean zero, checked by
   Monte Carlo with a confidence interval, not an equality.
3. Ito's formula: for `g = x²`, `exp(x)`, `sin(x)` and `X` geometric Brownian
   motion, `stochastic_differential` agrees with `ΔG(X)` computed from a
   simulated path with the scheme error scaling as the Euler order.
4. Convention conversion: simulating the Stratonovich SDE with the Heun
   scheme and the converted Ito SDE with Euler-Maruyama gives the same
   distribution (two-sample test) and the same moments for geometric
   Brownian motion (`E[X_T] = X0 exp(μT)` under both readings of the correct
   drift).
5. Generator checks: for Ornstein-Uhlenbeck `dX = θ(m - X)dt + σ dW`,
   `L x = θ(m - x)` and `L x² = 2θx(m - x) + σ²` in closed form; Dynkin's formula `E[g(X_T)] - g(X_0) = E∫ L g(X_s) ds` by Monte Carlo;
   backward Kolmogorov solved by the library's PDE route for an analytically
   solvable case (Black-Scholes heat-equation reduction).
6. Quadratic variation `[W]_T → T` with the discretisation error of order
   `sqrt(h)`, and `[X] = ∫σ² dt` for an Ito process; covariation of
   independent paths tends to zero.
7. Malliavin chain rule `D_t F(W_T) = F'(W_T)` and Skorokhod adjoint duality
   `E[F δ(u)] = E[∫ D_t F u_t dt]`, from the existing Series implementation
   re-derived through the operator interface.
8. Adaptedness probe rejects an anticipating integrand (`f = W_T`) when
   `check_adapted` is on, and the error says why.
9. Marcus versus Ito on a compound-Poisson-driven path: the Marcus correction
   term equals the closed form for a single jump of known size.

**Reuse record.** The following are reused unchanged and may not be
re-implemented: `sde::simulate_wiener_path`, `sde::ito_integral`,
`sde::strat_integral`, `sde::marcus_integral`, `sde::fluctua`,
`sde::fluctua_adjoint`, the adaptedness check, and `Series::calc.diff.ito/strat`,
`calc.antidiff.ito/strat`, `calc.malliavin`, `calc.skorokhod`. The new code is the
operator wrapper, the `PathFunctional` node, the structural Ito/Stratonovich
algebra (drift and diffusion transformation, which is *deterministic*
calculus on `Function`), and the contracts above.

#### Tranche 12 Record (first slice complete, 2026-10-28)

Where the built surface differs from the plan above, **the built surface is
normative**:

- `Convention` is `{Ito, Stratonovich, Klimontovich, Theta}`; `Marcus` is not
  an enumerator yet (an unimplemented option is not published). Marcus
  remains available through `sde::marcus_integral` on a jump path.
- `stochastic_integral(noise, IntegralOptions)`: the `time` argument was
  dropped (the sum uses only the noise increments; an unused name would have
  been an inert parameter). `IntegralOptions{convention, theta, cumulative,
  path_axis}`; `PathOptions{cumulative, path_axis}` for the variations.
- **Adaptedness is structural, not probed.** The integrand is a pointwise
  `Function` of the sampled variables and every `Function` operation is
  elementwise along the path axis (tensor operations act on output axes), so
  the Ito sum is non-anticipating by construction. An opaque callable, which
  could look ahead, is rejected when the operator is applied; integrands
  computed from the whole path go through `sde::ito_integral` and its
  randomised probe. This is a guarantee instead of a sampled check.
- **Path axis convention** (the design note the plan asked for): the sampled
  variables are rank-0 Functions, so their Series have only batch axes; the
  path axis is one of those batch axes (default last), every other axis is an
  independent path, the integrand must be scalar-valued, and the result has
  the path axis summed out (`cumulative` keeps it, starting at 0). This is
  independent of output axes because path functionals only accept
  rank-0 operands.
- Implementation: two registry primitives, `PathIntegral` and
  `PathCovariation` (attribute-carrying, so they serialize, simplify, rebind
  and compose like every other node; `Capabilities::path_functional` was
  added and combines by OR). Derivative rules: pathwise exchange
  `d/dv int F dW = int (dF/dv) dW` for every variable except the noise
  (rejected with an error naming the Malliavin derivative); the covariation
  has the product rule, so `d[X] = 2 [X, dX]`.
- Structural stochastic algebra (no sampling, deterministic calculus on
  `Function`): `generator` (one state, or n states with an [n] drift and
  [n, m] diffusion), `stochastic_differential` (`a = drift.grad g + (1/2 -
  theta) tr(sigma sigma^T Hess g)`, `b = sigma^T grad g`; theta = 0 is Ito's
  formula, 1/2 the chain rule, 1 the backward rule) and `convention_change`
  (`drift + (theta_from - theta_to) c`, `c_i = sum sigma_jk d_j sigma_ik`).
  The theta formula was derived (Ito form `drift + theta c`, then conversion
  of `g(X)` back with the y-derivative of `b`), and is checked against that
  derivation in the tests.
- Verified (`stochastic_operators_test.cpp`, 13 cases): exact discrete
  identities on a sampled path (Ito `(W_T^2 - [W])/2`, Stratonovich
  `W_T^2/2`, Klimontovich, theta linearity, Ito-Stratonovich gap = `[W]/2`);
  agreement with `sde::ito_integral/strat_integral` and with the Series theta
  integral; cumulative path; Monte Carlo martingale mean and isometry (1e5
  paths, error bars stated); quadratic variation vs the sde engine, `[W]_T ~ T`,
  covariation of independent paths, `[int W dW] ~ int W^2 dt`; pathwise
  parameter derivative and the rejected noise derivative; serialization
  round trip; generators of Ornstein-Uhlenbeck, geometric Brownian motion and
  a 2-D linear diffusion against closed forms; Ito/chain-rule/backward
  coefficients; multi-dimensional conversion; and a simulation showing the
  Stratonovich SDE and its converted Ito SDE have the same mean while the
  unconverted Ito reading does not.
- **Superseded by the Analytic Result Rule.** A second slice built
  `malliavin_derivative`, `skorokhod_integral` and `marcus_integral` as
  *evaluated-only* operators (a new opaque graph node holding a callable).
  Under the rule above they were **removed** along with the node: they are
  computed from a sampled path (a reverse pass; interpolation along jump
  segments) and have no graph form, so they remain on `Series::calc`
  (`calc.diff.malliavin_path`, `calc.diff.skorokhod`, `calc.antidiff.marcus`).
  Their identities were verified during that slice (`D(W_T^2) = 2 W_T`, the
  exact discrete `D_j int W dW = W_T - dW_j`, `delta(W_T) = W_T^2 - T`, the
  Malliavin/Skorokhod duality) and are covered by the Series tests. An
  *analytic* Malliavin derivative would need path-structural graph nodes
  (a reverse cumulative sum along the path, so that `D_j` of a structural
  path functional is itself a graph); that design is open and not started.
- **A Series bug found and fixed at the source** while doing this: in
  `calc.diff.skorokhod` and `calc.diff.malliavin(_path)` the "does anything
  depend on the increments" guard tested a slice of the process, which can
  carry a source link of its own while its sum has nothing to differentiate,
  so a process that does not depend on the increments (a constant, or one
  built with `zeros_like`) raised "nothing to differentiate". The guard now
  tests the summed objective; a regression test is in
  `PlussQit/tests/CodeM/test_patch95_stochastic_calc.cpp` (not built in this
  session's build tree).
- Serialization: `stochastic_integral` and `quadratic_variation` hold only
  names and numbers and save and load (bit-exact doubles); `generator`,
  `stochastic_differential`, `convention_change` and `covariation` hold
  Functions and are not serializable. Their results, when structural,
  serialize as Functions.

### Tranche 12b: Expansion Operators (Taylor And Fourier Series)

Added 2026-10-28. Both families turn a Function into a truncated series and
are the natural bridge from this module to the COS method in
`Math/Finance/pricing/cos_pricing.cpp`, which today hard-codes the cosine
coefficients of exp(y) and of a constant (`cos_chi_`, `cos_psi_`) and loops over
`k` in scalar code.

**Taylor operators** (a new `expansion` family; structural, built from
`differential`, `shift` and the algebra, so no new graph node):

```cpp
namespace expansion {
[[nodiscard]] Operator taylor(std::string_view variable, double center, std::int64_t order,
                              DiffOptions options = {});                 // polynomial Function in `variable`
[[nodiscard]] Operator taylor_coefficients(std::string_view variable, double center,
                                           std::int64_t order, DiffOptions options = {}); // f^(k)(a)/k!, shape [order+1]
[[nodiscard]] Operator taylor_remainder(std::string_view variable, double center,
                                        std::int64_t order, DiffOptions options = {});    // f^(n+1) / (n+1)!  (Lagrange form, to bound the error)
[[nodiscard]] Operator taylor(pluss::StackArray<std::string_view> variables,
                              pluss::RangeView<double> center, std::int64_t order,
                              DiffOptions options = {});                 // several variables, total degree <= order
}
```

`taylor` returns the polynomial `sum_k f^(k)(a)/k! (x-a)^k` as a `Function`
(so it can be integrated, differentiated and composed like any other);
`taylor_coefficients` returns the coefficient vector; the remainder operator
returns the Lagrange term so that an error bound is `sup |remainder| |x-a|^(n+1)`
over an interval. Multi-variable expansion uses `gradient`/`hessian` and
higher mixed derivatives through repeated `differential`, truncated at total
degree `order` (the node budget `max_nodes` already guards the growth).
Oracles: exp, sin, cos, log(1+x), (1+x)^p against their known series; the
expansion of a polynomial of degree <= order is exact; derivative of the
Taylor polynomial equals the Taylor polynomial of the derivative with order
n-1; error shrinks as `h^(n+1)` and respects the Lagrange bound; the
multivariable expansion of exp(x+y) and of x*y*z; Taylor of a composition via
`compose`; and, as the Ito-Taylor link, the stochastic generator's
`L^k g` terms reproduce the first Wagner-Platen terms in the stochastic
tranche's tests.

**Taylor and power-series operators: built (2026-10-28).** The Taylor half of
this tranche is complete, and a power-series layer was added first (on the
user's suggestion) because the structural route builds a derivative graph that
grows with the order (the module's `max_nodes` guard exists for that reason):

- `expansion::taylor(variable, center, order, DiffOptions, Engine)` and the
  multivariable overload (total degree <= order), `taylor_coefficients`,
  `taylor_remainder` (Lagrange factor). `Engine::{Auto, Structural, Jet}`.
  Structural is exact, differentiable and saveable; **Jet** carries truncated
  power series through the graph with the standard recurrences
  (product/quotient convolution, exp, log, sqrt, constant and general powers,
  sin/cos/sinh/cosh, tan/tanh) so the cost is O(order^2) per node, with no
  graph growth, any order (tested to 30), and batched over the free
  variables. **The jet result is an analytic graph** (built from graph nodes,
  O(order^2) of them, differentiable, composable and saveable), not a
  numerical evaluation; the expanded variable is replaced by its centre, so
  the coefficient Function depends on the other variables only. Measured:
  building the order-16 jet of exp takes 0.2 ms (the first version built
  through Function operations took 39 ms, because every operation rebuilt a
  plan; building on nodes, as the derivative engine does, removed that). Auto uses Structural up to order 8 and Jet above it when the
  Function is scalar and elementwise.
- Coefficient-vector operators (a truncated power series is its coefficient
  vector, a rank-1 Function): `power_series(variable, center)` (evaluate),
  `series_derivative`, `series_integral`, `series_product` (Cauchy product).
  `taylor` with the Jet engine is literally `power_series` applied to the
  jet coefficients.
- Verified (`taylor_operators_test.cpp`, 11 cases): exp, sin, log1p, (1+x)^p,
  1/(1-x), tan, tanh, sqrt(1+x), exp(sin x) and log at a shifted centre
  against their known series; polynomial exactness; `d/dx T_n = T_{n-1}[f']`;
  Lagrange bound and error scaling `h^(n+1)`; multivariable expansions;
  jet vs derivative-graph agreement to 1e-11; batched jets; termwise
  derivative/integral/product identities (`sin x cos x = sin(2x)/2`,
  `exp(x) exp(-x) = 1`); serialization of operators and of results;
  rejection of tensor-valued, opaque and singular cases.
- Not done: a radius-of-convergence estimator (needs a max over the tail of
  the coefficients; it will be an evaluated operator), series composition
  and reversion, and the Ito-Taylor (Wagner-Platen) expansion built on the
  generator.

**Fourier series operators** (family `series`, evaluated through the existing
integral engine; `fourier` transforms in Tranche 15 are a different object):

```cpp
namespace series {
// half-range expansions on [a, b]
[[nodiscard]] Operator cosine_coefficients(std::string_view variable, double lower, double upper, std::int64_t terms);
[[nodiscard]] Operator sine_coefficients  (std::string_view variable, double lower, double upper, std::int64_t terms);
[[nodiscard]] Operator cosine_series(std::string_view variable, double lower, double upper, std::int64_t terms);
[[nodiscard]] Operator sine_series  (std::string_view variable, double lower, double upper, std::int64_t terms);
// full trigonometric series on [-L, L] = cosine series of the even part + sine series of the odd part
[[nodiscard]] Operator fourier_coefficients(std::string_view variable, double half_period, std::int64_t terms);
[[nodiscard]] Operator fourier_series(std::string_view variable, double half_period, std::int64_t terms);
}
```

Definitions (period 2L, `u_k = k pi / (b - a)`): cosine coefficients
`A_k = 2/(b-a) int_a^b f(x) cos(u_k (x-a)) dx` (the k = 0 term halved in the
sum), sine coefficients `B_k = 2/(b-a) int_a^b f(x) sin(u_k (x-a)) dx`, and the
full series on `[-L, L]` has `a_k = (1/L) int f cos(k pi x/L) dx` and
`b_k = (1/L) int f sin(k pi x/L) dx`, which is exactly the cosine series of the
even extension plus the sine series of the odd extension, so
`fourier_series = cosine_series(f_even) + sine_series(f_odd)` is a test, not a
convention. Coefficient operators return a vector `Function` of shape
`[terms]`; series operators return the truncated sum as a `Function` of the
variable. Coefficient integrals use the structural antiderivative where the rule
table has one (polynomial times trigonometric is added to the table in this
tranche) and the existing quadrature otherwise; all `terms` coefficients are
computed in one batched quadrature, not a loop over `k`.

COS integration (the motivating use): the cosine coefficients of a payoff
`v(y)` on `[a, b]` are the `V_k` of the COS method, and the coefficients of a
density from its characteristic function are
`F_k = 2/(b-a) Re(phi(u_k) exp(-i u_k a))`. The tranche
(i) reproduces `cos_chi_`/`cos_psi_` as `cosine_coefficients` of `exp(y)` and
of the constant on `[c, d]` to 1e-12 (the Finance code is the independent oracle),
(ii) prices a vanilla call through the generic path with a user payoff
`Function` and matches `option_price_cos` to the stated tolerance, and
(iii) then prices payoffs that have no hand-coded `chi/psi` (digital, power,
capped call, butterfly) against closed forms (Black-Scholes) and cross-checks
against `integ` quadrature. Replacing the scalar loops in `cos_pricing.cpp`
by the batched coefficient operator is a *follow-up refactor of Finance*, done
only after the generic path matches the existing prices; it is not part of this
tranche's acceptance.

Other oracles: sine series of `x` on `[0, pi]` is `2 sum (-1)^(k+1) sin(kx)/k`;
cosine series of `x` is `pi/2 - (4/pi) sum cos((2k-1)x)/(2k-1)^2`; a square
wave's Gibbs overshoot is about 8.95% and does not decay with `terms`; Parseval
`int f^2 = (b-a)/2 (A_0^2/2 + sum A_k^2)`; uniform convergence of the cosine
series of a continuous function, and termwise-differentiated sine series of a
smooth odd function; convergence rate of the cosine coefficients is `O(k^-2)`
for kinked payoffs and `O(k^-1)` for jumps, matching COS-method theory.

**Fourier series operators: built (2026-10-28).** The family is
`operators::fourier` (not `series`, which would clash with the `pluss::series`
factory namespace): `cosine_coefficients`, `sine_coefficients` (each with an
overload that integrates over a sub-interval of the expansion interval while
the phase stays anchored at `lower`, the form the COS payoff coefficients
need), `cosine_series`, `sine_series`, and on `[-L, L]` `coefficients` and
`series`. `cosine_coefficients` returns `A_0..A_{terms-1}`, `sine_coefficients`
`B_1..B_terms`, the full `coefficients` `[a_0..a_{terms-1}, b_1..b_{terms-1}]`.

- Every coefficient is a definite integral of `f` against a trigonometric
  function built through the existing `integral` operator, so the results
  are analytic Functions: coefficient vectors depend on the other variables
  of `f` only; a series is a Function of the expanded variable, linear in
  `f`, differentiable in the parameters, serializable (operator and result).
  Unresolved integrands use the integral engine's lazy quadrature (the one
  accepted numerical fallback); exact antiderivatives are used where the rule
  table has them.
- Verified (`fourier_series_operators_test.cpp`, 8 cases, 170 assertions):
  sine and cosine coefficients of `x` and of `x(pi - x)` against closed forms
  to 1e-8; Parseval (sine to the truncated-tail size); the pointwise error
  shrinks as terms are added; the full series on `[-1, 1]` for `exp` against
  its closed-form `a_k`, `b_k`, equal to the cosine series of `cosh` plus the
  sine series of `sinh`, with the parity relation `S(-x) = C(x) - S_odd(x)`;
  the Gibbs overshoot of the square wave (peak 1.179, non-decaying); free
  variables stay free and `d/da` of the series is the series of `d f/da`;
  validation; serialization of all seven operators.
- **COS cross-check passes.** The payoff coefficients of the call over
  `[0, b]` equal the hand-coded `chi_k`/`psi_k` formulas of
  `Math/Finance/pricing/cos_pricing.cpp` to 1e-8 for 24 terms, and the COS
  method driven only by the new operator (96 terms, truncation `+-10` standard
  deviations) prices a vanilla call and a cash-or-nothing digital to 1e-6 and a
  power payoff `S_T^2` to 1e-4 relative against closed-form Black-Scholes. The
  digital and power payoffs have no hand-coded coefficients in the Finance
  code, which is the point of the generic path. Replacing the scalar loops in
  `cos_pricing.cpp` is still the Finance refactor noted above, not done.
- Performance (tracked as we go): coefficient `k` is its own scalar integral
  node, so evaluating `K` coefficients runs `K` quadratures (about 4 ms each
  here; the 96-term payoff coefficients for three payoffs plus the rest of the
  suite take 3.4 s). The follow-up is a vector-valued integral node that
  evaluates all coefficients in one batched quadrature call (the quadrature
  engine already batches), which needs the structural integral to accept
  output rank 1; not done.

References for this tranche (to be obtained and hashed per the standing rule;
none read yet): F. Fang and C. W. Oosterlee, "A novel pricing method for
European options based on Fourier-cosine series expansions", SIAM J. Sci.
Comput. 31 (2008) 826-848 (COS method; also the arXiv/TU Delft open version
where obtainable); Zygmund, *Trigonometric Series*; Folland, *Fourier Analysis
and Its Applications*; Kloeden and Platen for the Ito-Taylor expansion
(Wagner-Platen).

### Tranche 12c: Integral Transforms (Fourier, Laplace) — done

`fourier::transform / inverse_transform / cosine_transform / sine_transform` (Angular, Unitary,
Ordinary conventions) and `laplace::transform`. Each returns an analytic Function of a **new free
frequency variable** (`FunctionAccess::extend` re-bases the graph onto a trace with one more
variable; integrals and bound variables carry over) with the integration variable bound.
`integral` now accepts infinite limits; an infinite range has no primitive to substitute into, so it
stays an unresolved Integral node evaluated lazily by `integ::quadra` (the accepted exception to the
analytic-result rule). The result differentiates (in the frequency and in the Function's parameters)
and saves/loads (`fourier_*_transform`, `laplace_transform` keys).

Verified against closed forms: Gaussian (all conventions), shifted Gaussian (phase), exponential
cosine/sine transforms, Laplace of exp, t^2, sin, cos, derivatives in s and a, inverse recovery from an
analytic spectrum, io round-trip.

`laplace::inverse_transform(s, t, terms = 32)` is Talbot inversion on Weideman's optimized contour: a sum of
`terms / 2` compositions of F with constants over t, so unlike the other transforms it is a fully **structural**
graph (no quadrature at evaluation). A direct Bromwich integral was tried first and rejected: the oscillatory,
algebraically decaying integrand does not converge under the infinite-domain substitution. Verified: t e^{-t},
sin 3t (poles on the imaginary axis, accuracy degrades for large t), 1/sqrt(pi t) (branch cut), a parameter,
the t-derivative, io round trip.

Public calls: `operators::laplace::inverse_transform`, `operators::fourier::{transform, inverse_transform, cosine_transform, sine_transform}`,
`operators::fourier::Convention`, `operators::laplace::transform`.

Known limits: a transform of a transform nests two quadratures and fails to converge (inner
oscillatory integral at the huge frequencies the outer substitution reaches); no closed-form
(structural) table yet for Laplace/Fourier pairs; inverse Laplace is Talbot's method (below), not a Bromwich quadrature.

### Tranche 13: Measure-Theoretic Operators (Lebesgue)

**Why this is a different operator.** The Riemann/quadrature operators
partition the **domain**; the Lebesgue integral partitions the **range**:

```text
∫_A f dμ  =  sup over simple functions s <= f of  Σ_k c_k μ(A ∩ {s = c_k})
          =  ∫_0^∞ μ(A ∩ {f > t}) dt                         (f >= 0, layer-cake)
```

so the primitive is the **measure of a super-level set**, and the integral is
defined for integrands that are not Riemann integrable and for measures other
than Lebesgue measure (atomic, singular, mixed). Pluss has no measure
vocabulary today, and this tranche deliberately does not create one as a
type: measures are expressed through the operator that integrates against them
and through the existing operator algebra, and the distribution layer supplies
densities and CDFs.

**There is no measure type.** A measure is carried by the operator that
integrates against it, and measure algebra is the existing operator algebra:

```cpp
namespace measure {
struct LebesgueOptions {                // plain data, no behaviour
  std::int64_t levels = 0;              // 0 = adaptive range slicing
  double tolerance = 0.0;               // 0 = dtype default
  std::int64_t max_levels = 1 << 14;
  RangeView<double> null_points{};      // finite μ-null set: a.e. semantics
};
// one factory per measure kind, all returning operators::Operator
[[nodiscard]] Operator lebesgue(std::string_view variable, double lower, double upper,
                                LebesgueOptions options = {});                         // dμ = dx
[[nodiscard]] Operator weighted(std::string_view variable, const math::Function& density,
                                double lower, double upper, LebesgueOptions options = {}); // dμ = ρ dx
[[nodiscard]] Operator atomic(std::string_view variable, RangeView<double> points,
                              RangeView<double> weights);                               // Σ w_i δ_{x_i}
[[nodiscard]] Operator stieltjes(std::string_view variable, const math::Function& cdf,
                                 double lower, double upper, LebesgueOptions options = {}); // dμ = dF, jumps included
[[nodiscard]] Operator measure_of(std::string_view variable, double lower, double upper,
                                  LebesgueOptions options = {});                         // μ({x : f(x) != 0}) of an indicator
}
```

Everything else a measure type would provide already exists:

| Measure operation | How it is expressed |
| --- | --- |
| mixture `a μ + b ν` | `linear_combination({{a, I_mu}, {b, I_nu}})` (the integral is linear in the measure) |
| restriction `μ(· ∩ A)` | `multiply(indicator)` followed by the integral operator, or `measure_of` for the set itself |
| product measure / Fubini | `compose(I_y, I_x)`, with the order chosen by the caller; integrability failure raises an error |
| Radon-Nikodym derivative `dν/dμ` | ordinary `math::Function` division of the two density Functions (no operator at all); mutually singular supports are a domain error at evaluation |
| expectation under a law | `weighted(x, pdf, ...)`, `stieltjes(x, cdf, ...)` or `atomic(...)` built from the Distribution layer's pdf/cdf/pmf |
| change of measure | `multiply(dnu_dmu)` then the integral against `mu` |

`lebesgue(x, a, b)(f)` returns a `Function` of the remaining free variables
holding `∫_{[a,b]} f dμ`. `measure_of` returns `μ({x ∈ [a,b] : f(x) ≠ 0})` for an indicator-valued
`f`: the object the layer-cake needs and a useful result on its own. A
probability law is exactly a `weighted`, `stieltjes` or `atomic` operator built
from its pdf, cdf or pmf, which connects expectation to the Distribution/CGF
integration of Phase 7 without a new type.

**Evaluation algorithm (range partitioning, vectorised over batch axes).**

1. Evaluate `f` on a refinable analysis grid of the domain (reusing the
   existing Series vectorised evaluation) and bound its range `[m, M]`.
2. For each level `t_k`, compute `μ(A ∩ {f > t_k})`: for Lebesgue and density
   measures from exact bracketed roots of `f - t_k` between sign changes
   (reusing `math::roots`), summing density mass between roots by the CDF or
   by quadrature of the density; for atomic and counting measures by exact
   masking and summation (no quadrature at all); for Stieltjes by CDF
   differences, with jump contributions counted from the atoms of the CDF.
3. Assemble `Σ t_k Δμ_k` with the standard lower/upper simple-function
   bracket, refine the number of levels until the lower and upper sums
   differ by less than the tolerance, and report the bracket. Non-convergence
   is an error that carries the bracket, in the same spirit as the quadrature
   integral node; the result is never silently returned as exact.
4. Signed integrands use `f = f⁺ - f⁻`. Complex integrands integrate real and
   imaginary parts.
5. Product measures iterate the one-dimensional operator in the order that
   Tonelli permits; Fubini is checked by integrability diagnostics, and a
   failure raises an error rather than assuming interchange.

**Convergence-theorem diagnostics.** `lebesgue` reports, in the evaluation
metadata, whether the monotone-convergence or dominated-convergence
conditions held across refinement levels (the sequence of level sums is
monotone and bounded by an integrable envelope). They are diagnostics
about the numerical sequence, not proofs.

**Relationship to existing operators.** For a continuous bounded integrand on
a finite interval with Lebesgue measure the result agrees with
`integral(x, a, b)` to the stated tolerance; that equality is an acceptance
test, not a shortcut: the operators remain separate because the Lebesgue
operator also covers atomic, singular and mixed measures, a.e. modification,
and unbounded or oscillatory integrands where range slicing succeeds and
domain quadrature does not.

**Verification oracles:**

1. Agreement with the closed-form integrals used by `integral` for smooth
   `f` and Lebesgue measure.
2. A.e. invariance: changing `f` on a declared null set (a finite point set)
   leaves the integral unchanged; the Riemann-quadrature operator can be shown to
   differ on a spike that the Lebesgue operator correctly ignores.
3. Atomic measure: `∫ f dμ = Σ w_i f(x_i)` exactly (bitwise reproducible
   summation order stated).
4. Lebesgue-Stieltjes with a CDF that has a continuous part and jumps:
   `∫ f dF = ∫ f F' dx + Σ f(x_j) ΔF(x_j)` against the closed form for a
   mixed law built from the Distribution layer.
5. Expectation of a transformed random variable against the Distribution
   layer's own moment and CGF results (normal, exponential, discrete).
6. Layer-cake identity `∫ f dμ = ∫₀^∞ μ({f > t}) dt` evaluated both ways.
7. Monotone convergence: `f_n ↑ f` gives `∫ f_n ↑ ∫ f`; Fatou's inequality
   for a non-monotone sequence; dominated convergence with the diagnostic on.
8. Radon-Nikodym chain rule `dν/dλ = (dν/dμ)(dμ/dλ)` and `dμ/dμ = 1`.
9. Tonelli on a product of an atomic and a Lebesgue measure against the
   iterated sum-integral; a deliberately non-integrable function reports the
   Fubini failure.
10. Unbounded domain and singular integrands (`1/√x` on `(0, 1]`) where range
    slicing converges with a reported bracket.

**Reuse record.** Reuses `math::roots` for level-set endpoints, the Quadra
engine for density-mass between roots, the Distribution layer for pdf/cdf, and
Series reductions for atomic sums. No new quadrature or root-finding routine.

### Tranche 13 — implementation record (done, with a deliberate change of scope)

Built: `operators::measure::{lebesgue, weighted, atomic, stieltjes}`. All return analytic Functions:
`atomic` is an exact finite sum of the integrand with the variable fixed at each point
(`FunctionAccess::fix`: same trace, variable bound); `lebesgue`/`weighted`/`stieltjes` are unresolved
integrals split at the declared null points (so null points are quadrature endpoints and are never sampled)
and evaluated lazily; `stieltjes` takes the jump-free part of the CDF (its derivative is the density) plus
explicit jump points and sizes. Mixtures are `linear_combination`, change of measure is `multiply` (now
matching the coefficient's variables to the input's by name), product measures are `compose`.

**Not built: the range-slicing (layer-cake) evaluator described above.** Pinning it down showed that with a
finite range partition the lower/upper simple-function sums are the midpoint rule rearranged, so an operator
for it would be numerically a quadrature rule under another name, and any value it returned would be eager,
which the analytic-result rule forbids for operators. The capability that is genuinely new is the measures
(atomic, mixed, Stieltjes, a.e. sets, products), and that is what is implemented. Singular integrands such as
`1/sqrt(x)` on `(0, 1]` are not handled: the default Simpson integral node samples the endpoint and the
algebraic singularity defeats Gauss rules; they need a change of variable by the caller. `measure_of`
(measure of a super-level set) and the convergence-theorem diagnostics are not built; they would need an
eager root-bracketing evaluator, which belongs in `Series::calc`/`integ`, not in an operator.

Verified: Lebesgue integral equals the closed form (also with a free parameter), null points, atomic
sums exact to the last bit, second moment of the normal pdf, a mixed continuous-plus-atomic Stieltjes measure,
mixture, Radon-Nikodym change of measure (`3x^2` density), Fubini on a product measure.
Public calls: `operators::measure::{lebesgue, weighted, atomic, stieltjes}`.

### Addenda after Tranche 13

**Eager Lebesgue integral (`integ::lebesgue`, `integ::measure_of`, `integ_lebesgue.h`).** Built as an eager
`integ` function, not an operator (operators return analytic Functions). The range of the non-negative parts
of the integrand is partitioned geometrically, `t_k = y_min (1 + eps)^k`, so the lower/upper simple-function
bracket is within a factor `1 + eps` of the integral: a relative bracket, which stays tight for tall, thin
integrands where a uniform partition of `[0, max f]` would need astronomically many levels (a first uniform
version failed on `1/sqrt(x)` exactly this way). Cells are graded (`x = a + (b - a) u^g`) and doubled until the
value and bracket settle; the result carries `lower`, `upper`, `error_estimate`, the refinement `history` and a
`monotone` flag (the monotone-convergence signature, a diagnostic and not a proof). `measure_of` locates the
level-set boundary inside a cell by linear interpolation (second order). Verified: sin, signed, batched rows,
a.e. spike ignored (Simpson, whose nodes hit the spike, is off by more than 1), `1/sqrt(x)` on a graded mesh,
layer-cake identity `int f = int mu(f > t) dt`, `mu(x^2 > t)`, monotone convergence of `min(x^-1/2, n)`.
Public calls: `integ::lebesgue`, `integ::measure_of`, `integ::LebesgueOptions/LebesgueResult/MeasureResult`.

**Transform tables (structural closed forms).** `laplace::transform` and the Fourier family first try
`FunctionAccess::transform_table`: the integrand is parsed into exponential polynomials
`sum c x^n exp(e1 x + e2 x^2)` (sums, products, integer powers, `exp`/`sin`/`cos`/`sinh`/`cosh` of an
expression at most quadratic in `x`, division by x-free expressions) and the transform is written down:
`n!/(s - e1)^(n+1)` for Laplace, the real and imaginary parts of the same expression at `+-i w` for the
cosine and sine transforms, and `sqrt(pi/a) exp((b -+ i w)^2 / 4a)` differentiated `n` times in `w` for a
Gaussian-weighted whole-line Fourier transform and its inverse. The result is a purely structural Function
(`provenance != Quadrature`), exact to rounding; inputs outside the table (for example `1/(1 + x)`) still
fall back to the lazy integral. The formulas hold inside the region of convergence (Re s above every exponent,
a positive Gaussian coefficient) and are its analytic continuation outside it.

**Malliavin calculus (analytic).** `stochastic::malliavin_derivative(path)` and `stochastic::skorokhod_integral(path, dt)`
on a discretized Wiener path whose levels `W_1..W_N` are the Function's variables. A Cameron-Martin shift of
the increments moves `W_j` by `dt sum_{i<=j} h_i`, so `D_{t_i} F = sum_{j>=i} dF/dW_j` and
`delta(u) = sum u_i dW_i - dt sum_i D_{t_i} u_i`; both are built from `differential`, hence exact for the
discretized functional and analytic. Verified: `W_N^2` (`2 W_N`), `exp(W_3)` (zero after step 3), the path
integral (`T - t`), the product rule, `delta(W_N) = W_N^2 - N dt`, the Ito sum for an adapted integrand, and
`delta(D W_N^2) = 2 W_N^2 - 2 N dt`. N^2 differentials for the Skorokhod integral: for short paths.
Public calls: `operators::stochastic::{malliavin_derivative, skorokhod_integral}`.

**Fused COS kernel.** The vectorized `option_price_cos` now reduces on the host in one pass per option column
for CPU data that records no derivative: closed-form endpoint terms (`cos k pi = (-1)^k`), rotation recurrences
for the trig factors (reseeded every 32 terms), no `n x m` temporaries, a few threads for large batches. Results
unchanged (golden and MATLAB comparisons identical); the Heston strike chain went from 21.4 ms to 2.75 ms
and the generic GBM path now beats the scalar loop at N = 256 (7.5 vs 9.6 ms for 1000 strikes) and N = 1024
(16.1 vs 36.8 ms). Autodiff and non-CPU data keep the Series path.

**Native multigrid smoother.** `multigrid::Options::native_smoother` (default on): the Chebyshev iteration on
CPU levels that record no derivative runs as one fused pass per step (stencil, residual update, new direction,
iterate), with the level stencil cached on the host. It is the same polynomial: the V-cycle and the solution match
the Series path to 1e-10 / 1e-8 in 1-D to 3-D with periodic, Dirichlet and Neumann ends and jumping coefficients;
the solve is 1.6-1.9x faster at 256^2 and 16^3 (transfers and the CG outer loop are still Series expressions,
the next target if more is needed).

### Tranche 14: Vector, Variational And Directional Operators

| Operator | Meaning |
| --- | --- |
| `divergence(variables)`, `curl(variables)` | classical vector calculus on vector-valued Functions; identities `div curl = 0`, `curl grad = 0` are tests |
| `directional(direction)` | `v·∇f`, exact with `gradient` plus `multiply` |
| `material(velocity, time)` | `∂_t + v·∇`, the convective derivative used by NatSci fluids |
| `variational::functional_derivative(variable)` | Euler-Lagrange form `δF/δu = ∂L/∂u - d/dx ∂L/∂u' + d²/dx² ∂L/∂u'' - ...` of a Lagrangian density |
| `variational::euler_lagrange(variable)` | alias returning the stationarity condition `δF/δu = 0` as a Function |
| `wirtinger(z)` | `∂/∂z`, `∂/∂z̄` on complex-valued Functions (only if the primitive registry carries complex dtype; otherwise listed as blocked) |
| `adjoint(op)` | formal adjoint where `adjoint_known`: differential ↔ -differential, multiply ↔ multiply, scale ↔ scale; composition reverses |

Oracles: vector identities to rounding; E-L equations for the harmonic
oscillator, the mechanical Lagrangian `L = ½ u'² - V(u)`, and the minimal
surface equation; the harmonic-oscillator action reproduces Newton's law;
`<Dx f, g> = -<f, Dx g>` on compactly supported fields to quadrature
tolerance validates `adjoint`.

### Tranche 15: Fractional And Transform Operators

| Operator | Notes |
| --- | --- |
| `fractional::differential(x, α, kind)` | `kind ∈ {RiemannLiouville, Caputo, GrunwaldLetnikov}`, `α > 0` |
| `fractional::integral(x, α)` | Riemann-Liouville fractional integral |
| `transform::fourier(x)`, `transform::laplace(x)` | evaluated-only, sampled; Fourier through `Series::fft` |
| `transform::convolution(x, kernel)` | via FFT for uniform grids, direct otherwise |
| `transform::hilbert(x)` | analytic-signal operator via the Fourier route |

Fractional derivatives are non-local; they need a sampled history of the
function on the integration interval, so they are `evaluated_only` and carry a
grid contract like the stochastic operators do. Oracles: RL `D^{1/2} x = 2√(x/π)`;
Caputo of a constant is zero and RL of a constant is `x^{-α}/Γ(1-α)` (the
difference documents why the three kinds exist); semigroup property
`D^α D^β = D^{α+β}` away from the initial-condition terms; `α → 1` limit
recovers `differential`; Fourier/Laplace round trips and the convolution theorem.
All transform operators declare `platform_pure = false` until MPS has complex
dtype.

### Tranche 16: Extension Closure

- Serialization descriptors for every new factory, with schema versions and
  the `(key, version)` loader dispatch for the renamed absolute differential.
- Public-surface audit updated: allow-list of new factory names; forbidden
  public class names list extended with `Measure`, `MeasureSpec`, `Path`,
  `Filtration`, `Process`, `Connection`, `Metric`, `Tensor`, `Brownian`,
  `Manifold` (these must stay option structures, enums or `math::Function`
  values), plus a check that exactly two public classes exist in `math` and
  `operators`.
- Guide: a seventh chapter (or an extension of Operators) covering geometry,
  stochastic, measure and fractional operators with verified programs; the
  derivative chapter's absolute/covariant page rewritten as in Tranche 11.
- Cross-validation matrix: every new operator checked against an independent
  oracle, platform matrix CPU/MPS (path-functional and transform operators
  CPU-only until MPS complex support), thread-safety audit of the new node
  kind, benchmark of the geometric contraction and the Lebesgue level
  assembly.

### Tranches 14-16 — implementation record (done, 2026-10-28)

**Tranche 14 (vector, variational, adjoint).** Top-level `operators::divergence/curl/laplacian/directional/material`
(curl in two or three coordinates), `operators::variational::{functional_derivative, euler_lagrange}` (a Lagrangian
over jet variables `u, u', u'', ...`, the total derivative `D_x` built from `differential`, an error when the listed
jets are too few rather than a silent truncation), and `operators::adjoint(op)` backed by an `adjoint_factory` on the
operator concept: `differential(k) -> (-1)^k differential(k)`, `multiply` and `scale` (real coefficients), `shift(a) ->
shift(-a)`, compositions reverse, linear combinations combine; operators without a known adjoint throw. `wirtinger`
already existed. Verified: `curl grad = 0`, `div curl = 0`, rigid-rotation curl, convective derivative, E-L for the
harmonic oscillator (vanishes on `cos(wx)`), the quartic mechanical Lagrangian, the minimal surface equation, a
fourth-derivative (beam) Lagrangian, an x-dependent Lagrangian, and `<A f, g> = <f, A* g>` for five operators including
a composition and a linear combination, by quadrature over the whole line.

**Tranche 15 (fractional, convolution).** `fractional::integral/differential` (Riemann-Liouville, Caputo,
Grunwald-Letnikov taken equal to Riemann-Liouville), `operators::convolution`. They are analytic Functions, not the
evaluated-only sampled operators first sketched: polynomials with lower terminal 0 have exact power-law closed forms
(`FunctionAccess::fractional_table`), everything else is the lazy integral
`I^a f(x) = (x-a)^a / Gamma(a+1) int_0^1 f(x - (x-a) s^(1/a)) ds`, a substitution that removes the endpoint singularity
of `(x-t)^(a-1)`; the Riemann-Liouville derivative differentiates that integral, the Caputo derivative integrates
`f^(m)`. Verified: `D^(1/2) x = 2 sqrt(x/pi)`, the three kinds on a constant, `I^a x^2`, integer order = ordinary
derivative, the Mittag-Leffler series for `exp` (integral, derivative, Caputo), the semigroup `I^0.3 I^0.4 = I^0.7`,
a shifted terminal, Gaussian * Gaussian. **Hilbert transform:** not an operator (a principal value has a removable
singularity at the end of the integration range that the lazy integral cannot evaluate); it is the eager array function
`pluss::ma::signal::hilbert` / `analytic_signal` (`pluss/subsp/math/signal.h`, built on the Series FFT, checked against
`H cos = sin`, `H sin = -cos`, the envelope of a modulated carrier and `H H = -I`). **Not built:**, and `transform::fourier/laplace` as sampled operators (superseded by the analytic Fourier/Laplace family of
Tranche 12c).

**Tranche 16 (closure).** Serialization descriptors (`vector_*`, `variational_functional_derivative`,
`fractional_*`, `measure_lebesgue/atomic`, `malliavin_derivative`, `skorokhod_integral`, the transforms) with
round-trip tests; operators that hold a Function (`directional`, `material`, `convolution`, `weighted`, `stieltjes`)
are not serializable, by the same rule as `multiply`. The public-surface audit now forbids the extra type names
(`Measure`, `MeasureSpec`, `Path`, `Filtration`, `Process`, `Connection`, `Metric`, `Tensor`, `Brownian`, `Manifold`,
`Transform`, `Lagrangian`, `Fractional`, `Vector`), checks that the headers declare exactly the three classes
`Function`, `Operator` and the `Trace` builder (a planted fourth class fails the audit), and that every new factory is
present. Thread safety: lazy transform, lazy fractional, tabled and convolution Functions evaluated from eight threads
agree bit for bit with the serial value. Platform matrix: CPU only for path-functional and transform operators (MPS has
no complex dtype). Benchmark: the covariant derivative over 200000 points on the sphere took 1.5 s because the batched
`inverse` primitive ran a general factorization per matrix (1.4 s of it); closed forms for 1x1, 2x2 and 3x3 `inverse`
and `det` brought it to 78 ms, with the identity `A A^-1 = I` checked for all three sizes.

**Cross-validation matrix.** Every new operator has an independent oracle in its test: closed forms (transforms, tables,
fractional power laws, Mittag-Leffler series), identities (curl grad, div curl, product rule, layer cake, semigroup),
dual operators (adjoint by quadrature), and a rival implementation (Series path for the COS and multigrid kernels,
`quadra` for the lazy integrals, Simpson against the a.e. spike). References remain unobtained books; none was read, so
no SHA-256 records exist for this programme.

### References For The Extension Programme

Status of every reference: **cited from the author's knowledge; none has yet
been read for this programme.** The standing rule applies: before a tranche
closes, each formula used is checked against a primary or authoritative source
where one can be obtained (open access, arXiv, publisher abstract), the source
and a SHA-256 prefix of the retrieved file are recorded in the tranche's
reuse record, and sources that cannot be obtained are listed as unobtained.
Formulas are in every case *also* verified by independent numerical oracles
(the "Verification oracles" lists), so a mis-remembered convention cannot pass
silently.

| Area | Works (to be consulted) |
| --- | --- |
| Absolute differential, connections | G. Ricci-Curbastro and T. Levi-Civita, "Methodes de calcul differentiel absolu et leurs applications", Math. Ann. 54 (1901) 125-201 (origin of the absolute differential); T. Levi-Civita, "Nozione di parallelismo in una varieta qualunque", Rend. Circ. Mat. Palermo 42 (1917) (parallel displacement); J. M. Lee, *Introduction to Riemannian Manifolds* (2nd ed., Springer); M. do Carmo, *Riemannian Geometry*; Misner-Thorne-Wheeler, *Gravitation* (index conventions, Christoffel symbols, holonomy); J. A. Schouten, *Ricci-Calculus*. |
| Ito and Stratonovich calculus | K. Ito, "On stochastic differential equations", Mem. AMS 4 (1951); R. L. Stratonovich, "A new representation for stochastic integrals and equations", SIAM J. Control 4 (1966) 362-371; I. Karatzas and S. Shreve, *Brownian Motion and Stochastic Calculus*; B. Oksendal, *Stochastic Differential Equations*; P. Protter, *Stochastic Integration and Differential Equations*; P. Kloeden and E. Platen, *Numerical Solution of SDEs* (schemes, strong/weak order, theta family); C. Gardiner, *Handbook of Stochastic Methods* (Ito/Stratonovich/Klimontovich conversion); P. Hanggi and H. Thomas, Phys. Rep. 88 (1982) (convention discussion). |
| Jumps, Marcus, Levy | S. I. Marcus, "Modeling and approximation of stochastic differential equations driven by semimartingales", Stochastics 4 (1981); D. Applebaum, *Levy Processes and Stochastic Calculus*. |
| Generator, Dynkin | E. B. Dynkin, *Markov Processes*; Oksendal ch. 7-8 (generator, Dynkin formula, Kolmogorov equations). |
| Malliavin, Skorokhod | D. Nualart, *The Malliavin Calculus and Related Topics*; A. V. Skorokhod, "On a generalization of a stochastic integral", Theory Probab. Appl. 20 (1975). |
| Lebesgue, measures | H. Lebesgue, "Integrale, longueur, aire", Ann. Mat. Pura Appl. 7 (1902); G. B. Folland, *Real Analysis*; W. Rudin, *Real and Complex Analysis*; V. I. Bogachev, *Measure Theory* (Radon-Nikodym, Stieltjes, product measures); E. H. Lieb and M. Loss, *Analysis* (layer-cake representation); P. Billingsley, *Probability and Measure* (expectation as a Lebesgue integral). |
| Variational operators | I. M. Gelfand and S. V. Fomin, *Calculus of Variations*; R. Courant and D. Hilbert, *Methods of Mathematical Physics* vol. 1. |
| Fractional calculus | I. Podlubny, *Fractional Differential Equations*; A. Kilbas, H. Srivastava, J. Trujillo, *Theory and Applications of Fractional Differential Equations*; S. Samko, A. Kilbas, O. Marichev, *Fractional Integrals and Derivatives*; K. Diethelm, *The Analysis of Fractional Differential Equations* (Caputo versus Riemann-Liouville, numerical schemes). |
| Transforms | R. Bracewell, *The Fourier Transform and Its Applications*; A. Oppenheim and R. Schafer (discrete transforms); Hilbert transform per Bracewell / King, *Hilbert Transforms*. |
| Internal prior art | The existing `sde` header comments (Ito/Stratonovich discretisation identities, pathwise adjoint), `Series::calc.diff.ito/strat`, `calc.malliavin/skorokhod`, and the Math/Calculus guide. |

Books are generally unobtainable in this environment; for those the programme
relies on the independent oracles and states "unobtained" in the closing
record rather than claiming they were read. Open-access items (arXiv surveys
of Malliavin calculus and fractional operators, open lecture notes) will be
fetched and hashed where they exist.

### Clarification Of The Fourier Request (2026-10-28)

"Fourier operators" in the user's request means the **Fourier series
operators** (sine, cosine and full series, Tranche 12b), motivated by the COS
method, not the Fourier *transform*. The transform family (`transform::fourier`,
`transform::laplace`, convolution, Hilbert; Tranche 15) is kept as a later,
separate item and is not scheduled ahead of Taylor and Fourier series. Whether
the Laplace transform is still wanted is open.

### Ordering And Dependencies

```text
11 geometry ──┐
              ├── 13 measure (expectation needs Distribution measures; independent of 11)
12 stochastic ┤        (12 needs no new registry primitive; structural Ito algebra only)
              └── 14 vector/variational  (uses 11's metric/connection machinery for curvilinear forms)
15 fractional/transform (independent; MPS gap noted)
16 closure
```

Tranche 11 first because the rename is a breaking change that the guide
already documents; then 12 (largest conceptual risk, most existing engine to
reuse), 13, 14, 15, 16.

### Open Risks Recorded Up Front

- **Structural matrix primitives.** `levi_civita` needs `Inverse`/`Contract` graph nodes on `Function` (evaluation reuses `Series.linearalg`; the nodes carry shape and derivative rules). If adding them forces changes to the
  primitive registry's output-shape contract, Tranche 11 splits into 11a
  (registry primitives) and 11b (operators).
- **Path axis versus batch axes.** The blueprint's `Function` already notes
  that hand-built vector-valued functions conflate batch axes with the output
  axis. `path_axis` must be proven independent of both; the first deliverable of
  Tranche 12 is a design note settling the axis convention with tests, before any
  operator is written.
- **Lebesgue honesty.** On a floating-point grid no algorithm can integrate a
  nowhere-continuous function it only sees at samples. The operator is a
  genuine range-slicing integral for measures and level sets that are exactly
  computable (roots, atoms, CDF jumps); for arbitrary opaque callables it
  reduces to its grid resolution and says so in its metadata. Claims in the
  guide must match this, as with every other operator in this document.
- **Wirtinger operators** depend on complex dtype in the registry and may
  stay blocked.

## Performance Requirements

### Tranche 10P: Compiled Evaluation Plans

**Status: implemented through the public evaluator; lower-backend fusion remains
an explicit follow-up.**

The closure benchmark shows that graph transformation is not the dominant
cost: transforming the representative function takes roughly `246 us`, while
100 repeated evaluations take roughly `3.35 s` on the current development
machine. The next performance pass therefore targets execution, not additional
rewrite rules.

The following baseline is already complete and must not be rebuilt:

- every sealed `Function` owns an immutable, topologically ordered execution
  plan
- each unique DAG node has one plan entry and one result slot per invocation
- named arguments use a precomputed binding index
- derivative construction memoizes `(node, variable, order)` work

The completed implementation now:

1. Uses fixed stack storage for unary and binary primitive inputs; only truly
   variadic nodes allocate dynamic argument scratch.
2. Keeps all result slots invocation-local, preserving reentrancy and concurrent
   evaluation rather than introducing a mutable cross-call workspace cache.
3. Hoists remaining platform-independent validation and literal metadata out of
   the hot path; retain value-dependent domain checks at execution.
4. Reuses immutable graph constants internally and clones only when a constant
   is itself the escaping result, preserving the public mutation-isolation
   contract.
5. Caches scalar constants by value, dtype and platform for each invocation.
6. Caches integral nodes' transformed integrand plan and fixed abscissa/
   weight scaffolding, while moving numerical Series directly to the caller's
   platform and dtype.
7. Avoids rebuilding an integral's temporary `Function`, bound-argument vector,
   and unsqueezed parameter layout on every invocation.
8. Compiles last-use information into each execution step. CPU evaluation
   releases a slot immediately after its final consumer, so temporary memory
   follows live graph width rather than total graph size.
9. Preserves thread-safe concurrent evaluation by keeping plans immutable and
   invocation slots local to each call.

MPS deliberately retains result slots until the invocation ends. The current
native buffer cache records last writers but not every later reader; releasing
an input at graph-level last use can therefore force a queue drain or make the
buffer eligible for reuse before its submitted readers finish. Early MPS slot
release is blocked until the backend carries read-completion fences. This is a
backend lifetime requirement, not a reason to add host synchronization to the
Function evaluator.

The pass is accepted only when CPU and MPS benchmarks report separately:

- plan compile time
- cold evaluation latency
- warm evaluation latency
- repeated-evaluation throughput
- peak temporary bytes
- quadrature evaluation count

Correctness tests must compare planned and recursive execution for scalar,
vector, matrix, derivative, integral, serialized, and opaque-fallback graphs.
On the current development machine, three native runs over 100 warm evaluations
reported `3.185-3.197 s` for the CPU Float64 fourth derivative and
`0.874-0.875 s` for CPU Complex64 Wirtinger evaluation. MPS reported
`4.893-4.955 s` and `2.562-2.577 s` respectively. Analytic transformation took
`252-497 us`. Checksums agreed across runs and CPU/MPS within their expected
precision.

The measurements establish the remaining performance boundary: the immutable
host plan is no longer repeatedly compiling or retaining dead CPU values, but
each Series primitive still becomes a separate native operation. Small
structural graphs therefore submit many MPS kernels and command buffers. The
next material MPS tier is lower-backend graph fusion or command-buffer batching,
plus read-fence-aware buffer reuse; it is not another public wrapper or a
global mutable result cache. No optimization may introduce host extraction,
implicit CPU fallback, global mutable result caches, or platform-specific
public API.

### Tranche 10C: Complex-Valued Functions

**Status: implemented and verified.** Complex support is split deliberately so
characteristic functions do not share semantics with genuinely complex
variables.

**10C.1: real domain, complex codomain.** A `Function` may accept real Series
arguments and return `Complex64` or `Complex128`. Arithmetic, `exp`, circular
and hyperbolic functions, powers, structural derivatives, persistence, and
ordinary integration delegate to existing complex Series operations. Real-only
domain checks for `log` and `sqrt` are skipped for complex inputs, where the
Series principal branch is authoritative. This tier is sufficient for a
characteristic function `phi(t) = E[exp(i t X)]` and the moment identity
`E[X^n] = phi^(n)(0) / i^n` when the derivative exists.

**10C.2: holomorphic complex variables.** `wirtinger(variable)` constructs
`partial f / partial z`. A graph is holomorphic where
`conjugate_wirtinger(variable)(f)` evaluates to zero. Branch-sensitive
operations use their principal branch and retain their singularities. Ordinary
`differential` remains a real-parameter derivative, including when its codomain
is complex; it never silently changes meaning to a complex derivative.

**10C.3: non-holomorphic functions.** `wirtinger(variable)` and
`conjugate_wirtinger(variable)` provide the two derivatives. Conjugation,
real/imaginary projection, magnitude and phase have explicit paired rules. A
memoized paired graph traversal derives both members without duplicating the
rule engine. Higher orders repeatedly transform the analytic graph; no new
public Function class is introduced.

Acceptance covers complex constants, mixed real/complex promotion, CPU
`Complex128`, native CPU/MPS `Complex64`, characteristic-function moments,
complex integrals, principal branches, persistence, holomorphic identities and
non-holomorphic Wirtinger identities. MPS support includes native complex
arithmetic, powers, transcendental replay, components and `Float32`/`Complex64`
promotion with CPU fallback disabled.

- No loops over Series observations in Function or Operator coordination code.
- Host traversal is proportional to graph or operator-plan size only.
- Numerical work remains vectorized on the input Series platform.
- Constants move to the active platform without host extraction.
- Shared expression nodes evaluate once per invocation.
- Higher derivatives use `(node, variable, order)` caches.
- Direct `differential(variable, order)` is optimized without repeated operator
  composition.
- No implicit CPU fallback is permitted for MPS inputs.

## Test Strategy

Catch2 suites must cover:

- lambda construction and arity validation
- arbitrary variable counts and names
- scalar, vector, and matrix Function outputs
- first and higher derivatives
- mixed derivatives
- gradients, Jacobians, and Hessians
- structural versus autodiff parity
- exact integral roundtrips
- unresolved integral convergence
- Fundamental Theorem and Leibniz rules
- symmetric parity simplification
- domain errors and singularities
- non-smooth policies
- broadcasting and dtype promotion
- float32 and float64
- CPU and MPS
- operator composition and capabilities
- custom operators
- serialization and version mismatch
- graph-growth limits
- CGF-derived cumulants

Tests use analytic identities and independent high-precision references, not
only comparisons between two Pluss implementations.

## Public Surface Enforcement

The build must include an automated public-header audit that fails if any of
the retired specialized classes become public.

Compile-time contract tests must verify:

```cpp
static_assert(std::same_as<
    decltype(pluss::operators::differential("x")),
    pluss::operators::Operator>);

static_assert(std::same_as<
    decltype(pluss::operators::gradient()),
    pluss::operators::Operator>);

static_assert(std::same_as<
    decltype(std::declval<pluss::operators::Operator>()(
        std::declval<const pluss::math::Function&>())),
    pluss::math::Function>);
```

The audit rejects public declarations of:

```text
Variable
Expression
FunctionVector
FunctionMatrix
Differential
Partial
Gradient
Jacobian
Hessian
Integral
Shift
Scale
Multiplication
Builder
```

Private implementation types with these concepts must use internal names under
`detail` and must not appear in installed headers.

## Final Contract

The complete public mental model is:

```text
Function: a reusable mapping of values.
Operator: a reusable transformation of functions.
```

The canonical workflow is:

```cpp
const pluss::math::Function f = pluss::math::function(
    {"x", "y"},
    [](pluss::math::Function x, pluss::math::Function y) {
      return x.pow(2) * (x * y).sin() * x.cos().exp();
    });

const pluss::operators::Operator Dx2 =
    pluss::operators::differential("x", 2);

const pluss::math::Function result = Dx2(f);
```

No additional public class is required to understand that program.

Implementation work is not complete if the numerical results are correct but
the public surface violates this contract. API leanness, structural honesty,
platform neutrality, and mathematical correctness are equal acceptance
requirements.
