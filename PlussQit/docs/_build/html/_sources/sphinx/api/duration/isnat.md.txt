<!-- pluss-hand-authored -->

# Duration::isnat

[Back to Duration API Reference](../../duration_api_reference)

## Signature

```cpp
Series Duration::isnat() const;
Series Duration::isnan() const;  // alias for isnat()
```

## What It Does

`Duration` has the same missing-value convention as [`Timeline`](../timeline/isnat.md):
a sentinel value (`Duration::kNaT`) represents "no duration," and `isnat()`
tests for it elementwise, returning a boolean `Series`.

## Parameters

None — this is a zero-argument accessor on `self`.

## Returns

`Series`
    A boolean `Series` at `self`'s shape, one test result per position.

## Examples

Example 1. A valid duration and an explicitly-missing one.

```cpp
auto one_hour = pluss::duration::hours(1);
auto missing = pluss::duration::nanos(pluss::Duration::kNaT);
std::cout << "valid.isnat=" << pluss::print_series(one_hour.isnat()) << "\n";
std::cout << "missing.isnat=" << pluss::print_series(missing.isnat());
```

### Output

```text
valid.isnat=Series(shape=[], strides=[], dtype=bool, platform=cpu, numel=1, contiguous=true)
data=false
missing.isnat=Series(shape=[], strides=[], dtype=bool, platform=cpu, numel=1, contiguous=true)
data=true
```

## Notes

- `Duration::kNaT` is a `static constexpr std::int64_t` sentinel value (shared
  with `Timeline::kNaT` — both alias `Duration::kNaT` under the hood), not a
  distinct type; build a missing `Duration` with
  `pluss::duration::nanos(Duration::kNaT)`.
- `isnan()` is a pure alias for `isnat()` — provided for readers coming from a
  floating-point-NaN background, not a separate concept.

## See Also

- [`isvalid`](isvalid.md): the negation of this test.
- [`fill_nat`](fill_nat.md): replacing missing positions with a real value.
- [`where`](where.md): conditional selection, another mask-driven operation.
- [`Timeline::isnat`](../timeline/isnat.md): the same convention on the
  instant side.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="total_weeks.html">
    <span class="label">Previous</span>
    <span class="title">total_weeks</span>
  </a>
  <a class="next-link" href="isvalid.html">
    <span class="label">Next</span>
    <span class="title">isvalid</span>
  </a>
</div>
