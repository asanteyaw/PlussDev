# `pluss::ai::Artifact`

A named, type-erased value flowing between [`Step`](Step.md)s -- a closed
variant over every type a [`Procedure`](Procedure.md) can carry (`Series`,
`DataFrame`, `Note`, `Slate`, every `lymph`/`ice`/`nlp` payload and report
type, plus a handful of scalar primitives). Built with
[`procedure::input`](../build/input.md) for a run's initial inputs, or
produced by a step's own [`run`](Step/run.md).

## Signature

```cpp
pluss::ai::Artifact::Artifact();
explicit pluss::ai::Artifact::Artifact(std::string name);
pluss::ai::Artifact::Artifact(std::string name, Artifact::Value value);
```

## Construction

A default-constructed `Artifact` is unnamed and empty. The one-argument
constructor gives a name with no value (`empty()` stays `true`) -- what a
[`SinkSpec`](SinkSpec.md)-backed sink step returns when it has no input
to pass through. The two-argument constructor is the common case: a name
plus a value, built by hand or (far more often) via
[`procedure::input`](../build/input.md).

`Artifact::Value` is deliberately closed. Compilation rejects a payload type
that the procedure runtime cannot trace, route, or persist. Use one of the
supported core, tabular, engine, text, or scalar alternatives rather than
placing arbitrary state behind `std::any`. Caller-defined execution logic can
still travel through the graph as a `Step`; only the values crossing step
boundaries must belong to the variant.

## State

`name()`/`named()` -- the identity; `empty()`/`is<T>()`/`value()`/`get<T>()`
-- the typed payload, accessed either through the raw `Value` variant or
through the type-checked `is`/`get` pair.

## Naming And Routing

An artifact's name is its key in the per-run pool. Names are independent of
payload types: two artifacts may both hold `Series` while representing
different ports, and a later step selects them by declared input name. A
callable returning an unnamed `Artifact` is renamed to its step's declared
output; a callable returning an already named artifact preserves that explicit
name.

`named()` and `empty()` answer different questions. An artifact can be named
but empty, as with a sink that receives no input. It can also be unnamed but
hold a value while a callable is constructing a result for the adapter to
rename. Neither condition implies that a contained `Series` or `Note` is itself
defined; audit metadata tracks that inner definedness separately.

## Access And Lifetime

`get<T>()` returns a const reference into the artifact and validates the exact
variant alternative. There is no numeric or related-type conversion: asking
for `double` when the artifact holds `std::int64_t` is an error. Keep the
artifact or owning `ProcedureResult` alive for as long as that reference is
used. `value()` exposes the variant directly for generic visitors; its mutable
overload is primarily useful while implementing custom runners.

## Methods

See the method pages below for `name`/`named`/`empty`/`value`/`is`/`get`.

## Changelog

- **0.1** -- initial documented surface.

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 1
:hidden:

Artifact/Introduction
Artifact/empty
Artifact/get
Artifact/is
Artifact/name
Artifact/named
Artifact/value
```
<!-- apiref:toctree:end -->
