<!-- pluss-hand-authored -->

# DataFrameGrouped::agg

[Back to DataFrameGroupBuilder API Reference](../../dataframe_group_accessor_api_reference)

## Signatures

```cpp
DataFrameGrouped::agg(pluss::msharp::RangeView::Of<DataFrame::GroupSpec> specs) const -> DataFrame
DataFrameGrouped::agg(std::initializer_list<DataFrame::GroupSpec> specs) const -> DataFrame
```

## What It Does

Collapses the grouped handle to **one row per group**, computing the requested
aggregations from `GroupSpec`.

Each `GroupSpec` provides:

- `output`: output feature name; may be empty for auto-naming
- `feature`: source feature to aggregate; may be empty only for `GroupOp::Count`
- `op`: grouped operation such as `Count`, `Ndistinct`, `Sum`, `Mean`, `Median`,
  `Min`, `Max`, `Std`, `Var`, or `Mode`

This is the most expressive grouped terminal. Use it when you want mixed operations,
custom output names, or explicit row-count columns beside other aggregates.

## Parameters

**specs** (`RangeView<GroupSpec>` / `initializer_list<GroupSpec>`)

One or more grouped aggregation specs.

## Returns

`DataFrame`
    One output row per group. Group key columns appear first, followed by one
    output column per `GroupSpec`.

## Examples

Example 1. Mixed operations in one grouped pass.

```cpp
using namespace pluss::table;

auto loans = DataFrame({
    {"region", Thread(pluss::make_note({"North", "North", "South", "South"}))},
    {"amount", Thread(pluss::make_series({100.0, 250.0, 80.0, 120.0}))},
    {"status", Thread(pluss::make_note({"good", "good", "bad", "good"}))},
});

auto out = loans.group({.dropnull = true})
                .by({"region"})
                .agg({
                    {"loans", "", DataFrame::GroupOp::Count},
                    {"amount_sum", "amount", DataFrame::GroupOp::Sum},
                    {"amount_mean", "amount", DataFrame::GroupOp::Mean},
                    {"status_mode", "status", DataFrame::GroupOp::Mode},
                });
```

That single call mixes row count, numeric reduction, and categorical mode under one
stable grouping plan.

Example 2. Let output names be inferred.

```cpp
auto out = loans.group()
                .by({"region"})
                .agg({
                    {"", "amount", DataFrame::GroupOp::Min},
                    {"", "amount", DataFrame::GroupOp::Max},
                });
```

When `output` is left empty, Pluss generates a deterministic name from the source
feature and the operation.

Example 3. Aggregate by case-label levels.

```cpp
auto indexed = loans.set_label({"region", "year"});

auto out = indexed.group()
                  .by(DataFrame::LabelGroup{.label = "region"})
                  .agg({
                      {"amount_sum", "amount", DataFrame::GroupOp::Sum},
                  });
```

The grouped result still comes back as a normal `DataFrame`; the only difference is
that the grouping key came from the case axis rather than an ordinary feature.

## Notes

- Non-count operations require a source `feature`.
- Output feature names must be unique after inference; duplicates are rejected.
- `agg(...)` is the right surface when you want row count explicitly beside other
  aggregates. For example:
  `{"n", "", GroupOp::Count}` and `{"avg_amount", "amount", GroupOp::Mean}`.
- Group keys always lead the result. Aggregated outputs follow them.
- Today only `GroupLayout::Flat` is supported.

## See Also

- [`DataFrameGrouped::transform`](transform.md): same grouped reductions, but
  broadcast back to row space instead of collapsing groups.
- [`DataFrameGrouped::count`](count.md): a convenience grouped terminal when explicit
  specs are unnecessary.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="by.html">
    <span class="label">Previous</span>
    <span class="title">by</span>
  </a>
  <a class="next-link" href="transform.html">
    <span class="label">Next</span>
    <span class="title">transform</span>
  </a>
</div>
