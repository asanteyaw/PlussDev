# Optimization and Differentiation Contract

## Audience

This document is for implementers working on:

- gradient-based unconstrained optimization
- constrained optimization in the style of MATLAB `fmincon`
- transform-driven solver stacks in the spirit of JAXopt

It explains how such solvers are expected to interact with the Pluss autodiff system.

This page is intentionally more operational than [autodiff_architecture](autodiff_architecture.md). The architecture page defines the subsystem boundaries; this page defines the contract solver authors should rely on.

## Design Goal

The optimization layer must treat autodiff as a **derivative engine**, not as a public graph-manipulation API.

That means solver implementations should consume differentiation through stable transforms such as:

- `grad`
- `value_and_grad`
- `vjp`
- `jvp`

and should **not** depend directly on:

- tape node types
- reverse traversal internals
- saved intermediate bookkeeping
- alias/version implementation details

Those details belong to the engine layer and may evolve.

## What Kind of Problems Must Be Supported

The optimization stack should eventually cover at least the following problem families.

## 1. Unconstrained Minimization

Canonical form:

```text
minimize f(x)
```

where:

- `x` may be a single `Series`
- or a structured collection of `Series`
- `f(x)` is scalar-valued

Typical solvers:

- gradient descent
- nonlinear conjugate gradient
- L-BFGS
- Newton / quasi-Newton
- trust-region methods

## 2. Bound-Constrained Minimization

Canonical form:

```text
minimize f(x)
subject to l <= x <= u
```

This is a minimum bar for serious numerical optimization and should be first-class.

## 3. Linearly Constrained Problems

Canonical form:

```text
minimize f(x)
subject to Aeq x = beq
           Aineq x <= bineq
```

## 4. Nonlinearly Constrained Problems

Canonical form:

```text
minimize f(x)
subject to ceq(x) = 0
           cineq(x) <= 0
```

This is the `fmincon`-style target.

The derivative layer must therefore support not only objective gradients, but also:

- Jacobians of constraint maps
- vector-Jacobian and Jacobian-vector products
- Hessian or Hessian-vector interfaces later

## Public Differentiation Contract for Solvers

Solver authors should program against the following conceptual interface.

## Scalar Objective

Recommended target shape:

```cpp
Series f(const Series& x);
```

or for multiple parameter blocks:

```cpp
Series f(const std::vector<Series>& xs);
```

The output must be scalar-valued for reverse-mode gradient extraction unless an explicit seed / transform contract says otherwise.

## Objective Gradient

Preferred usage:

```cpp
auto vg = diff::value_and_grad(f);
auto [value, grad] = vg(x);
```

For multiple blocks:

```cpp
auto vg = diff::value_and_grad(f);
auto [value, grads] = vg(xs);
```

This is the core contract a first-order solver should depend on.

## Constraint Jacobians

Constraint handling should be designed around one of these conceptual forms:

```cpp
Series cineq(const Series& x);   // shape [m]
Series ceq(const Series& x);     // shape [k]
```

with future support from transforms such as:

- `jacrev`
- `jacfwd`
- `vjp`
- `jvp`

The solver layer should be prepared to consume either:

- full Jacobians
- Jacobian-vector products
- vector-Jacobian products

depending on the algorithm.

## Hessian Information

The contract should leave room for:

- exact Hessian
- Hessian-vector product
- quasi-Newton approximation
- user-supplied Hessian callback

The autodiff stack therefore must not be boxed into reverse-only first-order assumptions.

## Derivative Source Policy

Optimization code must support multiple derivative sources.

## 1. Autodiff-Derived

Default for differentiable `Series` programs.

## 2. User-Supplied Analytic Derivatives

Important for:

- performance-critical cases
- sparse Jacobians
- domain-specific constrained solvers
- cases where a user already knows the exact derivatives

## 3. Mixed Mode

Also important:

- autodiff objective gradient
- user-supplied constraint Jacobian
- user-supplied Hessian-vector product

The optimization layer should never force autodiff-only use.

## Parameter Representation

The solver layer must support both:

- a single `Series`
- multiple `Series` blocks

Later, it should support structured parameter containers.

### Immediate Practical Rule

In the first implementation wave, solver authors should assume one of these two forms:

```cpp
Series x;
```

or

```cpp
std::vector<Series> xs;
```

That is sufficient to begin implementation without prematurely committing to a complicated tree abstraction.

### Future Direction

Longer term, the system should support flatten/unflatten utilities so a solver can:

1. flatten structured parameters into an optimization state
2. compute derivatives
3. rebuild the original structure

This is a major requirement for both PlussAI and advanced optimization.

## Mutation Contract

Solver authors should assume transforms operate under **functional semantics**.

This means:

- objective evaluation should be treated as side-effect-free from the solver's point of view
- parameter updates should preferably be expressed functionally
- in-place mutation of tracked values is not a solver-facing primitive

### Practical Consequence

Do this:

```cpp
Series x_next = x.sub(step.mul(g));
```

Prefer this over mutating tracked values in place.

If in-place updates are used internally for performance, they must happen:

- on detached values
- or under explicit mutation rules outside active derivative recording

Solver implementations should not rely on mutating values that are still part of the active derivative graph.

## Separation of Responsibilities

## What the Differentiation Layer Owns

- graph recording
- version checks
- reverse / forward propagation
- derivative extraction
- transform composition

## What the Optimization Layer Owns

- solver logic
- trust region / line search policies
- penalty / barrier / augmented Lagrangian logic
- stopping criteria
- feasibility checks
- projection or filtering strategies
- convergence diagnostics

This separation must remain strict.

Optimization code should not inspect engine internals to decide solver behavior.

## Problem Interfaces for Future Solver Work

The optimization layer should eventually expose explicit problem descriptors rather than forcing every solver to accept raw lambdas with ad hoc semantics.

Example direction:

```cpp
struct ScalarObjective {
  std::function<Series(const Series&)> value;
};

struct NonlinearConstraints {
  std::function<Series(const Series&)> equality;
  std::function<Series(const Series&)> inequality;
};

struct Bounds {
  std::optional<Series> lower;
  std::optional<Series> upper;
};
```

This is not a final API prescription, but it is the right conceptual direction.

The key idea is:

- solver configuration should be explicit
- derivative source should be injectable
- constraints should not be hidden in overloaded function signatures

## Recommended Solver Consumption Pattern

For first-order unconstrained solvers, the intended consumption pattern is:

```cpp
auto vg = diff::value_and_grad(objective);
auto [value, grad] = vg(x);
```

For constrained solvers:

```cpp
auto objective_vg = diff::value_and_grad(objective);
auto constraint_jvp = diff::jvp(constraints, x, direction);
auto constraint_vjp = diff::vjp(constraints, x);
```

Practical rule for reusable objectives:

- if the same objective must run under both reverse-mode and forward-mode transforms,
  write it as a generic callable or templated functor rather than hard-coding
  `Trace` in the signature

Example:

```cpp
auto objective = [&](const auto& x) {
  return diff::sin(x) + diff::square(x);
};
```

The exact surface may expand, but the principle is fixed:

- **solver code consumes transforms**
- **transform code consumes the engine**

## Naming Expectations

Public naming should reflect Pluss language while remaining mathematically intelligible.

Examples:

- reverse propagation:
  - `loss.reverse()`

- differentiation transforms:
  - `diff::grad`
  - `diff::value_and_grad`
  - `diff::vjp`
  - `diff::jvp`

Solver authors should assume transform APIs use standard mathematical vocabulary even if some low-level storage names are Pluss-native.

## Compatibility Rules for Solver Work

When implementing new optimization algorithms:

1. do not break existing `Series` user code casually
2. do not bind solver APIs directly to tape internals
3. do not require PlussAI-only parameter abstractions
4. prefer additive interfaces
5. if a public break is necessary, discuss it first and propagate it consistently

## What Exists Today vs What to Build Next

### Already Present

- eager reverse-mode engine
- tracked-value recording
- stored sensitivities
- version-safety machinery
- a first optimizer helper surface

### Next Layer to Build

- `pluss::subsp::math::diff`
- `grad`
- `value_and_grad`
- `vjp`
- later `jvp`

The optimization implementation should start from that transform layer rather than extending the low-level tape API outward.

## Summary for Implementers

If you are building JAXopt/`fmincon`-style solvers in Pluss:

1. Treat autodiff as a derivative service.
2. Depend on transforms, not tape nodes.
3. Assume functional semantics at the solver boundary.
4. Support autodiff, analytic, and mixed derivative sources.
5. Keep constraints explicit and solver contracts clean.
6. Preserve `Series` compatibility unless a break is deliberate and coordinated.

That is the intended contract between the autodiff system and the future gradient-based optimization stack.
