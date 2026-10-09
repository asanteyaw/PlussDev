# Timeline::TimelineIndexAccessor::join_asof

[Back to Timeline Index Accessor API Reference](../../timeline_index_accessor_api_reference)

## Signature

```cpp
Timeline::TimelineIndexAccessor::join_asof(
    const Timeline::TimelineIndexAccessor& right,
    AsofDirection direction = AsofDirection::Backward,
    const Duration* tolerance = nullptr,
    bool allow_exact_matches = true) const -> pluss::Series
```

## What It Does

Performs vectorized nearest-key matching from the left index to a right index.

For every timestamp in `self`, `join_asof()` returns the matching position in the
right index, or `-1` when no valid match exists under the requested direction
and tolerance rules.

This is one of the most practical `Timeline::TimelineIndexAccessor` methods because it powers:

- event-to-market-state alignment
- quote-to-trade matching
- left-preserving feature enrichment
- "most recent known value" joins across two irregular time axes

`join_asof()` solves the temporal matching problem first. It does not merge the
payload for you. That is why the result is a right-side position map rather than
an already-materialized frame.

## Parameters

**right**

The right-hand index to match against.

Treat `self` as the axis you want to preserve and `right` as the axis you want
to look into.

**direction**

Which side of time to search on the right index.

Supported values:

- `AsofDirection::Backward`
  Match the latest right timestamp less than or equal to the left timestamp.
- `AsofDirection::Forward`
  Match the earliest right timestamp greater than or equal to the left timestamp.
- `AsofDirection::Nearest`
  Match whichever right timestamp is closest in absolute time distance.

This is the main semantic switch in the call.

**tolerance**

An optional maximum permitted time gap.

- `nullptr`
  No extra tolerance cap.
- pointer to a `Duration`
  Reject matches whose gap exceeds that duration.

Tolerance is applied after the directional candidate has been chosen.

**allow_exact_matches**

Controls whether equal timestamps count as valid matches.

- `true`
  Equal timestamps are allowed.
- `false`
  Exact timestamp ties are excluded.

## Returns

`pluss::Series`
    A 1D integer `Series` of right-hand positions, one per left timestamp, with
    `-1` for unmatched rows.

## Examples

Example 1. Backward as-of matching: the latest right timestamp less than or
equal to each left timestamp.

```cpp
auto left = pluss::timeline::make({
    "2024-11-01T00:00:00Z",
    "2024-11-01T00:20:00Z",
    "2024-11-01T00:40:00Z",
    "2024-11-01T01:00:00Z",
    "2024-11-01T01:20:00Z",
}).index();

auto right = pluss::timeline::make({
    "2024-11-01T00:05:00Z",
    "2024-11-01T00:30:00Z",
    "2024-11-01T01:10:00Z",
}).index();

auto map = left.join_asof(right);
std::cout << pluss::series::print(map) << "\n";
```

### Output

```text
Series(shape=[5], strides=[1], dtype=int64, platform=cpu, numel=5, contiguous=true)
data=[-1, 0, 1, 1, 2]
```

Example 2. Switch to nearest-neighbor matching and cap the permitted distance
with `tolerance`.

```cpp
auto tol = pluss::duration::minutes(25);
auto map = left.join_asof(
    right,
    pluss::Timeline::TimelineIndexAccessor::AsofDirection::Nearest,
    &tol,
    true);
std::cout << pluss::series::print(map) << "\n";
```

### Output

```text
Series(shape=[5], strides=[1], dtype=int64, platform=cpu, numel=5, contiguous=true)
data=[0, 1, 1, 2, 2]
```

Example 3. Switch to forward matching when the left axis should look ahead.

```cpp
auto map = left.join_asof(
    right,
    pluss::Timeline::TimelineIndexAccessor::AsofDirection::Forward);
std::cout << pluss::series::print(map) << "\n";
```

### Output

```text
Series(shape=[5], strides=[1], dtype=int64, platform=cpu, numel=5, contiguous=true)
data=[0, 1, 2, 2, -1]
```

## Notes

- `join_asof()` preserves the left index cardinality.
- The output is a position map, not a merged table by itself.
- `Backward` is usually the correct mode for "latest known state."
- `Forward` is usually the correct mode for "next scheduled or next observed event."
- `Nearest` is usually the correct mode for symmetric sensor alignment, often
  paired with a non-null tolerance.
- Use the result to gather right-hand values or rows after the match has been
  established.

## See Also

- [`Timeline::TimelineIndexAccessor::asof`](asof.md): scalar backward lookup.
- [`Timeline::TimelineIndexAccessor::align_union`](align_union.md): union-style alignment rather than
  nearest-key matching.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="is_unique.html">
    <span class="label">Previous</span>
    <span class="title">is_unique</span>
  </a>
  <a class="next-link" href="max.html">
    <span class="label">Next</span>
    <span class="title">max</span>
  </a>
</div>
