# `pluss::ai::SinkSpec`

A terminal step's single field: the target name its one input artifact
is published under.

Built with [`procedure::sink`](../build/sink.md), then passed to the
`step(name, SinkSpec)` overload -- see [`build`](../build.md) -- to
create a persistable, [`Sink`](Introduction.md)-capability step with no
runner of its own.

## Fields

| Field | Meaning | Default |
| --- | --- | --- |
| `target` | Conceptual publication destination recorded in step metadata | No valid empty default; `procedure::sink("")` throws |

### `target`

`std::string`, no default (required at construction via
[`procedure::sink`](../build/sink.md), which throws on an empty string).
A name describing where this sink's artifact is conceptually headed --
not a filesystem path or live connection; `Step::run`'s own `Sink`
fallback simply passes the one input through under the *step's* name,
regardless of what `target` says. `target` itself is recorded as this
step's [`component`](Step/component.md) and as a
[`persist_fields`](Step/persist_fields.md) entry, for a saved bundle's
audit trail to show what the sink was conceptually pointed at.

## Guidance

A sink step is `procedure`'s way of marking "this value is a published,
terminal result" without requiring an actual I/O side effect baked into
the graph -- `target` is documentation, read from the saved bundle's
audit trail, not a live destination `run()` writes to.

## See Also

- [`procedure::sink`](../build/sink.md) -- the only way to build one.
- [`build`](../build.md) -- the `step(name, SinkSpec)` overload that
  consumes it.
