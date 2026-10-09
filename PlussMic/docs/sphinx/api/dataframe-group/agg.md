<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 0d4fbeada958 -->

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

## When To Use It

Use `agg(...)` when the grouped output should be one row per group, but the value-side
 operations are not uniform.

Typical reasons:

- one column needs `sum`, another needs `mean`
- you want row count beside other aggregates
- you want explicit output names
- you want grouped mode beside numeric grouped reductions in one result

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
    {"region", Slate(pluss::make_note({"North", "North", "South", "South"}))},
    {"amount", Slate(pluss::make_series({100.0, 250.0, 80.0, 120.0}))},
    {"status", Slate(pluss::make_note({"good", "good", "bad", "good"}))},
});

auto out = loans.group({.dropnull = true})
                .by({"region"})
                .agg({
                    {"loans", "", DataFrame::GroupOp::Count},
                    {"amount_sum", "amount", DataFrame::GroupOp::Sum},
                    {"amount_mean", "amount", DataFrame::GroupOp::Mean},
                    {"status_mode", "status", DataFrame::GroupOp::Mode},
                });
std::cout << out;
```

### Output

```text
==========================================================
     | region  loans  amount_sum  amount_mean  status_mode
----------------------------------------------------------
0    | North   2      350         175          good       
1    | South   2      200         100          bad        
----------------------------------------------------------
[2 cs x 5 feat]
```

That single call mixes row count, numeric reduction, and categorical mode under one
stable grouping plan. `South`'s `status` values are `"bad"` and `"good"`, tied at one
occurrence each; `status_mode` picks `"bad"` because it was seen first (row `2`), not
because it sorts first alphabetically — the same first-occurrence tie-break
[`Slate::freq()`](../slate/freq.md) uses.

Example 2. Let output names be inferred.

```cpp
auto out = loans.group()
                .by({"region"})
                .agg({
                    {"", "amount", DataFrame::GroupOp::Min},
                    {"", "amount", DataFrame::GroupOp::Max},
                });
std::cout << out;
```

### Output

```text
=====================================
     | region  amount_min  amount_max
-------------------------------------
0    | North   100         250       
1    | South   80          120       
-------------------------------------
[2 cs x 3 feat]
```

When `output` is left empty, Pluss generates a deterministic name from the source
feature and the operation: `"amount"` + `GroupOp::Min` becomes `"amount_min"`, and
likewise for `"amount_max"`.

Example 3. Aggregate by case-label levels.

```cpp
auto indexed = DataFrame({
    {"region", Slate(pluss::make_note({"North", "North", "South", "South"}))},
    {"year", Slate(pluss::make_series({2023.0, 2024.0, 2023.0, 2024.0}))},
    {"amount", Slate(pluss::make_series({100.0, 250.0, 80.0, 120.0}))},
}).label().set({"region", "year"});

auto out = indexed.group()
                  .by(DataFrame::LabelGroup{.names = {"region", "year"}})
                  .agg({
                      {"amount_sum", "amount", DataFrame::GroupOp::Sum},
                  });
std::cout << out;
```

### Output

```text
===============================
     | region  year  amount_sum
-------------------------------
0    | North   2023  100       
1    | North   2024  250       
2    | South   2023  80        
3    | South   2024  120       
-------------------------------
[4 cs x 3 feat]
```

The grouped result still comes back as a normal `DataFrame`; the only difference is
that the grouping key came from the case axis rather than an ordinary feature. Here
every `(region, year)` pair is already unique, so each "group" has exactly one row
and `amount_sum` just echoes the original `amount` value.

## Failure Modes

- empty spec list -> validation error
- non-count spec with empty `feature` -> validation error
- duplicate output names after inference -> validation error
- unsupported op/value kind combination -> validation error
- unsupported grouped layout -> validation error

## Notes

- Non-count operations require a source `feature`.
- Output feature names must be unique after inference; duplicates are rejected.
- `agg(...)` is the right surface when you want row count explicitly beside other
  aggregates. For example:
  `{"n", "", GroupOp::Count}` and `{"avg_amount", "amount", GroupOp::Mean}`.
- Group keys always lead the result. Aggregated outputs follow them, in the order
  their `GroupSpec` entries were given.
- `GroupOp::Mode` (and every other tie-sensitive grouped op) breaks ties by
  first-occurrence order within the group, not alphabetically or numerically.
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
