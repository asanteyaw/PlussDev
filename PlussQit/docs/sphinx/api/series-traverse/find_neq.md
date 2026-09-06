<!-- pluss-hand-authored -->

# SeriesTraverseAccessor::find_neq

[Back to SeriesTraverseAccessor API Reference](../../series_traverse_accessor_api_reference.md)

## Signatures

```cpp
TraverseMatch SeriesTraverseAccessor::find_neq(double rhs) const;
TraverseMatch SeriesTraverseAccessor::find_neq(const Series& rhs) const;

TraverseMatch SeriesTraverseProxy::find_neq(double rhs) const;
TraverseMatch SeriesTraverseProxy::find_neq(const Series& rhs) const;
```

## What It Does

`find_neq(rhs)` is the negation of [`find_eq(rhs)`](find_eq.md): it returns the
coordinates of every position whose value is **not** equal to `rhs`. It is
[`find_all`](find_all.md) with the mask `source.ne(rhs)`.

Both the scalar and the peer-`Series` forms behave exactly as in
[`find_eq`](find_eq.md), just inverted. See that page for the shape rule,
broadcasting, gradient behavior, and the exact-equality caveat.

## Parameters

**rhs** (`double` or `const Series&`)

The value that matched positions must **differ** from. A `Series` `rhs` is
broadcast against the traversed shape.

## Returns

[`TraverseMatch`](match.md)
    Coordinates and values of the non-equal positions, row-major.

## Example

```cpp
auto board = pluss::make_series({
    {1.0, 0.0, 1.0},
    {0.0, 1.0, 0.0},
});

auto not_one = board.traverse.find_neq(1.0);

std::cout << "count=" << not_one.count() << "\n";
std::cout << not_one.coords();
```

### Output

```text
count=3
pluss::Series(shape=[3,2], strides=[2,1], dtype=int64, platform=cpu, numel=6, contiguous=true)
data=[[0, 1],
  [1, 0],
  [1, 2]]
```

Every position that is not `1` — the complement of
[`find_eq(1.0)`](find_eq.md) on the same board.

## See Also

- [`find_eq`](find_eq.md): the primary page; `find_neq` is its negation.
- [`find_not`](find_not.md): negate an arbitrary mask.
- [`TraverseMatch`](match.md): the result object.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="find_le.html">
    <span class="label">Previous</span>
    <span class="title">find_le</span>
  </a>
  <a class="next-link" href="find_not.html">
    <span class="label">Next</span>
    <span class="title">find_not</span>
  </a>
</div>
