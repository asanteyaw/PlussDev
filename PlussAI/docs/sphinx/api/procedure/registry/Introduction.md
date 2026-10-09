# `procedure::registry` Introduction

## What This Category Owns

`registry` owns the mechanism behind
[`StepPersistSpec::custom`](../core/StepPersistSpec.md): a module-level
map from string key to a reload recipe, populated by
[`bind`](bind.md)/[`bind_ice`](bind_ice.md), and consulted by
[`step`](step.md)'s `CustomPersist` overload and by
[`procedure::load`](../persist/load.md). It is the lower-level mechanism
[`callable`](../callable.md)'s own `bind` is sugar over.

## Three Pieces That Fit Together

A [`CustomContract`](CustomContract.md) is the full registry record: a
key, schema versions, a `StepLoader` that reconstructs a `Step` from a
saved bundle, and an optional `CompatibilityGate`. A
[`CustomPersist`](CustomPersist.md) is the lighter-weight request a
caller passes to [`step`](step.md) directly -- it only works once a
matching `CustomContract` is already bound, since `step`'s own
`CustomPersist` overload looks the key up and fails fast if nothing is
bound. [`bind`](bind.md) registers a `CustomContract` (either built by
hand, or from a bare `StepLoader`); [`bind_ice`](bind_ice.md) registers a
narrower kind of factory specifically for reconstructing `ice::Model`
subclasses by `restore_key`, used by [`ice`](../ice.md)'s owning `step`
overload rather than by `CustomPersist` steps.

## Why A Registry At All

[`procedure::load`](../persist/load.md) needs to turn a saved
component's component-tag string back into a real, running `Step` --
for a built-in adapter, that mapping is fixed at compile time, but for a
[`callable`](../callable.md) step or a caller-defined component there is
no way to know the right reconstruction logic without the caller
supplying it. `registry` is that supplied-logic slot, keyed by a plain
string so it survives a save/load round-trip through a text manifest.

## Related Pages

- [`StepPersistSpec`](../core/StepPersistSpec.md) -- the `custom` flag
  this mechanism sets.
- [`callable`](../callable.md) -- `bind`'s own sugar built on top of
  this.
- [`persist`](../persist.md) -- where a bound contract is actually
  consulted, during `load`.
