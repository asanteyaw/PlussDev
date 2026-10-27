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

`math::Function` carries scalar, vector, or matrix output shape internally.
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
- scalar, vector, or matrix output shape
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
operators. Their inclusion must be driven by real requirements rather than an
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

### Tranche 8: Integral Engine

- exact primitive antiderivatives
- bounded structural patterns
- special-function forms
- anchored and definite integral operators
- unresolved Integral nodes
- Fundamental Theorem and Leibniz rules
- existing quadrature delegation and convergence metadata

### Tranche 9: Existing-System Integration

- explicit `Series::calc` fallback and provenance
- no duplicate tape or quadrature engine
- distribution CGF Functions
- structural cumulant verification
- umbrella integration with PlussMa

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

Every tranche closes only after its Catch2 tests, contract audits, relevant
platform tests, and affected repository builds pass. A tranche is not complete
when only its happy-path demonstration works.

## Performance Requirements

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
