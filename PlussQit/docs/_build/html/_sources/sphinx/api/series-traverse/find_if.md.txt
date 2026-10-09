<!-- pluss-hand-authored -->

# Series::TraverseAccessor::find_if

[Back to Series::TraverseAccessor API Reference](../../series_traverse_accessor_api_reference.md)

## Signatures

```cpp
TraverseMatch Series::TraverseAccessor::find_if(const Series& mask) const;
TraverseMatch SeriesTraverseProxy::find_if(const Series& mask) const;
```

## What It Does

`find_if(mask)` is an exact alias of [`find_all(mask)`](find_all.md) — same
parameters, same shape rule, same [`TraverseMatch`](match.md) result. Both
names exist so call sites can read naturally: `find_all` when you think of it as
"collect every hit," `find_if` when you think of it as "keep positions where
this predicate holds."

See [`find_all`](find_all.md) for the full description, examples, and gotchas.

## Example

```cpp
auto grid = pluss::make_series({
    {10.0, 4.0, 7.0},
    {2.0, 9.0, 6.0},
    {8.0, 1.0, 5.0},
});

auto a = grid.traverse.find_all(grid.gt(7.0));
auto b = grid.traverse.find_if(grid.gt(7.0));

std::cout << "find_all count=" << a.count()
          << "  find_if count=" << b.count();
```

### Output

```text
find_all count=3  find_if count=3
```

## See Also

- [`find_all`](find_all.md): the primary page for this operation.
- [`TraverseMatch`](match.md): the result object.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="find_ge.html">
    <span class="label">Previous</span>
    <span class="title">find_ge</span>
  </a>
  <a class="next-link" href="find_le.html">
    <span class="label">Next</span>
    <span class="title">find_le</span>
  </a>
</div>
