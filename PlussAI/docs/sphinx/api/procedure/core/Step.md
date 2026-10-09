# `pluss::ai::Step`

One named node in a [`Procedure`](Procedure.md): a capability mask,
declared input/output artifact names, an optional persistence spec, and
the runner closure that executes it. Built almost always through
[`procedure::step`](../build/step.md)'s many overloads rather than this
constructor directly.

## Signature

```cpp
pluss::ai::Step::Step();
pluss::ai::Step::Step(std::string name, StepOptions options);
pluss::ai::Step::Step(std::string name, StepOptions options, Step::Runner runner);
pluss::ai::Step::Step(std::string name, StepOptions options, StepPersistSpec persist, Step::Runner runner);
```

## Construction

A default-constructed `Step` has an empty name and default `StepOptions`
(capability `Map`, no ports) -- not useful on its own, and never produced
by [`procedure::step`](../build/step.md). Every real `Step` comes from one
of `procedure::step`'s overloads: see [`build`](../build.md) for the three
generic forms and [`callable`](../callable.md)/[`lymph`](../lymph.md)/
[`ice`](../ice.md)/[`nlp`](../nlp.md)/[`module`](../module.md)/
[`registry`](../registry.md) for the engine-specific adapters, each of
which fills in `StepOptions`/`StepPersistSpec`/the runner closure
appropriately for what it wraps.

The direct constructors are the low-level escape hatch. They do not infer
ports, capabilities, persistence metadata, or component identity. A built-in
`procedure::step` overload does infer those properties and should be preferred
whenever it matches the wrapped behavior. Hand-built steps are appropriate for
joins, gates, watchers, or other graph nodes whose runner consumes the complete
`std::vector<Artifact>` rather than one strongly typed value.

## State

`name()` plus everything `StepOptions`/`StepPersistSpec` declare, each
exposed through its own narrow accessor rather than requiring callers to
reach into the options struct directly: capability/port queries
(`inputs`/`outputs`/`capability_mask`/`has`), persistence queries
(`persistable`/`reloadable`/`frozen`/`custom`/`persist_key`/
`persist_schema_version`/`component_schema_version`/`persist_fields`/
`can_save_component`), and runtime queries (`trained`/
`can_change_row_count`/`component`/`runnable`).

## Runner Contract

`Step::Runner` receives the resolved input artifacts in the order declared by
`StepOptions::inputs`, followed by the current `RunOptions`. It returns exactly
one `Artifact`. A runner may return an artifact whose name differs from the
declared output, but doing so can make downstream name resolution impossible;
built-in adapters consistently publish the declared output name.

A step without a runner is not automatically invalid. Sink-capability steps
pass through their first input, and a non-sink placeholder passes through one
input or produces an empty artifact when it has none. Use `runnable()` to
distinguish a closure-backed step from those fallback semantics.

## Ownership

The runner closure and `StepPersistSpec` are stored by value. Captured handles,
models, or shared state therefore follow the ownership decisions made by the
builder overload. Callable, module, Lymph, and NLP adapters own their captured
state. The `ice::Model&` adapter is intentionally non-owning and requires the
referenced model to outlive every copy and execution of the step.

## Safety Declarations

`StepOptions` is an executable contract, not descriptive decoration.
`Procedure::run()` validates required columns and time features before calling
the runner, blocks undeclared future access in learning and assessment modes,
and compares input/output row counts afterward unless
`can_change_row_count=true`. These central checks let custom runners remain
focused on their transformation while retaining the same safety envelope as a
built-in adapter.

## Methods

See the method pages below for every accessor plus `run`/`save_component`.

## Changelog

- **0.1** -- initial documented surface.

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 1
:hidden:

Step/Introduction
Step/can_change_row_count
Step/can_save_component
Step/capability_mask
Step/component
Step/component_schema_version
Step/custom
Step/frozen
Step/has
Step/inputs
Step/name
Step/options
Step/outputs
Step/persist
Step/persist_fields
Step/persist_key
Step/persist_schema_version
Step/persistable
Step/reloadable
Step/run
Step/runnable
Step/save_component
Step/trained
```
<!-- apiref:toctree:end -->
