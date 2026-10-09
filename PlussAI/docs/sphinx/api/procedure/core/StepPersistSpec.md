# `pluss::ai::StepPersistSpec`

How one [`Step`](Step.md) saves itself inside a [`Procedure`](Procedure.md)
bundle: a component key, schema versions, reloadable/frozen/custom
flags, the audit field list, and the actual `Saver` closure.

Attached via the `step(name, options, StepPersistSpec, runner)` overload
or built automatically by a built-in engine adapter; a plain
[`callable`](../callable.md) step with no explicit persistence has a
default-constructed one (`key` empty, no `Saver`).

## Fields

| Field | Meaning | Default | Written to bundle |
| --- | --- | --- | --- |
| `key` | Loader/component identity | `""` | Yes |
| `schema_version` | Version of the procedure-side persisted record | `1` | Yes |
| `component_schema_version` | Version of the wrapped component's own format | `0` | Yes |
| `reloadable` | Promise that the step can be reconstructed | `false` | Yes |
| `frozen` | Require immutable compatibility semantics | `false` | Yes |
| `custom` | Route restoration through the custom registry | `false` | Yes |
| `fields` | Additional textual contract metadata | `{}` | Yes |
| `save` | Callback that writes the component artifact | Empty | Invoked by `Procedure::save()`; not serialized itself |

### `key`

`std::string`, default empty. The identifier a saved bundle's manifest
records for this step's component, and what
[`procedure::load`](../persist/load.md) looks up to reconstruct it --
see [`Step::persist_key`](Step/persist_key.md).

### `schema_version`

`std::int64_t`, default `1`. Versions the *bundle entry's own* layout --
see [`Step::persist_schema_version`](Step/persist_schema_version.md) and
its distinction from `component_schema_version`.

### `component_schema_version`

`std::int64_t`, default `0`. Versions the *wrapped component's* own
saved format, independent of `schema_version` -- see
[`Step::component_schema_version`](Step/component_schema_version.md).

### `reloadable`

`bool`, default `false`. Whether
[`procedure::load`](../persist/load.md) is expected to be able to
reconstruct this step's component from the saved artifact -- see
[`Step::reloadable`](Step/reloadable.md).

### `frozen`

`bool`, default `false`. A signal that this component's saved form
should never change between saves -- see [`Step::frozen`](Step/frozen.md).

### `custom`

`bool`, default `false`. Whether this step's persistence goes through
the [`registry`](../registry.md)'s custom-contract mechanism rather than
a built-in adapter's own fixed logic -- see [`Step::custom`](Step/custom.md).

### `fields`

`std::vector<std::pair<std::string, std::string>>`, default empty. A flat
`key, value` metadata list written into the bundle's audit records
alongside the component artifact -- see
[`Step::persist_fields`](Step/persist_fields.md).

### `save`

`StepPersistSpec::Saver` (`std::function<void(std::string_view)>`),
default empty. The closure [`Step::save_component`](Step/save_component.md)
invokes with the destination path; left empty for a step with nothing
beyond its own metadata to write (a [`SinkSpec`](SinkSpec.md)-backed
sink step, for instance).

## Guidance

Every built-in engine adapter fills this struct in automatically with
values appropriate to what it wraps; fill it in by hand only for a
[`callable`](../callable.md) step that needs persistence outside the
[`registry`](../registry.md)'s key-based mechanism. `key` is the one
field every other piece of the persistence story depends on --
`schema_version`/`component_schema_version` are meaningless without it,
and `reloadable` promises that a real `StepLoader` exists for it
somewhere (built-in, or bound via the [`registry`](../registry.md)).

## See Also

- [`Step`](Step.md) -- every accessor this struct backs.
- [`registry`](../registry.md) -- `CustomContract`/`CustomPersist`, the
  caller-facing mechanism this struct's `custom` flag connects to.
- [`persist`](../persist.md) -- the bundle format this struct's fields
  end up written into.
