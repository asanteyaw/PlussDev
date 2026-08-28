<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: e1d3ba0fdcbc -->

# DataFrameSortBuilder::within

[Back to DataFrameSortBuilder API Reference](../../dataframe_sort_accessor_api_reference)

## Signatures

```cpp
DataFrameSortBuilder DataFrameSortBuilder::within(std::string_view feature) const
DataFrameSortBuilder DataFrameSortBuilder::within(std::initializer_list<std::string_view> features) const
DataFrameSortBuilder DataFrameSortBuilder::within(pluss::msharp::RangeView::Of<std::string_view> features) const
```

## What It Does

Adds grouping key(s) that partition the frame **before** the
[`by(...)`](by.md) value keys are applied — rows are only ever reordered
*within* a group, never across group boundaries. Groups themselves keep
their first-seen relative order in the source frame; only the row order
inside each group changes.

```{important} within() only partitions — it doesn't sort by itself
`within(...)` alone defines no value ordering; pair it with
[`by(...)`](by.md) for the actual sort key(s) applied inside each group.
```

## Parameters

**feature** / **features**
    One or more feature names to group by, in priority order. Any feature
    kind works; groups are compared by value.

## Returns

`DataFrameSortBuilder`
    A new builder with the group partition set. The value-key plan (set via
    `by()`/`then()`) is unaffected and still required for an actual sort.

## Examples

Example 1. Partition by `"city"`, then sort each city's rows by `"sales"`
descending — city groups stay in first-seen order (`"Tema"` appears first at
row `0`), but rows *within* each city are reordered.

```cpp
using namespace pluss::table;

auto df = DataFrame({
    {"city",  Thread(pluss::make_note({"Tema", "Accra", "Accra", "Tema"}))},
    {"sales", Thread(pluss::make_series({120.0, 100.0, 90.0, 200.0}))},
});

auto out = df.sort().within({"city"}).by("sales").desc().yield();
std::cout << out;
```

### Output

```text
===================
     | city   sales
-------------------
3    | Tema   200  
0    | Tema   120  
1    | Accra  100  
2    | Accra  90   
-------------------
[4 cs x 2 feat]
```

`"Tema"` is first because row `0` (the first `"Tema"` row) appears before
row `1` (the first `"Accra"` row) in the source frame — group order is
preserved from the original frame, independent of the `"sales"` values.
Within `"Tema"`, `200` (row `3`) sorts before `120` (row `0`); within
`"Accra"`, `100` sorts before `90` — both descending, per `.desc()`.

## Notes

- See the important callout above.
- Group order is taken from the source frame's first occurrence of each
  distinct group value, not sorted alphabetically or by any other criterion.
- Can be combined with [`then(...)`](then.md) for multi-key tie-breaking
  inside each group, exactly as without `within(...)`.
- `within(...)` validates its feature names immediately because the partition itself
  must be resolved before any later terminal can run.

## See Also

- [`DataFrameSortBuilder::by`](by.md): supplies the value key(s) applied
  inside each group.
- [`DataFrame::sort`](../dataframe/sort.md): the entry point this builder is
  reached from.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="then.html">
    <span class="label">Previous</span>
    <span class="title">then</span>
  </a>
  <a class="next-link" href="yield.html">
    <span class="label">Next</span>
    <span class="title">yield</span>
  </a>
</div>
