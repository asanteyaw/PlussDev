<!-- pluss-hand-authored -->

# Series::TraverseAccessor::find_not

[Back to Series::TraverseAccessor API Reference](../../series_traverse_accessor_api_reference.md)

## Signatures

```cpp
TraverseMatch Series::TraverseAccessor::find_not(const Series& mask) const;
TraverseMatch SeriesTraverseProxy::find_not(const Series& mask) const;
```

## What It Does

`find_not(mask)` returns the coordinates of every position where `mask` is
**false**. It is [`find_all`](find_all.md) with `mask.logical_not()`.

It is the single-mask member of the boolean-combinator family alongside
[`find_and`](find_and.md) and [`find_or`](find_or.md). For a predicate that is a
plain equality, [`find_neq`](find_neq.md) is the more direct spelling; use
`find_not` to invert a mask that came from arbitrary logic.

Shape rules and gradient behavior match [`find_and`](find_and.md).

## Parameters

**mask** (`const Series&`)

A condition shaped like the traversed `Series`. Matched positions are the ones
where it is **not** true.

## Returns

[`TraverseMatch`](match.md)
    Row-major coordinates and matched values of the false positions.

## Example

```cpp
auto grid = pluss::make_series({
    {10.0, 4.0, 7.0},
    {2.0, 9.0, 6.0},
    {8.0, 1.0, 5.0},
});

auto below_five = grid.traverse.find_not(grid.ge(5.0));

std::cout << "count=" << below_five.count() << "\n";
std::cout << below_five.coords();
```

### Output

```text
count=3
pluss::Series(shape=[3,2], strides=[2,1], dtype=int64, platform=cpu, numel=6, contiguous=true)
data=[[0, 1],
  [1, 0],
  [2, 1]]
```

`find_not(grid.ge(5.0))` is the same set as `find_le` would give for `< 5` —
the values `4`, `2`, `1`.

## See Also

- [`find_and`](find_and.md): the primary page for the boolean-combinator family.
- [`find_or`](find_or.md): either mask true.
- [`find_neq`](find_neq.md): the direct spelling when the predicate is equality.
- [`TraverseMatch`](match.md): the result object.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="find_neq.html">
    <span class="label">Previous</span>
    <span class="title">find_neq</span>
  </a>
  <a class="next-link" href="find_or.html">
    <span class="label">Next</span>
    <span class="title">find_or</span>
  </a>
</div>
