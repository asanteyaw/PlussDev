# `procedure::callable` Introduction

## What This Category Owns

`callable` owns the simplest, engine-agnostic way to put real logic into
a step: a plain function or lambda over `Series` or `DataFrame`, wrapped
by [`step`](step.md) into a `Map`-capability [`Step`](../core/Step.md)
with no persistence by default, plus [`bind`](bind.md), its own sugar
for making such a step reloadable by a registry key.

## Why No Engine Knowledge Is Needed Here

Every other adapter family ([`lymph`](../lymph.md)/[`ice`](../ice.md)/
[`nlp`](../nlp.md)/[`module`](../module.md)) wraps a specific kind of
component with its own learn/infer/assess story. `callable` wraps
nothing but the function itself -- it works for arithmetic on a
`Series`, a column projection on a `DataFrame`, or any other
single-input, single-output transformation that does not need to be a
full model. A callable step's `RunOptions::mode` is simply ignored, since
a plain function has only one behavior regardless of
[`ProcedureMode`](../core/Introduction.md).

## Two Return Shapes

A callable's return value is wrapped automatically: returning any
`Artifact::Value`-compatible type (a `Series`, a `bool`, and so on)
publishes it under the step's own declared output name; returning an
`Artifact` directly is used as-is (or renamed to the declared output if
left unnamed) -- useful for a callable that wants to name its own result
dynamically rather than always using the step's fixed output name.

## Reading Order

1. [`step`](step.md) -- the two overloads: without and with a registry
   key.
2. [`bind`](bind.md) -- the registry sugar the keyed overload depends
   on.

## Related Pages

- [`build`](../build.md) -- the generic `step` overloads this family
  layers on top of.
- [`registry`](../registry.md) -- the lower-level mechanism
  [`bind`](bind.md) is sugar over.
