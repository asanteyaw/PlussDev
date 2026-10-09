# Timeline::TimelineIndexAccessor::align_union

[Back to Timeline Index Accessor API Reference](../../timeline_index_accessor_api_reference)

## Signature

```cpp
AlignUnion Timeline::TimelineIndexAccessor::align_union(const Timeline::TimelineIndexAccessor& other) const
```

## What It Does

Builds a union time axis plus left/right indexer maps into that union.

This is the time-series alignment primitive for "put these two irregular axes on
one shared clock, then tell me where each original axis lands."

It is the exact-match alternative to [`join_asof`](join_asof.md). Nothing is
matched approximately here. The union clock contains only real timestamps that
appear on at least one side.

The returned `AlignUnion` struct contains:

- `index`
  The union time axis as a `Timeline`.
- `left_indexer`
  A position map from the union axis into `self`, with `-1` where the left side
  has no timestamp.
- `right_indexer`
  A position map from the union axis into `other`, with `-1` where the right
  side has no timestamp.

Conceptually, the method performs three steps:

- build the shared exact clock
- locate the left axis on that clock
- locate the right axis on that clock

## Parameters

**other**

The other `Timeline::TimelineIndexAccessor` to align against.

## Returns

`AlignUnion`
    A struct containing the union axis and both side indexers.

## Examples

Example 1. Build one shared exact axis for two irregular time feeds.

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
    "2024-11-01T00:20:00Z",
    "2024-11-01T01:05:00Z",
    "2024-11-01T01:20:00Z",
}).index();

auto aligned = left.align_union(right);
std::cout << pluss::timeline::print(aligned.index) << "\n";
std::cout << pluss::series::print(aligned.left_indexer) << "\n";
std::cout << pluss::series::print(aligned.right_indexer) << "\n";
```

### Output

```text
Timeline(shape=[7], tz=UTC)
[ 2024-11-01 00:00:00, 2024-11-01 00:05:00, 2024-11-01 00:20:00, 2024-11-01 00:40:00, 2024-11-01 01:00:00, 2024-11-01 01:05:00, 2024-11-01 01:20:00 ]

Series(shape=[7], strides=[1], dtype=int64, platform=cpu, numel=7, contiguous=true)
data=[0, -1, 1, 2, 3, -1, 4]

Series(shape=[7], strides=[1], dtype=int64, platform=cpu, numel=7, contiguous=true)
data=[-1, 0, 1, -1, -1, 2, 3]
```

Example 2. Gather aligned payloads after building the union.

```cpp
auto left_vals = pluss::make_series(std::vector<double>{12.0, 18.0, 15.0});
auto right_vals = pluss::make_series(std::vector<double>{101.0, 103.0, 102.0});

auto shared = left.align_union(right);

// Use shared.left_indexer and shared.right_indexer as the alignment maps.
std::cout << pluss::timeline::print(shared.index) << "\n";
std::cout << pluss::series::print(shared.left_indexer) << "\n";
std::cout << pluss::series::print(shared.right_indexer) << "\n";
```

## Notes

- `align_union()` is about exact-axis union, not nearest-neighbor matching.
- The output axis is a `Timeline`, because the union itself is the temporal
  payload; index semantics can be restored later if needed.
- A timestamp present on both sides appears once in the union and maps to both
  indexers.
- A left-only timestamp yields a valid left index and `-1` on the right side.
- A right-only timestamp yields `-1` on the left side and a valid right index.
- This method is ideal when you need both series represented on one exact set of
  timestamps before further computation.

## See Also

- [`Timeline::TimelineIndexAccessor::join_asof`](join_asof.md): nearest-key alignment instead of exact
  union alignment.
- [`Timeline::TimelineIndexAccessor::timeline`](timeline.md): continue working directly on the union
  axis as a temporal payload.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="../../timeline_index_accessor_api_reference.html">
    <span class="label">Previous</span>
    <span class="title">Timeline Index Accessor API Reference</span>
  </a>
  <a class="next-link" href="argsort.html">
    <span class="label">Next</span>
    <span class="title">argsort</span>
  </a>
</div>
