<!-- pluss-hand-authored -->

# Series::TraverseAccessor::find_or

[Back to Series::TraverseAccessor API Reference](../../series_traverse_accessor_api_reference.md)

## Signatures

```cpp
TraverseMatch Series::TraverseAccessor::find_or(const Series& lhs_mask,
                                              const Series& rhs_mask) const;
TraverseMatch SeriesTraverseProxy::find_or(const Series& lhs_mask,
                                           const Series& rhs_mask) const;
```

## What It Does

`find_or(lhs_mask, rhs_mask)` returns the coordinates of every position where
**either** mask is true. It is [`find_all`](find_all.md) with the mask
`lhs_mask.logical_or(rhs_mask)`.

Shape rules, the "non-zero means true" convention, and gradient behavior are
identical to [`find_and`](find_and.md); see that page for the full description.

## Parameters

**lhs_mask**, **rhs_mask** (`const Series&`)

Two conditions shaped like the traversed `Series`. A position matches if it is
true in at least one of them.

## Returns

[`TraverseMatch`](match.md)
    Row-major coordinates and matched values of the positions satisfying either
    mask.

## Example

```cpp
auto grid = pluss::make_series({
    {10.0, 4.0, 7.0},
    {2.0, 9.0, 6.0},
    {8.0, 1.0, 5.0},
});

auto extremes = grid.traverse.find_or(grid.lt(2.0), grid.gt(9.0));

std::cout << "count=" << extremes.count() << "\n";
std::cout << extremes.coords();
std::cout << extremes.values();
```

### Output

```text
count=2
pluss::Series(shape=[2,2], strides=[2,1], dtype=int64, platform=cpu, numel=4, contiguous=true)
data=[[0, 0],
  [2, 1]]
pluss::Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[10, 1]
```

`10` clears the upper condition and `1` clears the lower one; nothing else
matches either.

## See Also

- [`find_and`](find_and.md): the primary page for the boolean-combinator family.
- [`find_not`](find_not.md): a mask false.
- [`TraverseMatch`](match.md): the result object.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="find_not.html">
    <span class="label">Previous</span>
    <span class="title">find_not</span>
  </a>
  <a class="next-link" href="match.html">
    <span class="label">Next</span>
    <span class="title">match</span>
  </a>
</div>
