<!-- pluss-hand-authored -->

# SeriesTraverseAccessor::find_and

[Back to SeriesTraverseAccessor API Reference](../../series_traverse_accessor_api_reference.md)

## Signatures

```cpp
TraverseMatch SeriesTraverseAccessor::find_and(const Series& lhs_mask,
                                               const Series& rhs_mask) const;
TraverseMatch SeriesTraverseProxy::find_and(const Series& lhs_mask,
                                            const Series& rhs_mask) const;
```

The companions [`find_or`](find_or.md) and [`find_not`](find_not.md) cover the
other two boolean combinators.

## What It Does

`find_and(lhs_mask, rhs_mask)` returns the coordinates of every position where
**both** masks are true. It is [`find_all`](find_all.md) with the mask
`lhs_mask.logical_and(rhs_mask)`.

The whole family — `find_and`, [`find_or`](find_or.md), [`find_not`](find_not.md)
— exists to combine two conditions you already have as masks without writing an
intermediate `logical_and` / `logical_or` / `logical_not` and a separate
`find_all`. When the two conditions are simple comparisons against the *same*
`Series`, [`find_btw`](find_btw.md) is often the more direct tool; `find_and`
earns its place when the two masks come from different sources or different
logic.

Both masks are interpreted as "non-zero means true" and must match the
traversed shape (no broadcasting between the two masks or against the source).

## Parameters

**lhs_mask** (`const Series&`)

The first condition, shaped like the traversed `Series`.

**rhs_mask** (`const Series&`)

The second condition, same shape.

## Returns

[`TraverseMatch`](match.md)
    Row-major coordinates and matched values of the positions satisfying both
    masks. `K` can be `0`.

## Gradient

Coordinates carry no gradient. `match.values()` and `match.select_from(other)`
are differentiable gathers.

## Examples

Example 1. Positions that are both at least `5` and at most `9`.

```cpp
auto grid = pluss::make_series({
    {10.0, 4.0, 7.0},
    {2.0, 9.0, 6.0},
    {8.0, 1.0, 5.0},
});

auto hits = grid.traverse.find_and(grid.ge(5.0), grid.le(9.0));

std::cout << "count=" << hits.count() << "\n";
std::cout << hits.coords();
std::cout << hits.values();
```

### Output

```text
count=5
pluss::Series(shape=[5,2], strides=[2,1], dtype=int64, platform=cpu, numel=10, contiguous=true)
data=[[0, 2],
  [1, 1],
  [1, 2],
  [2, 0],
  [2, 2]]
pluss::Series(shape=[5], strides=[1], dtype=float64, platform=cpu, numel=5, contiguous=true)
data=[7, 9, 6, 8, 5]
```

`10` fails the upper mask and `4`, `2`, `1` fail the lower one, so they are
excluded.

Example 2. Two masks from different `Series`: keep positions where `grid` is
high *and* the aligned `cost` is low.

```cpp
auto grid = pluss::make_series({
    {10.0, 4.0, 7.0},
    {2.0, 9.0, 6.0},
    {8.0, 1.0, 5.0},
});
auto cost = pluss::make_series({
    {9.0, 1.0, 2.0},
    {5.0, 3.0, 8.0},
    {1.0, 6.0, 4.0},
});

auto hits = grid.traverse.find_and(grid.ge(6.0), cost.le(4.0));

std::cout << "count=" << hits.count() << "\n";
std::cout << hits.values();
```

### Output

```text
count=3
pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[7, 9, 8]
```

`grid >= 6` at five spots; of those, `cost <= 4` only at `grid[0,2]=7`,
`grid[1,1]=9`, `grid[2,0]=8`.

## Gotchas And Failure Modes

- Both masks must match the traversed shape exactly. There is no broadcasting.
- "Non-zero means true" — a non-boolean mask is accepted.
- When both conditions compare the same `Series` to constants, prefer
  [`find_btw`](find_btw.md).
- Coordinates are non-differentiable; `values()` and `select_from()` are.

## Summary

`find_and(a, b)` finds every position where two masks are both true:

- convenience over [`find_all`](find_all.md) with a `logical_and`
- pairs with [`find_or`](find_or.md) and [`find_not`](find_not.md)
- exact shape match on both masks

## See Also

- [`find_or`](find_or.md): either mask true.
- [`find_not`](find_not.md): a mask false.
- [`find_btw`](find_btw.md): the direct tool when both conditions bound the same
  `Series`.
- [`TraverseMatch`](match.md): the result object.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="find_all.html">
    <span class="label">Previous</span>
    <span class="title">find_all</span>
  </a>
  <a class="next-link" href="find_btw.html">
    <span class="label">Next</span>
    <span class="title">find_btw</span>
  </a>
</div>
