<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 4b0f8816b9a3 -->

# DataFrame::pivot_table

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signature

```cpp
DataFrame::pivot_table(std::string_view case_feature,
                        std::string_view feature_feature,
                        std::string_view value_feature,
                        GroupOp op = GroupOp::Mean,
                        bool dropnull_keys = true,
                        bool render = false) const -> DataFrameView
```

## What It Does

Reshapes long-format data into a wide summary table: one distinct value of
`case_feature` per output row, one distinct value of `feature_feature` per output
column, and each cell holding `op` applied to every `value_feature` observation that
shares that row's and column's key values.

`pivot_table()` is closely related to [`group()`](group.md) — both aggregate
`value_feature` per key combination — but `group()` keeps every key as its own row
feature (a "long" result), while `pivot_table()` spreads one key out across columns (a
"wide" result). Reach for `pivot_table()` specifically when you want that
row/column cross-tabulation shape, for example to visualize or export a summary as a
matrix.

## Parameters

**case_feature** (`std::string_view`)

Feature whose distinct values become the output rows.

**feature_feature** (`std::string_view`)

Feature whose distinct values become the new output columns.

**value_feature** (`std::string_view`)

Feature whose values are aggregated into each pivoted cell.

**op** (`GroupOp`, default `GroupOp::Mean`)

The aggregation applied within each `(case_feature, feature_feature)` cell — the same
operation set as [`group()`](group.md)'s `GroupSpec::op`:

- `GroupOp::Count`: the number of observations in the cell, including any that are
  themselves null. Use this to answer "how many orders fell in this combination" rather
  than to summarize `value_feature`'s magnitude at all.
- `GroupOp::Ndistinct`: the number of *distinct* `value_feature` values in the cell.
  Useful when `value_feature` is closer to a category or an ID than a quantity — for
  example, counting how many distinct `product` values a customer bought per city rather
  than summing an amount.
- `GroupOp::Sum`: the total of every observed value in the cell, nulls skipped. This is
  the natural choice when `value_feature` is a quantity that should accumulate, such as
  `units` or `revenue`.
- `GroupOp::Mean`: the arithmetic average of the observed values in the cell, nulls
  skipped. The default — most appropriate when `value_feature` measures a rate, price, or
  score rather than something you want summed.
- `GroupOp::Min` / `GroupOp::Max`: the smallest / largest observed value in the cell.
  Useful for questions like "the latest order date" or "the lowest price seen" per
  combination.
- `GroupOp::Std` / `GroupOp::Var`: the sample standard deviation / variance of the
  observed values in the cell (divides by `n - 1`, not `n`). A cell with only one
  observation reports `NULL` here, since sample variance is undefined for `n = 1`.

**dropnull_keys** (`bool`, default `true`)

When `true`, cases with a null `case_feature` or `feature_feature` value are excluded
before pivoting. When `false`, a null key value forms its own row or column instead of
being dropped.

**render** (`bool`, default `false`)

Display toggle for the returned `DataFrameView`; `false` for explicit printing, `true` to
allow auto-render in a temporary-value context.

## Returns

`DataFrameView`
    A new frame with one row per distinct `case_feature` value, `case_feature` itself as
    the first column, and one additional column per distinct `feature_feature` value.
    Cells for `(case_feature, feature_feature)` combinations that never occur together in
    the source data are `NULL` rather than `0` — the aggregation genuinely has no
    observations to summarize for that cell, which is different from summarizing an
    empty group as zero.

## Examples

Example 1. Pivot total units sold by city (rows) and product (columns).

```cpp
using namespace pluss::table;

auto orders = DataFrame({
    {"order_id", Thread(pluss::make_series({101, 102, 103, 104, 105, 106}))},
    {"city",     Thread(pluss::make_note({"Accra", "Accra", "Tema", "Tema", "Kumasi", "Kumasi"}))},
    {"product",  Thread(pluss::make_note({"Laptop", "Headphones", "Laptop", "Monitor", "Headphones", "Monitor"}))},
    {"units",    Thread(pluss::make_series({2, 5, 1, 3, 4, 2}))},
});

auto by_city_product = orders.pivot_table("city", "product", "units", DataFrame::GroupOp::Sum);
std::cout << by_city_product;
```

### Output

```text
==========================================
     | city    Laptop  Headphones  Monitor
------------------------------------------
0    | Accra   2       5           NULL   
1    | Tema    1       NULL        3      
2    | Kumasi  NULL    4           2      
------------------------------------------
[3 cs x 4 feat]
```

Every `(city, product)` pair that never occurs in `orders` — Accra never sold a
`Monitor`, Tema never sold `Headphones`, Kumasi never sold a `Laptop` — shows up as
`NULL`, not `0`. That distinction matters downstream: summing a row of `NULL`s and real
numbers with a null-skipping reduction gives the true total, whereas a stray `0` would
silently (and wrongly) count as "zero units sold" for a combination that was simply never
observed.

Example 2. Use the default aggregation (`Mean`) instead of naming `Sum` explicitly.

```cpp
auto avg_units = orders.pivot_table("city", "product", "units");  // op defaults to GroupOp::Mean
std::cout << avg_units;
```

### Output

```text
==========================================
     | city    Laptop  Headphones  Monitor
------------------------------------------
0    | Accra   2       5           NULL   
1    | Tema    1       NULL        3      
2    | Kumasi  NULL    4           2      
------------------------------------------
[3 cs x 4 feat]
```

Since every `(city, product)` combination in this particular dataset happens to occur
exactly once, `Mean` and `Sum` produce numerically identical cells here — the difference
only shows up once a combination has more than one matching order. `NULL` cells are still
`NULL` regardless of `op`: there is nothing to average or sum when zero observations
share that key pair.

Example 3. Keep a null key value as its own row instead of dropping it.

```cpp
auto orders_with_gap = DataFrame({
    {"order_id", Thread(pluss::make_series({101, 102, 103, 104, 105, 106, 107}))},
    {"city",     Thread(pluss::make_note({"Accra", "Accra", "Tema", "Tema", "Kumasi", "Kumasi", ""}), pluss::make_series({true, true, true, true, true, true, false}))},
    {"product",  Thread(pluss::make_note({"Laptop", "Headphones", "Laptop", "Monitor", "Headphones", "Monitor", "Laptop"}))},
    {"units",    Thread(pluss::make_series({2, 5, 1, 3, 4, 2, 9}))},
});

auto with_null_row = orders_with_gap.pivot_table(
    "city", "product", "units", DataFrame::GroupOp::Sum, /*dropnull_keys=*/false);
std::cout << with_null_row;
```

### Output

```text
==========================================
     | city    Laptop  Headphones  Monitor
------------------------------------------
0    | Accra   2       5           NULL   
1    | Tema    1       NULL        3      
2    | Kumasi  NULL    4           2      
3    | NULL    9       NULL        NULL   
------------------------------------------
[4 cs x 4 feat]
```

Order `107` has no recorded `city` (the case's `city` cell is null via the validity mask
constructor). With `dropnull_keys = false`, that order is not discarded — it forms its
own row keyed on a null `city`, keeping its `Laptop` total of `9` visible. With the
default `dropnull_keys = true`, that same order would simply vanish from the pivoted
result with no trace, which is the right behavior when a missing key represents bad data
you want to exclude, but the wrong behavior when a missing key is itself a meaningful
group (an "unassigned" bucket, for example).

## Notes

- `pivot_table()` never invents a value for an unobserved `(case_feature,
  feature_feature)` pair; missing cells are always `NULL`. Follow with
  [`fillnull(0.0)`](fillnull.md) explicitly if a zero is the correct interpretation for
  your data.
- If more than one row of `orders` shares the exact same `(case_feature,
  feature_feature)` key pair, `op` aggregates across all of them into the single
  resulting cell — this is exactly the scenario `pivot_table()` exists for, as opposed to
  a plain reshape that would need every key pair to be unique already.
- For a result that keeps every key as its own row feature instead of spreading one key
  into columns, use [`group()`](group.md) directly.

## See Also

- [`DataFrame::group`](group.md): the long-format sibling of this reshape.
- [`DataFrame::pivot`](pivot.md): reshape without aggregation, for data that is already
  one row per `(case_feature, feature_feature)` pair.
- [`DataFrame::fillnull`](fillnull.md): replace the `NULL` cells this method leaves for
  unobserved combinations.
- [`DataFrame::stack`](stack.md): fold pivoted columns back into a long format.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="pivot.html">
    <span class="label">Previous</span>
    <span class="title">pivot</span>
  </a>
  <a class="next-link" href="place.html">
    <span class="label">Next</span>
    <span class="title">place</span>
  </a>
</div>
