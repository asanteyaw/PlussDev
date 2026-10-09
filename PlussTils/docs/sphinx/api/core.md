# Core Utilities API Reference

[Back to PlussTils API Reference](../api_reference.md) · [Core Manual](../manual/core.md)

The Core reference owns public operations in `pluss::tils`: environment access,
compact value declaration, assumption checks, scalar conversion, output,
callable transmission, and axis-wise mapping. Each operation will receive a
separate A-Z page after its implementation, overloads, errors, and exact output
have been verified.

Implementation helpers and former global forwarding declarations are not part
of this surface.

## Families

- [Deterministic Seeds](core/seeds.md): reset, inspect, and allocate from the
  process-wide seed stream used by default-seeded convenience operations.
- [Environment](core/environment.md): copy and parse process-environment values
  with explicit absence and fallback contracts.
- [Scalar conversion](core/scalar-casts.md): construct typed scalar Series and
  extract scalar host values without implicit dtype guessing.
- [Dispatch](core/dispatch.md): apply callables to values, ranges, zipped
  ranges, or oriented slices.
- [Output](core/output.md): the output engine, its levels and options, and every
  member that writes or logs values.
- [Print options](core/print-options.md): every field, builder, and preset of
  `PrintOptions`, and the enumerations they use.
- [Value flow](core/value-flow.md): immutable `let` bindings and fluent
  `suppose` transformations, each with its own reference page.

```{toctree}
:maxdepth: 2
:hidden:

core/seeds
core/environment
core/scalar-casts
core/dispatch
core/value-flow
core/output
core/print-options
core-suppose
core-let
```
