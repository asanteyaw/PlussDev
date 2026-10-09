# `pluss::ai::procedure::CustomContract`

The full registry record for a custom persistable step kind: its key,
schema versions, frozen flag, the `StepLoader` that reconstructs a
`Step` from a saved bundle, and an optional `CompatibilityGate` for
custom version-compatibility logic.

Built by hand and passed to [`bind`](bind.md), or built implicitly by
[`callable::bind`](../callable/bind.md) for the common single-callable
case.

## Fields

### `key`

`std::string`, default empty. The registry key this contract is bound
under -- what [`procedure::load`](../persist/load.md) looks up by a
saved step's own [`persist_key`](../core/Step/persist_key.md).

### `persist_schema_version`

`std::int64_t`, default `1`. The bundle-entry-layout version
[`load`](../persist/load.md) checks against the saved step's own
[`persist_schema_version`](../core/Step/persist_schema_version.md) --
unless `compatible` is set, in which case that check is skipped in
favor of `compatible`'s own logic.

### `component_schema_version`

`std::int64_t`, default `1`. The wrapped-component-format version,
checked the same way as `persist_schema_version` -- see
[`component_schema_version`](../core/Step/component_schema_version.md).

### `frozen`

`bool`, default `false`. A signal that this component's saved form
should never change between saves -- see
[`frozen`](../core/Step/frozen.md).

### `load`

`StepLoader` (`std::function<Step(std::string, StepOptions, std::string_view, const std::vector<std::pair<std::string, std::string>>&)>`),
default empty. Reconstructs a [`Step`](../core/Step.md) given the saved
step's name, its manifest-recorded `StepOptions`, the path to its
component artifact, and its recorded
[`persist_fields`](../core/Step/persist_fields.md). Must be callable --
[`bind`](bind.md) throws if it is not.

### `compatible`

`CompatibilityGate` (`std::function<std::optional<std::string>(std::int64_t, std::int64_t, const std::vector<std::pair<std::string, std::string>>&)>`),
default empty. An optional custom check, given the saved step's own
`persist_schema_version`/`component_schema_version`/`persist_fields`,
returning `std::nullopt` to accept or a message to reject -- replaces
the default exact-version-match check entirely when present.

## Guidance

Most callers never build a `CustomContract` by hand -- reach for
[`callable::bind`](../callable/bind.md) for a single callable, which
builds one automatically. Build one directly only for a component whose
reload logic needs more than `callable::bind` provides: several related
fields read back from `persist_fields`, or a `compatible` gate that
accepts a range of versions instead of requiring an exact match.

## See Also

- [`bind`](bind.md) -- the only way to register one.
- [`CustomPersist`](CustomPersist.md) -- the lighter request `step`
  itself takes, which depends on a contract already being bound.
