# `pluss::ai::procedure::CustomPersist`

The lightweight persistence request passed to
`step(..., CustomPersist, runner)`: a registry key that must already be
bound via [`bind`](bind.md), plus reloadable/frozen flags, audit fields,
and a `Saver`.

Passed directly to the generic `step(name, options, CustomPersist,
runner)` overload (see [`step`](step.md)), or built implicitly by
[`callable`](../callable.md)'s own keyed `step` overload.

## Fields

### `key`

`std::string`, no default. Must already be bound via
[`bind`](bind.md) -- [`step`](step.md) throws immediately if nothing is
bound for it.

### `reloadable`

`bool`, default `true`. Whether [`procedure::load`](../persist/load.md)
is expected to reconstruct this step -- see
[`reloadable`](../core/Step/reloadable.md).

### `frozen`

`bool`, default `false`. See [`frozen`](../core/Step/frozen.md).

### `fields`

`std::vector<std::pair<std::string, std::string>>`, default empty.
Extra audit metadata -- see
[`persist_fields`](../core/Step/persist_fields.md).

### `save`

`StepPersistSpec::Saver`, default empty. The closure
[`save_component`](../core/Step/save_component.md) invokes.

## Guidance

Build one directly only when hand-constructing a step via the generic
`step(name, options, CustomPersist, runner)` overload rather than going
through [`callable`](../callable.md)'s own keyed `step` sugar, which
builds a `CustomPersist` (with a fixed `fields = {{"external_ref",
"registry"}}` entry) automatically.

## See Also

- [`step`](step.md) -- the overload this feeds.
- [`CustomContract`](CustomContract.md) -- the fuller record this
  depends on already being bound.
