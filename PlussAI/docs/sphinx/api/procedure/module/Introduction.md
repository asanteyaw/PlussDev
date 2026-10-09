# `procedure::module` Introduction

## What This Category Owns

`module` owns the raw-module escape hatch: wrapping any object with a
`.forward(const Series&) -> Series` method -- an `nn` layer, or any
plain struct of the caller's own -- as a `Map`+`Infer`-capability
[`Step`](../core/Step.md) with no persistence of its own. This is the
least-guided of the four adapter families (alongside
[`lymph`](../lymph.md)/[`ice`](../ice.md)/[`nlp`](../nlp.md)): the
design's own stable public path runs from `Procedure` through `ice` to
`nn`, so reach for [`ice`](../ice.md) when the thing being wrapped is
already (or could be) an `ice::Model`, and keep `module` for a
genuinely raw transform that is not, and does not need to be, a full
model.

## Why No Persistence

A `module` step never attaches a [`StepPersistSpec`](../core/StepPersistSpec.md)
-- there is no generic way to serialize an arbitrary caller-defined
object's state, so the step is always unpersistable. A caller who needs
a persistable raw module should wrap it as an `ice::Model` subclass (see
[`ice`](../ice.md)) or register it through the
[`registry`](../registry.md)'s `CustomPersist` mechanism instead.

## What Satisfies The Concept

Anything with a method matching `Series forward(const Series&)` (or
convertible to `Series`) -- an actual `nn` layer object (`nn::ReLU{}`,
`nn::Linear{...}`, a hand-composed `nn::Sequential`), or a plain struct
with nothing but that one method, as this family's own example shows.

## Related Pages

- [`ice`](../ice.md) -- the guided path for anything that is, or could
  be, a full `ice::Model`.
- [`registry`](../registry.md) -- adding persistence to a step this
  family cannot persist on its own.
