<!-- pluss-hand-authored -->

# Timeline Factory API Reference

[Back to PlussQit API Reference](../api_reference.md)

This reference teaches the free functions in `pluss::timeline`. They answer a
different question from the [`Timeline` method reference](timeline_api_reference.md):
how to create, combine, choose, or render instants before there is a receiver on
which to call a method.

## Choose By Intent

- Parse complete timestamps with `make`; require date-only or clock-only input
  with `date` and `time`.
- Use `make_timeline` only when the flat construction shortcut reads more
  naturally at the call site; it is an exact alias of `timeline::make`.
- Preserve exact integer nanoseconds with `from_epoch`.
- Build regular sequences with `date_range` or `arange`; their step and endpoint
  contracts differ and are documented separately.
- Allocate known shapes with `epoch`, `full`, or `full_like`.
- Sample half-open nanosecond intervals with `random`, or inclusive intervals
  with `rand_between`.
- Concatenate flattened inputs with `unite`, and choose by ordered conditions
  with `select`.
- Render a diagnostic representation with `print`, or shape-preserving text
  values with `format`.
- Resolve fixed or relative scalar expressions with `moment`, `now`, and `today`.

Every deterministic output in these pages is produced by
`examples/CodeH/timeline_factory_reference_demo.cpp`; its exact transcript is
checked in at
`docs/sphinx/_verified/timeline_factory_reference_output.txt`. Seeded examples
make the random pages reproducible; live-clock examples state invariants rather
than pretending that `now()` has a fixed output.

In signatures, `string_range` and `integer_range` mean any accepted contiguous
range of those values; `nested_string_values` means a rectangular braced literal
up to rank three. Those labels describe call-site concepts and intentionally do
not expose the private reader adapters that implement overload unification.

```{toctree}
:hidden:
:maxdepth: 1

api/timeline_factory/arange
api/timeline_factory/date
api/timeline_factory/date_range
api/timeline_factory/epoch
api/timeline_factory/format
api/timeline_factory/from_epoch
api/timeline_factory/full
api/timeline_factory/full_like
api/timeline_factory/make
api/timeline_factory/make_timeline
api/timeline_factory/moment
api/timeline_factory/now
api/timeline_factory/print
api/timeline_factory/rand_between
api/timeline_factory/random
api/timeline_factory/select
api/timeline_factory/time
api/timeline_factory/today
api/timeline_factory/unite
```

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="../api_reference.html"><span class="label">Previous</span><span class="title">PlussQit API Reference</span></a>
  <a class="next-link" href="api/timeline_factory/arange.html"><span class="label">Next</span><span class="title">arange</span></a>
</div>
