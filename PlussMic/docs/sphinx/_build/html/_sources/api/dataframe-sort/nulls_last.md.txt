<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 8baf039b7674 -->

# DataFrameSortBuilder::nulls_last

[Back to DataFrameSortBuilder API Reference](../../dataframe_sort_accessor_api_reference)

## Signature

```cpp
DataFrameSortBuilder DataFrameSortBuilder::nulls_last() const
```

## What It Does

Moves missing values to the **end** of the result for the active key — the
most recently added key via [`by(...)`](by.md)/[`then(...)`](then.md). This
is also the builder's own default (`nulls_last = true` on
[`DataFrame::sort(...)`](../dataframe/sort.md) unless overridden).

```{important} nulls_last() mutates only the active key
Same scoping rule as [`asc()`](asc.md)/[`desc()`](desc.md) — this changes
only the most recently added key's null placement, not every key in the
plan.
```

## Parameters

`None`

## Returns

`DataFrameSortBuilder`
    A new builder with the active key's null placement set to end.

## Examples

Example 1. Explicit `nulls_last()` reproduces the builder's default
placement — a missing `"sales"` value sorts after every real value.

```cpp
using namespace pluss::table;

auto df = DataFrame({
    {"sales", Slate(pluss::make_series({120.0, 100.0, 90.0}),
                     pluss::make_series(std::vector<bool>{true, false, true}))},
});

auto out = df.sort().by("sales").nulls_last().yield();
std::cout << out;
```

### Output

```text
============
     | sales
------------
2    | 90   
0    | 120  
1    | NULL 
------------
[3 cs x 1 feat]
```

## Notes

- See the important callout above.
- Requires an active key to exist — call `.by(...)`/`.then(...)` first.
- Mainly useful to reverse a prior `.nulls_first()` on the active key, or
  when a builder-level `nulls_last = false` default needs overriding back
  for this specific key.

## See Also

- [`DataFrameSortBuilder::nulls_first`](nulls_first.md): the opposite
  placement.
- [`DataFrameSortBuilder::by`](by.md): supplies the key this method mutates.
- [`DataFrame::sort`](../dataframe/sort.md): the entry point this builder is
  reached from, whose `nulls_last` parameter sets every key's default.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="nulls_first.html">
    <span class="label">Previous</span>
    <span class="title">nulls_first</span>
  </a>
  <a class="next-link" href="order.html">
    <span class="label">Next</span>
    <span class="title">order</span>
  </a>
</div>
