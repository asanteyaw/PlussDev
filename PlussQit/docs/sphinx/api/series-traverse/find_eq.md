<!-- pluss-hand-authored -->

# Series::TraverseAccessor::find_eq

[Back to Series::TraverseAccessor API Reference](../../series_traverse_accessor_api_reference.md)

## Signatures

```cpp
TraverseMatch Series::TraverseAccessor::find_eq(double rhs) const;
TraverseMatch Series::TraverseAccessor::find_eq(const Series& rhs) const;

TraverseMatch SeriesTraverseProxy::find_eq(double rhs) const;
TraverseMatch SeriesTraverseProxy::find_eq(const Series& rhs) const;
```

## What It Does

`find_eq(rhs)` returns the coordinates of every position whose value equals
`rhs`. It is [`find_all`](find_all.md) with the mask `source.eq(rhs)` built for
you.

Two forms:

- **`find_eq(double)`** compares every element against one scalar.
- **`find_eq(const Series&)`** compares element by element against a peer
  `Series`, following ordinary broadcasting, so the threshold can vary by row,
  column, or position.

The negation is [`find_neq`](find_neq.md).

Equality is exact floating-point equality. Values produced by arithmetic rather
than written literally may not compare equal; use [`find_btw`](find_btw.md) with
a small band when you need tolerance.

## Parameters

**rhs** (`double` or `const Series&`)

The comparison value. A `Series` `rhs` is broadcast against the traversed
`Series` (or sub-view); the result mask must resolve to the traversed shape.

## Returns

[`TraverseMatch`](match.md)
    Coordinates and matched values of the equal positions, row-major. `K` can
    be `0`.

## Gradient

Coordinates carry no gradient. `match.values()` and `match.select_from(other)`
are differentiable gathers.

## Examples

Example 1. Find one exact value.

```cpp
auto grid = pluss::make_series({
    {10.0, 4.0, 7.0},
    {2.0, 9.0, 6.0},
    {8.0, 1.0, 5.0},
});

auto hits = grid.traverse.find_eq(9.0);

std::cout << "count=" << hits.count() << "\n";
std::cout << hits.coords();
```

### Output

```text
count=1
pluss::Series(shape=[1,2], strides=[2,1], dtype=int64, platform=cpu, numel=2, contiguous=true)
data=[[1, 1]]
```

Example 2. A repeated value returns every occurrence, in row-major order.

```cpp
auto board = pluss::make_series({
    {1.0, 0.0, 1.0},
    {0.0, 1.0, 0.0},
});

auto ones = board.traverse.find_eq(1.0);

std::cout << "count=" << ones.count() << "\n";
std::cout << ones.coords();
```

### Output

```text
count=3
pluss::Series(shape=[3,2], strides=[2,1], dtype=int64, platform=cpu, numel=6, contiguous=true)
data=[[0, 0],
  [0, 2],
  [1, 1]]
```

Example 3. A `Series` right-hand side gives a per-column target.

```cpp
auto grid = pluss::make_series({
    {10.0, 4.0, 7.0},
    {2.0, 9.0, 6.0},
    {8.0, 1.0, 5.0},
});

auto targets = pluss::make_series({8.0, 9.0, 7.0});   // one per column
auto hits = grid.traverse.find_eq(targets);

std::cout << "count=" << hits.count() << "\n";
std::cout << hits.coords();
```

### Output

```text
count=3
pluss::Series(shape=[3,2], strides=[2,1], dtype=int64, platform=cpu, numel=6, contiguous=true)
data=[[0, 2],
  [1, 1],
  [2, 0]]
```

Column `0` matched its target `8` at row `2`, column `1` matched `9` at row
`1`, column `2` matched `7` at row `0`.

Example 4. A value that is never present returns an empty match.

```cpp
auto grid = pluss::make_series({
    {10.0, 4.0, 7.0},
    {2.0, 9.0, 6.0},
    {8.0, 1.0, 5.0},
});

std::cout << "count=" << grid.traverse.find_eq(3.5).count();
```

### Output

```text
count=0
```

## Gotchas And Failure Modes

- Exact floating-point equality. Prefer [`find_btw`](find_btw.md) with a narrow
  band for computed values.
- A `Series` `rhs` follows broadcasting, but the broadcast result must match the
  traversed shape.
- Coordinates are non-differentiable; `values()` and `select_from()` are.

## Summary

`find_eq(rhs)` finds every position equal to a scalar or a broadcast peer:

- convenience over [`find_all`](find_all.md) with an `eq` mask
- exact equality; [`find_neq`](find_neq.md) is the negation
- empty match when nothing is equal

## See Also

- [`find_neq`](find_neq.md): the negation.
- [`find_btw`](find_btw.md): tolerance-band matching for computed values.
- [`find_all`](find_all.md): supply your own mask.
- [`TraverseMatch`](match.md): the result object.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="find_btw.html">
    <span class="label">Previous</span>
    <span class="title">find_btw</span>
  </a>
  <a class="next-link" href="find_first.html">
    <span class="label">Next</span>
    <span class="title">find_first</span>
  </a>
</div>
