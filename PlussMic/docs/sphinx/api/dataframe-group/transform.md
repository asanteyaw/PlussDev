<!-- pluss-hand-authored -->

# DataFrameGrouped::transform

[Back to DataFrameGroupBuilder API Reference](../../dataframe_group_accessor_api_reference)

## Signatures

```cpp
DataFrameGrouped::transform(pluss::msharp::RangeView::Of<DataFrame::GroupSpec> specs) const -> DataFrame
DataFrameGrouped::transform(std::initializer_list<DataFrame::GroupSpec> specs) const -> DataFrame
```

## What It Does

Computes grouped reductions, then **broadcasts the result back to every row in the
original group** instead of collapsing to one row per group.

This is the grouped “decorate each row with a group statistic” surface. Common uses:

- add region mean amount to every row in that region
- add per-customer transaction count to each transaction
- add within-group max/min/spread signals before later row-wise logic

## Parameters

**specs** (`RangeView<GroupSpec>` / `initializer_list<GroupSpec>`)

One or more grouped reduction specs, using the same `GroupSpec` contract as
[`agg()`](agg.md).

## Returns

`DataFrame`
    A frame with the same case count and case labels as the source, containing only
    the broadcasted grouped output features.

## Examples

Example 1. Attach per-city mean amount to every original row.

```cpp
using namespace pluss::table;

auto sales = DataFrame({
    {"city", Thread(pluss::make_note({"Accra", "Accra", "Tema", "Tema"}))},
    {"amount", Thread(pluss::make_series({100.0, 80.0, 50.0, 40.0}))},
});

auto city_mean = sales.group()
                      .by({"city"})
                      .transform({
                          {"amount_mean", "amount", DataFrame::GroupOp::Mean},
                      });
```

Rows 0 and 1 receive the same broadcasted `amount_mean`; rows 2 and 3 receive the
other one.

Example 2. Broadcast several grouped signals at once.

```cpp
auto stats = sales.group()
                  .by({"city"})
                  .transform({
                      {"amount_mean", "amount", DataFrame::GroupOp::Mean},
                      {"amount_max", "amount", DataFrame::GroupOp::Max},
                      {"n", "", DataFrame::GroupOp::Count},
                  });
```

That produces a row-aligned grouped feature block you can join back mentally with the
source row space without doing an explicit merge.

## Notes

- `transform(...)` returns only the broadcasted grouped features, not the key columns
  and not the original source columns.
- Case order is preserved exactly. This is what makes it suitable for downstream
  row-wise feature engineering.
- Like `agg(...)`, non-count operations require a source `feature`.

## See Also

- [`DataFrameGrouped::agg`](agg.md): one row per group instead of row-space
  broadcast.
- [`DataFrame::join`](../dataframe/join.md): if you want to combine the broadcasted
  result explicitly with another frame afterwards.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="agg.html">
    <span class="label">Previous</span>
    <span class="title">agg</span>
  </a>
  <a class="next-link" href="filter.html">
    <span class="label">Next</span>
    <span class="title">filter</span>
  </a>
</div>
