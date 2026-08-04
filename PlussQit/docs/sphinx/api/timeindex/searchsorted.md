# TimeIndex::searchsorted

[Back to TimeIndex API Reference](../../timeindex_api_reference)

## Signature

```cpp
TimeIndex::searchsorted(const Timeline& key, bool right = false) const -> std::int64_t
```

## What It Does

Returns the insertion position for a scalar timestamp in the sorted index.

This is the temporal equivalent of a lower-bound or upper-bound search:

- `right = false` gives the left insertion point.
- `right = true` gives the right insertion point.

That makes `searchsorted()` a load-bearing primitive for building time slices,
bucketing event streams, or mapping ad-hoc timestamps into the nearest valid
interval boundary.

At the `TimeIndex` level, this is one of the most fundamental ordered-axis
questions: if this timestamp were inserted here, where would it land?

## Parameters

**key**

The timestamp to insert. It must be a scalar `Timeline`, not a vector of times.

If your input is still text, parse or construct the scalar timestamp first and
then search against the axis.

**right**

Controls which side of equal timestamps is chosen.

- `false`
  Return the first valid insertion position.
- `true`
  Return the insertion position after any equal timestamps already present.

## Returns

`std::int64_t`
    An insertion position in `[0, size()]`.

## Examples

Example 1. Find the insertion position for an in-between timestamp.

```cpp
auto idx = pluss::timeindex::make({
    "2024-11-01T00:00:00Z",
    "2024-11-01T00:20:00Z",
    "2024-11-01T00:40:00Z",
    "2024-11-01T01:00:00Z",
    "2024-11-01T01:20:00Z",
});

auto key = pluss::timeline::make("2024-11-01T00:55:00Z");

auto left_pos = idx.searchsorted(key, false);
auto right_pos = idx.searchsorted(key, true);
std::cout << "left_pos = " << left_pos << "\n";
std::cout << "right_pos = " << right_pos << "\n";
```

### Output

```text
left_pos = 3
right_pos = 3
```

Example 2. Toggle `right` when you care about which side of an equal run should
own the insertion point.

```cpp
auto pos = idx.searchsorted(key, /*right=*/true);
std::cout << "pos = " << pos << "\n";
```

This distinction becomes important once duplicates exist and you need to decide
whether the new case belongs before or after the equal block.

## Notes

- `searchsorted()` expects a scalar key. Passing a vector of keys is not this
  method's job.
- `right = false` behaves like a left insertion point.
- `right = true` behaves like an upper-bound style insertion point.
- The semantics only make sense against a sorted `TimeIndex`.
- Use `searchsorted()` when you need the raw insertion logic itself. Use
  `slice()` or `asof()` when you want a higher-level answer built on top of the
  same ordering semantics.
- This is one of the core building blocks underneath `slice`, `slice_time`, and
  `asof`-style logic.

## See Also

- [`TimeIndex::asof`](asof.md): get the last observed timestamp position at or
  before a key.
- [`TimeIndex::slice`](slice.md): turn two temporal bounds into a range of
  matching case positions.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="host.html">
    <span class="label">Previous</span>
    <span class="title">host</span>
  </a>
  <a class="next-link" href="slice.html">
    <span class="label">Next</span>
    <span class="title">slice</span>
  </a>
</div>
