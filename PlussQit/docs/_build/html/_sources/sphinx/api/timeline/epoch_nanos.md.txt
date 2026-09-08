<!-- pluss-hand-authored -->

# Timeline::epoch_nanos

[Back to Timeline API Reference](../../timeline_api_reference)

## Signature

```cpp
Series Timeline::epoch_nanos() const
```

## What It Does

`epoch_nanos()` exposes the timeline's machine-time values as an `Int64`
`Series` while preserving the timeline's logical shape. It is the bridge to
numeric operations: masks, reshaping, serialization utilities, exact
differences, and custom computations that naturally operate on `Series`.

Unlike [`ticks()`](ticks.md), this method does
not flatten a matrix or 3-D temporal array into a C++ vector. Unlike
[`host()`](host.md), it returns one stable result type regardless of rank and
cardinality.

The values are UTC epoch nanoseconds. The timeline's presentation timezone is
not embedded in the numeric `Series`; retain `tz()` separately if the values
will later be reconstructed with the same wall-clock presentation.

The result is newly materialized rather than a mutable view into the
`Timeline`. Numeric transformations of the returned `Series` do not rewrite
the source timeline.

## Returns

An `Int64` `Series` with the same shape and number of positions as the source
timeline. Absent instants appear as `Timeline::kNaT`.

## Examples

Example 1. Preserve matrix shape while exposing exact ticks.

```cpp
auto matrix = pluss::timeline::make({
    {"2024-02-29T12:30:00Z", "2024-03-01T00:00:00Z"},
    {"2024-03-02T06:15:00Z", ""},
});
pluss::series::print(std::cout, matrix.epoch_nanos());
std::cout << "\n";
```

### Output

```text
Series(shape=[2,2], strides=[2,1], dtype=int64, platform=cpu, numel=4, contiguous=true)
data=
⎡ 1709209800000000000  1709251200000000000 ⎤
⎣ 1709360100000000000 -9223372036854775808 ⎦
```

Example 2. Compare shape-preserving and flattening exports.

```cpp
auto matrix = pluss::timeline::epoch({2, 3});
auto series = matrix.epoch_nanos();
auto vector = matrix.ticks();

std::cout << "series dim=" << series.dim()
          << ", series numel=" << series.numel()
          << ", vector size=" << vector.size() << "\n";
```

### Output

```text
series dim=2, series numel=6, vector size=6
```

Example 3. Build a validity mask without mistaking the `NaT` sentinel for an
ordinary early date.

```cpp
auto times = pluss::timeline::make({
    "2024-02-29T12:30:00Z", "", "2024-03-01T00:00:00Z"
});
auto valid = times.epoch_nanos().ne(pluss::Timeline::kNaT);
pluss::series::print(std::cout, valid);
std::cout << "\n";
```

### Output

```text
Series(shape=[3], strides=[1], dtype=bool, platform=cpu, numel=3, contiguous=true)
data=[true, false, true]
```

For ordinary validity work, [`isvalid()`](isvalid.md) communicates this
intent more directly. The numeric form is useful when a larger numeric
pipeline already operates on epoch ticks.

## Gotchas And Failure Modes

- The returned `Series` contains no timezone metadata. Save `tz()` separately
  for a presentation-preserving round trip.
- Arithmetic on raw epoch values is integer arithmetic. Prefer Timeline and
  Duration methods when calendar or absent-instant semantics matter.
- Never treat `Timeline::kNaT` as an ordinary timestamp; filter or mask it
  before ordering or arithmetic in a generic numeric pipeline.
- This method preserves shape; call `flatten()` explicitly if a flat `Series`
  is required.
- The conversion allocates an independent `Series`; cache it if several
  downstream numeric operations will reuse the same ticks.

## Summary

Use `epoch_nanos()` for a shape-preserving numeric bridge, the vector methods
for owning flat C++ data, and `host()` when a rank-sensitive host variant is
the desired integration format.

## See Also

- [`Timeline::ticks`](ticks.md)
- [`Timeline::host`](host.md)
- [`Timeline::isvalid`](isvalid.md)
- [`Timeline::tz`](tz.md)

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="copy_ticks.html"><span class="label">Previous</span><span class="title">copy_ticks</span></a>
  <a class="next-link" href="epoch_micros.html"><span class="label">Next</span><span class="title">epoch_micros</span></a>
</div>
