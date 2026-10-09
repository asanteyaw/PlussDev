<!-- pluss-hand-authored -->

# Series::TraverseAccessor::find_le

[Back to Series::TraverseAccessor API Reference](../../series_traverse_accessor_api_reference.md)

## Signatures

```cpp
TraverseMatch Series::TraverseAccessor::find_le(double rhs) const;
TraverseMatch Series::TraverseAccessor::find_le(const Series& rhs) const;

TraverseMatch SeriesTraverseProxy::find_le(double rhs) const;
TraverseMatch SeriesTraverseProxy::find_le(const Series& rhs) const;
```

## What It Does

`find_le(rhs)` is the mirror of [`find_ge(rhs)`](find_ge.md): it returns the
coordinates of every position whose value is less than or equal to `rhs`. It is
[`find_all`](find_all.md) with the mask `source.le(rhs)`.

The scalar and peer-`Series` forms, the inclusive comparison, the sub-view
coordinate rule, and the gradient behavior are all exactly as documented on
[`find_ge`](find_ge.md).

## Parameters

**rhs** (`double` or `const Series&`)

The upper bound. A `Series` `rhs` broadcasts against the traversed shape.

## Returns

[`TraverseMatch`](match.md)
    Row-major coordinates and matched values of the positions at or below the
    bound.

## Example

```cpp
auto grid = pluss::make_series({
    {10.0, 4.0, 7.0},
    {2.0, 9.0, 6.0},
    {8.0, 1.0, 5.0},
});

auto low = grid.traverse.find_le(4.0);

std::cout << "count=" << low.count() << "\n";
std::cout << low.coords();
std::cout << low.values();
```

### Output

```text
count=3
pluss::Series(shape=[3,2], strides=[2,1], dtype=int64, platform=cpu, numel=6, contiguous=true)
data=[[0, 1],
  [1, 0],
  [2, 1]]
pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[4, 2, 1]
```

## See Also

- [`find_ge`](find_ge.md): the primary page; `find_le` is the at-or-below mirror.
- [`find_btw`](find_btw.md): a two-sided band.
- [`TraverseMatch`](match.md): the result object.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="find_if.html">
    <span class="label">Previous</span>
    <span class="title">find_if</span>
  </a>
  <a class="next-link" href="find_neq.html">
    <span class="label">Next</span>
    <span class="title">find_neq</span>
  </a>
</div>
