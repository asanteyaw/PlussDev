# Autodiff and Optimization Architecture

## Purpose

This document defines the intended long-term architecture for autodiff and derivative-based optimization across:

- `pluss::codem::Series`
- `PlussAI` machine-learning / deep-learning workloads
- `pluss::subsp::math::optim` numerical optimization

The goal is not to clone PyTorch or JAX mechanically. The goal is to build a production-grade differentiation and optimization stack that uses `Series` as the common substrate while supporting:

- eager, runtime-friendly ML/DL workflows
- JAXopt-style transform-driven optimization
- MATLAB `fmincon`-style solver flexibility
- strong mutation / aliasing correctness
- backward-compatible evolution of the existing `Series` API

This document is the design contract for future implementation work.

## Design Position

Pluss sits at the intersection of two worlds:

- **PyTorch-like execution**:
  - eager values
  - runtime dispatch
  - mutation and aliasing
  - deep-learning ergonomics

- **JAX / JAXopt-like transforms**:
  - `grad`
  - `value_and_grad`
  - `vjp`
  - `jvp`
  - solver interfaces built on derivative transforms rather than direct graph manipulation

The correct design is therefore:

- **Torch-like engine semantics**
- **JAX-like differentiation and optimization surface**
- **Pluss-native naming and integration**

This means the internals may resemble a tape-based eager autodiff engine, but the public differentiation story for optimization should be transformation-first rather than `backward()`-first.

In Pluss language:

- PyTorch `loss.backward()` becomes **`loss.reverse()`**

That operation remains useful for DL workflows, but it is not the foundational abstraction for the entire optimization stack.

## Core Principles

## 1. `Series` Is the Substrate, Not the Policy

`Series` is the common runtime value type. It is responsible for:

- storage
- shape / stride / view behavior
- dtype and backend
- aliasing
- mutation
- dispatch

`Series` must not become:

- a deep-learning-only tensor type
- a JAX-style traced symbolic object
- an optimizer-specific parameter wrapper

Autodiff and optimization policies live on top of `Series`, not inside its public identity.

## 2. One Engine, Multiple Front Doors

There should be one differentiation engine shared across the system.

That engine should support at least two usage styles:

- **DL style** for PlussAI
- **transform style** for `pluss::subsp::math::optim`

This avoids duplicated differentiation logic and keeps correctness centralized.

## 3. Functional Semantics for Derivative Transforms

Derivative transforms must behave as though user code is mathematical and functional, even when the runtime is eager and mutable.

This implies:

- saved values must be version-checked
- in-place mutation of tracked values must be controlled
- transforms should not depend on ambient global hidden state
- differentiation APIs should be deterministic and composable

## 4. Compatibility First

Many libraries already consume `Series`.

Therefore:

- additive changes are preferred over breaking changes
- existing `Series` code should continue working whenever possible
- if a breaking change is required, it must be discussed first
- once approved, the change must be propagated through the codebase in the same patch set

This architecture is intended to evolve the current codebase, not reset it.

## 5. Backend Realism

Autodiff should not assume that long-term competitive performance will come only from in-house CPU and MPS kernels.

The correct separation is:

- autodiff owns derivative semantics
- `Series` runtime owns execution and backend dispatch
- backend libraries own heavy numerical throughput where appropriate

That means Pluss can and should use dedicated compute platforms when they materially improve the system:

- BLAS / LAPACK / Accelerate on CPU
- Metal / MPS on Apple platforms
- future CUDA-family backends such as cuBLAS / cuSOLVER / cuDNN where appropriate
- future compiler / graph backends if Pluss later chooses to stage or fuse workloads

What autodiff must not do is hard-code itself to one backend philosophy such as XLA. JAX uses XLA because that is JAX's execution model. Pluss is `Series`-native and eager first. The differentiation layer should therefore remain backend-independent, while the runtime grows more serious execution backends underneath it.

In short:

- yes, Pluss should use dedicated platforms for heavy computation
- no, autodiff itself should not become a backend compiler project
- the right place for that maturity is Layer 0 runtime/backend dispatch

## Layered Architecture

## Layer 0: `Series` Runtime

Suggested ownership:

- `pluss::codem`

Responsibilities:

- `Storage`
- `SeriesImpl`
- shape / stride / view semantics
- backend dispatch
- type promotion
- alias tracking
- version counters
- direct mutation rules

Key requirement:

- alias-related correctness must be enforced at this layer, because higher layers cannot reason correctly about mutation without trustworthy runtime invariants

### Layer 0 Invariants

1. Views sharing storage must share version semantics.
2. Value-mutating in-place operations must bump version counters.
3. Non-mutating metadata operations must not bump value versions.
4. Backend dispatch behavior must remain independent of autodiff policy.

## Layer 1: Autodiff Engine

Suggested ownership:

- internal engine under `pluss::codem::ad`
- current implementation may continue to live under `pluss::codem::series_tape` while the API stabilizes

Responsibilities:

- reverse-mode graph recording
- node / edge metadata
- saved parent values and versions
- root seeding
- topological traversal
- sensitivity accumulation
- leaf vs non-leaf behavior
- graph lifetime rules
- protection against illegal mutation after capture

This layer is where eager reverse-mode actually happens.

### Current State

The current tape-based work already belongs here:

- tracked-value metadata
- source-link propagation
- version-safety work
- operator pullbacks
- reverse traversal

This work should be preserved and hardened, not discarded.

### Layer 1 Invariants

1. Reverse propagation must be correct for supported ops.
2. Saved intermediates must be invalidated if aliased storage was mutated after capture.
3. The engine must remain backend-agnostic wherever possible.
4. Public user APIs should not depend directly on engine internals.

## Layer 2: Differentiation Transforms

Suggested namespace:

- `pluss::subsp::math::diff`

This is the public mathematical differentiation layer. It should feel closer to JAX than to raw PyTorch autograd internals.

Primary APIs:

- `grad(f)`
- `value_and_grad(f)`
- `vjp(f, x)`
- `jvp(f, x, tangent)`
- later:
  - `jacrev`
  - `jacfwd`
  - `hvp`
  - `linearize`
  - structured Jacobian / Hessian helpers

### Why This Layer Exists

Optimization libraries should not be built around manual graph plumbing.

Instead of teaching solvers to perform:

- tape setup
- reverse seeding
- sensitivity harvesting

we expose derivative transforms that do that internally.

This gives optimization code:

- cleaner interfaces
- composability
- better testing
- less coupling to engine details

### Authoring Rule for Higher-Order AD

If callers want exact higher-order transforms such as:

- `jvp`
- `jacfwd`
- `jacrev`
- `hvp`

they should write objectives generically over the differentiation value type.

Preferred style:

```cpp
auto f = [](const auto& x) {
  return sin(x) + x * x;
};
```

or for structured parameters:

```cpp
auto f = [](const auto& params) {
  return ...;
};
```

This lets the same objective run with:

- `Trace`
- `DualTrace`
- future differentiation wrappers

If an objective is hard-coded to `Trace` only, first-order reverse-mode can still work, but higher-order and forward-mode transforms may be unavailable or may fall back to compatibility paths.

This is a hard design recommendation and should be treated as part of the public autodiff contract.

### MPS / Backend Adaptation Note

Recent autodiff work has primarily changed:

- the public differentiation surface
- tape / version safety
- transform APIs

It has not been aimed at rewriting the MPS backend itself.

Therefore ongoing backend work, including MPS maturity work, should generally adapt by targeting the newer public autodiff surface:

- `Series::reverse()`
- `diff::grad`
- `diff::value_and_grad`
- `diff::vjp`
- `diff::jvp`
- `diff::jacrev`
- `diff::jacfwd`
- `diff::hvp`

rather than by rewriting backend kernels around old internal differentiation entrypoints.

### Example Direction

```cpp
auto vg = pluss::subsp::math::diff::value_and_grad(f);
auto [value, grad] = vg(x);
```

not:

```cpp
auto loss = f(x);
loss.reverse();
```

`reverse()` remains valid, but is not the preferred abstraction for solver code.

## Layer 3A: DL / PlussAI Surface

Suggested ownership:

- `PlussAI`
- potentially `pluss::ai`

Responsibilities:

- parameter containers
- module parameter discovery
- training-loop helpers
- gradient reset utilities
- optimizer wrappers convenient for ML/DL users
- optional convenience methods around `reverse()`

This layer can offer eager ergonomics familiar to Torch users, but it should use Layer 2/1 rather than re-implementing differentiation logic.

### DL-Side Naming

Pluss naming should prefer its own vocabulary:

- `reverse()` instead of `backward()`

Future convenience names may include:

- `retain_sensitivity()`
- `clear_sensitivity()`
- `tracks_slope()`

provided those names stay coherent with the broader Pluss vocabulary.

## Layer 3B: Numerical Optimization Surface

Namespace:

- `pluss::subsp::math::optim`

This layer is solver-facing and should be transform-first.

Responsibilities:

- first-order and second-order derivative-based solvers
- constrained optimization
- trust-region and line-search infrastructure
- solver state, tolerances, stopping rules
- objective and constraint contracts

### Target Feel

This layer should combine:

- **JAXopt-like derivative composition**
- **MATLAB `fmincon`-style flexibility**

It should support:

- unconstrained objectives
- box constraints
- linear equality / inequality constraints
- nonlinear equality / inequality constraints
- explicit or autodiff-derived gradients
- explicit or autodiff-derived Jacobians
- Hessian / Hessian-vector interfaces where appropriate

## API Philosophy

## Primary Differentiation Story

For optimization and advanced math, the primary story is:

- `grad`
- `value_and_grad`
- `vjp`
- `jvp`

For DL ergonomics, `reverse()` is still supported.

This avoids forcing every derivative consumer into a DL-centered programming model.

## Parameter Structures

Long-term, the system must support both:

- raw `Series`
- structured parameter collections

Structured parameters are necessary for:

- ML models
- coupled optimization variables
- constraint packs
- multi-block objectives

This implies a future need for:

- flatten / unflatten utilities
- tree-like parameter traversal
- structure-preserving gradient results

These do not need to mimic JAX pytrees exactly, but the capability is required.

## Mutation and Alias Policy

This is one of the most important design areas.

### Policy

- transforms assume functional mathematical semantics
- runtime remains eager and may expose mutation
- mutation of tracked values is guarded by engine/runtime rules

### Consequences

1. In-place writes on tracked values may be forbidden outright.
2. Saved values used for reverse propagation must be version-checked.
3. Views / aliases must share version semantics.
4. Optimizer updates should prefer detached or functional parameter updates.

This is the correct compromise between:

- Torch-style eager practicality
- JAX-style transform purity

## Reverse-Mode and Forward-Mode

Reverse-mode is only half the story.

### Reverse-Mode Near Term

Use reverse-mode as the first production-grade derivative engine because it directly supports:

- scalar-loss training
- first-order optimization
- common ML workloads

### Forward-Mode Requirement

Forward-mode / JVP support is also required for the long-term vision because:

- Jacobian-vector products are natural in optimization
- Hessian-vector products often compose efficiently from JVP/VJP
- implicit differentiation and advanced solvers depend on it

Therefore:

- reverse-mode first
- forward-mode next
- both exposed through the transform layer

## Production-Grade Expectations

This project should not settle for toy autodiff.

### What "Production-Grade" Means Here

1. **Correctness before convenience**
   - saved-tensor safety
   - alias/version correctness
   - explicit edge-case handling

2. **Layer separation**
   - no solver logic hidden inside the tape layer
   - no backend policy baked into optimizer APIs

3. **Composable public transforms**
   - gradients are obtained through stable math-facing APIs

4. **Backward-compatible evolution**
   - existing `Series` consumers should not break casually

5. **Extensible operator coverage**
   - derivative rules should move toward a registry / generated system rather than permanent ad hoc placement

6. **Serious mutation semantics**
   - explicit, enforceable, testable

## Compatibility Policy

Compatibility with existing `Series` consumers is a design requirement.

### Rules

1. Prefer additive APIs.
2. Keep current `Series` arithmetic and view APIs working.
3. Avoid renaming or removing public methods unless necessary.
4. If a public break is necessary:
   - discuss first
   - propagate the change through the repository in the same effort
   - document migration clearly

### Current Autodiff Work and Compatibility

The current autograd work should be viewed as:

- the beginning of Layer 1
- not a final public API contract

That means:

- internals may continue to evolve
- public `Series` semantics should remain stable unless intentionally changed

## Recommended Namespace Plan

Recommended long-term split:

- `pluss::codem`
  - `Series`
  - runtime, storage, backend, views

- `pluss::codem::ad`
  - low-level autodiff engine

- `pluss::subsp::math::diff`
  - derivative transforms

- `pluss::subsp::math::optim`
  - optimization algorithms, constraints, solver interfaces

- `pluss::ai`
  - ML / DL parameter and training conveniences

## Naming Direction

The platform should keep Pluss-native naming where it improves identity and clarity.

Examples:

- `reverse()` for reverse propagation
- `sensitivity()` rather than `grad()` on stored value state
- `tracks_slope()` rather than `requires_grad()`

However, naming must not become so exotic that mathematically literate users cannot map concepts.

Rule:

- be Pluss-native at the surface
- stay mathematically conventional in transform APIs

So this is good:

- `diff::grad`
- `diff::value_and_grad`
- `loss.reverse()`

## Implementation Roadmap

## Phase 1: Harden the Existing Reverse Engine

- expand backward coverage for core ops
- strengthen mutation/version checks
- define `reverse()` semantics cleanly
- improve tests

## Phase 2: Introduce Transform Layer

- create `pluss::subsp::math::diff`
- implement:
  - `grad`
  - `value_and_grad`
  - `vjp`
- keep current low-level APIs functioning

## Phase 3: Optimization Adapters

- move gradient-based optimizer APIs to consume transform outputs
- make solver contracts explicit
- keep ML-friendly convenience wrappers additive

## Phase 4: Forward-Mode Support

- implement JVP / dual representation
- add higher-order derivative composition

## Phase 5: Structured Parameters and Solver Expansion

- parameter flattening / unflattening
- constraint Jacobians
- Hessian-vector products
- trust-region / quasi-Newton / constrained solvers

## Non-Goals

At this stage, the project should not attempt to fully replicate every PyTorch autograd feature immediately.

Not immediate goals:

- total Torch parity in hooks and engine internals
- full graph staging / tracing in the JAX sense
- replacing `Series` with a pure symbolic transform system

Those may become partial goals later, but they are not the right first constraints.

## Summary

The intended destination is:

- **one serious autodiff engine**
- **two public usage modes**
- **Torch-grade runtime correctness**
- **JAX-grade transform surface**
- **Pluss-native optimization and naming**

The current tape and autograd work is not a false start. It is the base engine layer.

Future implementation should therefore:

- preserve and harden the engine
- build a clean transform layer above it
- keep `Series` stable for existing consumers
- make `pluss::subsp::math::optim` derivative-first and solver-grade

That is the path most likely to produce a real industry-contender API rather than a toy autodiff subsystem.
