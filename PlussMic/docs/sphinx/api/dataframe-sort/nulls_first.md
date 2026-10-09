<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 4efb744eeb08 -->

# DataFrameSortBuilder::nulls_first

[Back to DataFrameSortBuilder API Reference](../../dataframe_sort_accessor_api_reference)

## Signature

```cpp
DataFrameSortBuilder DataFrameSortBuilder::nulls_first() const
```

## What It Does

Moves missing values to the **front** of the result for the active key —
the most recently added key via [`by(...)`](by.md)/[`then(...)`](then.md).

```{important} nulls_first() mutates only the active key
Same scoping rule as [`asc()`](asc.md)/[`desc()`](desc.md) — this changes
only the most recently added key's null placement, not every key in the
plan.
```

## Parameters

`None`

## Returns

`DataFrameSortBuilder`
    A new builder with the active key's null placement set to front.

## Examples

Example 1. A missing `"sales"` value sorts first instead of the default
last.

```cpp
using namespace pluss::table;

auto df = DataFrame({
    {"sales", Slate(pluss::make_series({120.0, 100.0, 90.0}),
                     pluss::make_series(std::vector<bool>{true, false, true}))},
});

auto default_out = df.sort().by("sales").yield();
std::cout << "default (nulls_last):\n" << default_out;

auto out = df.sort().by("sales").nulls_first().yield();
std::cout << "nulls_first:\n" << out;
```

### Output

```text
default (nulls_last):
============
     | sales
------------
2    | 90   
0    | 120  
1    | NULL 
------------
[3 cs x 1 feat]
nulls_first:
============
     | sales
------------
1    | NULL 
2    | 90   
0    | 120  
------------
[3 cs x 1 feat]
```

The missing row (`1`) moves from last to first; the two real values (`90`,
`120`) keep their relative ascending order in both cases.

## Notes

- See the important callout above.
- Requires an active key to exist — call `.by(...)`/`.then(...)` first.
- Overrides whatever `nulls_last` default [`DataFrame::sort(ascending,
  nulls_last)`](../dataframe/sort.md) set, for this key only.

## See Also

- [`DataFrameSortBuilder::nulls_last`](nulls_last.md): the opposite
  placement, and the builder's own default.
- [`DataFrameSortBuilder::by`](by.md): supplies the key this method mutates.
- [`DataFrame::sort`](../dataframe/sort.md): the entry point this builder is
  reached from, whose `nulls_last` parameter sets every key's default.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="head.html">
    <span class="label">Previous</span>
    <span class="title">head</span>
  </a>
  <a class="next-link" href="nulls_last.html">
    <span class="label">Next</span>
    <span class="title">nulls_last</span>
  </a>
</div>
