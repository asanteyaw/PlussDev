# PlussAI `procedure`

`procedure` is PlussAI's cross-engine execution, persistence, and routing
layer. `lymph`, `ice`, and `nlp` each answer "how does one ML path turn raw
input into a prediction?"; `procedure` answers "how does a deployable system
admit data, run the right components in the right order, record what
happened, enforce safety rules, and emit results?" It is the production
graph that coordinates those engines, not a replacement for any of them.

## Public Positioning

Teach `procedure` as the family for:

- wiring `lymph`/`ice`/`nlp`/raw-module components into one named, validated,
  ordered DAG
- running that DAG once per call in `Infer`/`Learn`/`Assess` mode and getting
  back every produced artifact plus a structured audit trail
- persisting and restoring the whole wired system -- weights and graph shape
  together -- as one directory bundle
- production safety defaults: schema checks, time/leakage checks, and
  deterministic step ordering, enforced centrally rather than per component

It is not a modeling family by itself. It contains no training algorithms of
its own; every learning/inference/assessment call a `Step` makes delegates to
the engine it adapts.

## Include Surface

```cpp
#include <pluss/ai/procedure.h>
```

## Public Namespace

```cpp
namespace proc = pluss::ai::procedure;
```

## Relationship To Other Families

- wires and persists `lymph::workflow::Policy`/`lymph::models::Model`/
  `lymph::workflow::Workflow`, `ice::Model` subclasses, and `nlp::Workflow`
  artifacts without changing any of their own APIs
- reaches raw `nn` modules only through an explicitly-named advanced escape
  hatch, keeping `Procedure -> Ice -> NodeK` the stable, guided public path
- accepts plain functions/lambdas over `Series`/`DataFrame` for steps that
  need no engine at all

## The Execution Contract

A `Procedure` is a reusable graph definition, not a container for one run's
data. Its `Step` objects retain component state and callable behavior; its
`Link` objects describe dependency edges. Every call to `run()` creates a new
artifact pool, seeds that pool with the named inputs supplied by the caller,
executes each reachable step once, and returns the resulting pool and audit
records in a new `ProcedureResult`.

That separation matters in practice:

- constructing a graph validates names, links, capabilities, and acyclicity;
- invoking the graph validates the artifacts supplied for that particular
  call, including required columns, time roles, and adapter-specific types;
- inspecting a `ProcedureResult` never mutates the graph that produced it;
- saving a graph persists its topology and every persistable component, not
  the transient artifacts from a prior run.

Links order steps and map ports; artifact names carry values. A link from
`clean` to `score` says that `score` depends on `clean`. Supplying
`from_output` and `to_input` additionally says which named output satisfies
which named input when a step has more than one port. Independent steps in the
same topological wave may run concurrently when `ProcedureOptions::workers`
is greater than one, but a dependent step cannot begin until every linked
predecessor has completed.

## Choosing A Step Adapter

| Wrapped behavior | Adapter | Ownership and persistence |
| --- | --- | --- |
| One ordinary `Series` or `DataFrame` transformation | [`callable`](procedure/callable.md) | Callable is captured by the step; add a registry key when the graph must reload it |
| Tabular admission, statistical model, or complete tabular workflow | [`lymph`](procedure/lymph.md) | Component state is owned by the step and is bundle-persistable |
| Trainable `ice::Model` | [`ice`](procedure/ice.md) | Reference form is non-owning; value form owns and can restore through a registered factory |
| Text-analysis workflow | [`nlp`](procedure/nlp.md) | Workflow is owned and bundle-persistable |
| Object exposing only `forward(const Series&)` | [`module`](procedure/module.md) | Captured as an inference transform; deliberately not persistable |
| Caller-defined component with its own save/load contract | [`registry`](procedure/registry.md) | Contract key, schemas, compatibility gate, loader, and saver are explicit |

Prefer the narrowest adapter that preserves the component's real semantics.
A raw module can produce a prediction, but it cannot express learning,
assessment, or restoration. A plain callable is excellent for stateless data
motion, but a model adapter is the correct choice when the graph must retain
trained state.

## Modes And Safety Boundaries

`RunOptions::mode` is interpreted by model-aware adapters. `Infer` produces
predictions, `Learn` updates the wrapped component and publishes the trained
component, and `Assess` publishes the engine's assessment report. Stateless
callable, policy, and module steps have one behavior and therefore ignore the
mode.

Safety declarations live on the step instead of being scattered through
runner code. A step can require named columns, require a time feature, declare
that it may change row count, or declare intentional access to future data.
`run()` checks those declarations before or immediately after the runner at
the relevant boundary. With `fail_fast=true`, a violation throws. With
`fail_fast=false`, execution stops cleanly and the returned result records the
failed step and warning while retaining artifacts produced earlier in the
run.

## Worked Example: Branch, Join, And Audit

This graph computes two independent transformations of the same input and
joins their results. The first two steps may execute concurrently; the join
waits for both named artifacts.

```cpp
#include <pluss/ai/procedure.h>
#include <pluss/pluss.h>

#include <iostream>
#include <vector>

int main() {
  namespace ai = pluss::ai;

  const auto cfg = pluss::Config{}
      .with_dtype(pluss::ScalarType::Float64)
      .with_platform(pluss::pCPU);
  const auto x = pluss::make_series(
      std::vector<double>{1.0, 2.0, 4.0, 8.0}, {4}, cfg);

  ai::StepOptions join{};
  join.capabilities = {ai::StepCapability::Join};
  join.inputs = {"centered", "energy"};
  join.outputs = {"score"};

  ai::ProcedureOptions options{};
  options.workers = 2;

  const auto proc = ai::procedure::make({
      ai::procedure::step("center", "x", "centered",
          [](const pluss::Series& value) {
            return value.sub(value.mean());
          }),
      ai::procedure::step("energy", "x", "energy",
          [](const pluss::Series& value) {
            return value.mul(value);
          }),
      ai::procedure::step("score", join,
          [](const std::vector<ai::Artifact>& inputs,
             const ai::RunOptions&) {
            return ai::Artifact(
                "score",
                ai::Artifact::Value(
                    inputs[0].get<pluss::Series>().add(
                        inputs[1].get<pluss::Series>())));
          }),
  }, {
      ai::procedure::link("center", "score", "centered", "centered"),
      ai::procedure::link("energy", "score", "energy", "energy"),
  }, options);

  const auto result = proc.run({ai::procedure::input("x", x)});
  std::cout << "score_elements="
            << result.get<pluss::Series>("score").numel() << '\n';
  std::cout << "steps_ran=" << result.trace().size() << '\n';
  std::cout << "audit_records=" << result.audit().size() << '\n';
}
```

Output:

```text
score_elements=4
steps_ran=3
audit_records=3
```

The output order of independent steps is not a synchronization contract.
Depend on artifact names and links, not on which parallel branch happens to
finish first.

## Worked Example: Inspect A Soft Failure

Use non-fail-fast execution when a service boundary needs a structured result
instead of exception control flow. The missing `signal` artifact prevents the
runner from being called.

```cpp
#include <pluss/ai/procedure.h>
#include <pluss/pluss.h>

#include <iostream>

int main() {
  namespace ai = pluss::ai;

  ai::ProcedureOptions options{};
  options.fail_fast = false;

  const auto proc = ai::procedure::make({
      ai::procedure::step("needs_signal", "signal", "out",
          [](const pluss::Series& signal) { return signal; }),
  }, {}, options);

  const auto result = proc.run();
  std::cout << "ok=" << result.ok() << '\n';
  std::cout << "failed_step=" << result.failed_step() << '\n';
  std::cout << "warnings=" << result.warnings().size() << '\n';
}
```

Output:

```text
ok=0
failed_step=needs_signal
warnings=1
```

Artifacts emitted before a soft failure remain available. Steps after the
failure do not run.

## Categories

This reference follows the framework -> category -> type narrowing order,
matching [`ice`](plussai_ice.md)'s own convention. Each category page states
what the concern owns, what it does not, and the first type or function to
reach for.

```{toctree}
:maxdepth: 3

procedure/core
procedure/build
procedure/callable
procedure/lymph
procedure/ice
procedure/nlp
procedure/module
procedure/registry
procedure/persist
```

## Reading Strategy

1. [core](procedure/core.md) -- `Procedure`/`Step`/`Link`/`Artifact`/
   `ProcedureResult` and their supporting option structs, referenced by
   every later category.
2. [build](procedure/build.md) -- the generic `step`/`sink`/`link`/`input`/
   `make` free functions every adapter overload specializes.
3. [callable](procedure/callable.md) -- the simplest adapter: a plain
   function or lambda over `Series`/`DataFrame`, with its own registry
   shorthand.
4. [lymph](procedure/lymph.md) -- wiring a `lymph::workflow::Policy`,
   `lymph::models::Model`, or `lymph::workflow::Workflow` in as a step.
5. [ice](procedure/ice.md) -- wiring an `ice::Model` subclass in as a step,
   by reference or owned-with-persistence.
6. [nlp](procedure/nlp.md) -- wiring an `nlp::Workflow` in as a step.
7. [module](procedure/module.md) -- the raw-module escape hatch for any
   `nn`-style `Series -> Series` object, the least-guided of the four
   adapter families.
8. [registry](procedure/registry.md) -- the custom-contract mechanism a
   `CustomPersist`-backed step needs to reload after `save`/`load`.
9. [persist](procedure/persist.md) -- the directory-bundle format
   `Procedure::save` writes and `procedure::load` reads, with its checksum
   and schema-compatibility guarantees.

## Include And Lifetime Notes

Including `<pluss/ai/procedure.h>` exposes the complete family. A `Procedure`
may safely outlive local values passed to `run()` because those values are
copied or moved into the per-run artifact pool. Adapter lifetime differs by
overload: most adapters own shared component state, while the `ice::Model&`
overload intentionally retains a non-owning reference and therefore requires
the model to outlive every use of the procedure. The individual adapter pages
state their ownership contract explicitly.
