# `procedure::ice` Introduction

## What This Category Owns

`ice` owns the two ways to wrap an [`ice::Model`](../../ice/model/Model.md)
subclass as a [`Step`](../core/Step.md): a non-owning reference overload
for a model whose lifetime the caller keeps, and an owning template
overload that takes a `restore_key`, making the step persistable.

## Reference Vs. Owning

The reference overload (`step(name, input, output, ice::Model&, ...)`)
is the simpler path: the caller keeps the model alive (typically a local
variable spanning several `Procedure::run` calls across `Learn` then
`Infer`), and the step never persists it -- there is no
`StepPersistSpec` attached, since a model the caller already owns
outside the graph has no story for `save`/`load` to restore.

The owning overload (`step(name, input, output, IceModel, restore_key,
...)`) instead takes ownership of a moved-in model and, when
`restore_key` is non-empty, becomes persistable: [`save`](../core/Procedure/save.md)
writes the model's own parameters, and [`load`](../persist/load.md)
reconstructs a fresh instance via whatever factory was bound for
`restore_key` through [`bind_ice`](../registry/bind_ice.md) before
restoring its saved weights into it. An empty `restore_key` keeps the
owning overload's step unpersistable, same as the reference overload.

## Related Pages

- [`ice::Model`](../../ice/model/Model.md) -- the base class every
  wrapped model derives from.
- [`registry`](../registry.md) -- `bind_ice`, required before a
  `restore_key` can actually reload anything.
- [`module`](../module.md) -- the lower-level escape hatch for raw `nn`
  objects that are not full `ice::Model`s.
