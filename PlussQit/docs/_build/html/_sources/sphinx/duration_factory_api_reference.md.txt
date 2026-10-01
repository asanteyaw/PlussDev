<!-- pluss-hand-authored -->

# Duration Factory API Reference

[Back to PlussQit API Reference](../api_reference.md)

The free functions in `pluss::duration` construct and render elapsed-time
arrays. They are separate from the [Duration method reference](duration_api_reference.md):
factories answer how a value enters the type; methods answer what an existing
`Duration` can do.

## Choose By Intent

- Use `nanos`, `microseconds`, `milliseconds`, `seconds`, `minutes`, `hours`,
  `days`, or `weeks` when the source unit is known at the call site.
- Use `make(values, Unit)` when the unit is selected at runtime.
  `pluss::make_duration(...)` is its intentional root-level forwarding form.
- Use `range` for an inclusive, fixed-width one-dimensional progression.
- Use `zeros` and `full` for shape-first allocation; use their `_like`
  counterparts when an existing Duration supplies the shape.
- Use `random` for seeded or entropy-seeded uniform integer nanoseconds in a
  half-open interval.
- Use `print` for diagnostic rendering to a string or stream.

All duration values are stored as signed 64-bit nanoseconds. Construction is
exact: integer inputs never pass through floating point. Public arrays support
ranks zero through three; explicit shapes reject negative extents, cardinality
overflow, and flat-data length mismatches before allocating. Unit scaling also
checks signed-nanosecond overflow. `Duration::kNaT` is the reserved absent value.

The unit constructors accept four data geometries: one integer produces a
scalar; a contiguous integer range or flat braced list produces rank one;
rectangular nested braces infer rank and shape; and `(flat, sizes)` applies
runtime shape metadata. Every deterministic output in this family comes from
`examples/CodeH/duration_factory_reference_demo.cpp`; its transcript is
`docs/sphinx/_verified/duration_factory_reference_output.txt`.

```{toctree}
:hidden:
:maxdepth: 1

api/duration_factory/days
api/duration_factory/full
api/duration_factory/full_like
api/duration_factory/hours
api/duration_factory/make
api/duration_factory/microseconds
api/duration_factory/milliseconds
api/duration_factory/minutes
api/duration_factory/nanos
api/duration_factory/print
api/duration_factory/random
api/duration_factory/range
api/duration_factory/seconds
api/duration_factory/weeks
api/duration_factory/zeros
api/duration_factory/zeros_like
```

