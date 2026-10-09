# `pluss::ai::Link`

A directed edge between two [`Step`](Step.md)s by name, with optional
named-output/named-input endpoints for steps that produce or accept more
than one artifact.

Built with [`procedure::link`](../build/link.md), then passed to
[`procedure::make`](../build/make.md) alongside the step list.
[`Procedure::check`](Procedure/check.md) validates every link's
endpoints (and, where given, its named ports) before a `Procedure` value
exists.

## Fields

### `from`

`std::string`, no default (required at construction via
[`procedure::link`](../build/link.md), which throws on an empty string).
The name of the upstream step.

### `to`

`std::string`, no default (required, same as `from`). The name of the
downstream step.

### `from_output`

`std::string`, default empty. Which of `from`'s declared
[`outputs`](Step/outputs.md) to read, for a step with more than one. Left
empty for a step with a single (or no) declared output, where there is
nothing to disambiguate.

### `to_input`

`std::string`, default empty. Which of `to`'s declared
[`inputs`](Step/inputs.md) to feed, for a step with more than one. Left
empty the same way as `from_output`.

## Guidance

Most links in practice leave `from_output`/`to_input` empty -- most
steps have exactly one input and one output, so there is nothing to
disambiguate. Fill them in for a [`Join`](Introduction.md)-capability
step with several named inputs, or any step with several named outputs;
[`Procedure::check`](Procedure/check.md) rejects a named port that the
referenced step never declared.

## See Also

- [`procedure::link`](../build/link.md) -- the only way to build one.
- [`Step::inputs`](Step/inputs.md) / [`Step::outputs`](Step/outputs.md)
  -- what `from_output`/`to_input` address.
