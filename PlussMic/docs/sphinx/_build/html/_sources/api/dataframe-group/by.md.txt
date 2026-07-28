<!-- pluss-hand-authored -->

# DataFrameGroupBuilder::by

[Back to DataFrameGroupBuilder API Reference](../../dataframe_group_accessor_api_reference)

## Signatures

```cpp
DataFrameGroupBuilder::by(std::initializer_list<std::string_view> keys) const -> DataFrameGrouped
DataFrameGroupBuilder::by(pluss::msharp::RangeView::Of<std::string_view> keys) const -> DataFrameGrouped
DataFrameGroupBuilder::by(const DataFrame::LabelGroup& keys) const -> DataFrameGrouped
```

## What It Does

Chooses what defines a group, returning a grouped handle that can then execute
`agg(...)`, `transform(...)`, `filter(...)`, or a convenience terminal such as
`mean()` or `count()`.

There are two distinct grouping domains:

- **Feature-key grouping**: ordinary columns such as `"city"`, `"product"`, or
  `"status"`
- **Case-label grouping**: levels on the case axis, selected through
  `LabelGroup`

`by(...)` still does not execute the grouped work. It only fixes the grouping keys
and returns `DataFrameGrouped`.

## Parameters

**keys** (`initializer_list<string_view>` / `RangeView<...>`)

One or more ordinary feature names already present on the frame.

**keys** (`const DataFrame::LabelGroup&`)

Case-label grouping instructions.

- `label`: one named case-label level
- `names`: several case-label levels

Use exactly one of them. `LabelGroup{.label = "region"}` is the single-level form;
`LabelGroup{.names = {"region", "year"}}` is the multi-level form.

## Returns

`DataFrameGrouped`
    A grouped handle that carries the original frame, the resolved group keys,
    and the `GroupOptions` captured by `group(...)`.

## Examples

Example 1. Group by one ordinary feature.

```cpp
using namespace pluss::table;

auto sales = DataFrame({
    {"city", Thread(pluss::make_note({"Accra", "Accra", "Tema", "Tema"}))},
    {"amount", Thread(pluss::make_series({100.0, 80.0, 50.0, 40.0}))},
    {"units", Thread(pluss::make_series({2, 1, 3, 1}))},
});

auto grouped = sales.group({.dropnull = true}).by({"city"});
auto out = grouped.sum();
```

`by({"city"})` says “rows with the same `city` belong to the same group”. The actual
work still happens at `.sum()`.

Example 2. Group by several ordinary features at once.

```cpp
auto out = sales.group({.sort = true})
               .by({"city", "product"})
               .agg({
                   {"orders", "", DataFrame::GroupOp::Count},
                   {"revenue", "amount", DataFrame::GroupOp::Sum},
               });
```

Now the group key is the full tuple `(city, product)`, not either feature on its own.

Example 3. Group by case-label levels instead of ordinary columns.

```cpp
auto indexed = sales.set_label({"region", "year"});

auto out = indexed.group()
                  .by(DataFrame::LabelGroup{.names = {"region", "year"}})
                  .agg({
                      {"revenue", "amount", DataFrame::GroupOp::Sum},
                  });
```

This is useful when the grouping identity already lives on the case axis and you do
not want to materialize it as ordinary columns first.

## Notes

- The initializer-list and `RangeView` overloads are semantically the same; the
  `RangeView` form is for dynamic key lists.
- `LabelGroup` is stricter than ordinary key lists: it requires the source frame to
  already have case labels, and it rejects “both `label` and `names`” at once.
- Group order is not chosen by `by(...)` alone. It also depends on the `sort` option
  captured earlier by `group(...)`.

## See Also

- [`DataFrameGrouped::agg`](agg.md): collapse to one row per group.
- [`DataFrameGrouped::transform`](transform.md): broadcast grouped values back to
  original row order.
- [`DataFrameGrouped::filter`](filter.md): keep or drop whole groups from the source.

<div class="pluss-inline-prev-next">
  <a class="next-link" href="agg.html">
    <span class="label">Next</span>
    <span class="title">agg</span>
  </a>
</div>
