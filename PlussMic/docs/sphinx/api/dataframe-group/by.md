<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 5ab01632f4bb -->

# DataFrameGroupBuilder::by

[Back to DataFrameGroupBuilder API Reference](../../dataframe_group_accessor_api_reference)

## Signatures

```cpp
DataFrameGrouped DataFrameGroupBuilder::by(std::initializer_list<std::string_view> keys) const
DataFrameGrouped DataFrameGroupBuilder::by(pluss::msharp::RangeView::Of<std::string_view> keys) const
DataFrameGrouped DataFrameGroupBuilder::by(const DataFrame::LabelGroup& keys) const
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

When you supply more than one key, grouping uses the full tuple of key values.
So `by({"city", "product"})` means rows are grouped by `(city, product)`, not
by `city` alone with `product` acting as a later optional pass. Two rows land
in the same group only when every key in the tuple matches.

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
    {"city", Slate(pluss::make_note({"Accra", "Accra", "Tema", "Tema"}))},
    {"amount", Slate(pluss::make_series({100.0, 80.0, 50.0, 40.0}))},
    {"units", Slate(pluss::make_series({2.0, 1.0, 3.0, 1.0}))},
});

auto grouped = sales.group({.dropnull = true}).by({"city"});
auto out = grouped.sum();
std::cout << out;
```

### Output

```text
===================================
     | city   amount_sum  units_sum
-----------------------------------
0    | Accra  180         3        
1    | Tema   90          4        
-----------------------------------
[2 cs x 3 feat]
```

`by({"city"})` says "rows with the same `city` belong to the same group". The actual
work still happens at `.sum()`.

Example 2. Group by several ordinary features at once.

```cpp
auto orders = DataFrame({
    {"city", Slate(pluss::make_note({"Accra", "Accra", "Accra", "Tema"}))},
    {"product", Slate(pluss::make_note({"Tea", "Tea", "Coffee", "Tea"}))},
    {"amount", Slate(pluss::make_series({100.0, 80.0, 30.0, 50.0}))},
});

auto out = orders.group({.sort = true})
               .by({"city", "product"})
               .agg({
                   {"orders", "", DataFrame::GroupOp::Count},
                   {"revenue", "amount", DataFrame::GroupOp::Sum},
               });
std::cout << out;
```

### Output

```text
======================================
     | city   product  orders  revenue
--------------------------------------
0    | Accra  Coffee   1       30     
1    | Accra  Tea      2       180    
2    | Tema   Tea      1       50     
--------------------------------------
[3 cs x 4 feat]
```

Now the group key is the full tuple `(city, product)`, not either feature on its own.
`(Accra, Tea)` occurs on rows `0` and `1`, so `orders` is `2` and `revenue` sums both
amounts (`100 + 80 = 180`). With `sort = true`, the three groups print in ascending
key order — `(Accra, Coffee)` before `(Accra, Tea)` before `(Tema, Tea)` — rather than
the order each combination first appeared.

Example 3. Group by case-label levels instead of ordinary columns.

```cpp
auto indexed = DataFrame({
    {"region", Slate(pluss::make_note({"North", "North", "South", "South"}))},
    {"year", Slate(pluss::make_series({2023.0, 2024.0, 2023.0, 2024.0}))},
    {"amount", Slate(pluss::make_series({100.0, 250.0, 80.0, 120.0}))},
}).label().set({"region", "year"});

auto out = indexed.group()
                  .by(DataFrame::LabelGroup{.label = "region"})
                  .agg({
                      {"amount_sum", "amount", DataFrame::GroupOp::Sum},
                  });
std::cout << out;
```

### Output

```text
=========================
     | region  amount_sum
-------------------------
0    | North   350       
1    | South   200       
-------------------------
[2 cs x 2 feat]
```

`label().set({"region", "year"})` moves those two ordinary columns onto the case axis
as a two-level case label. `LabelGroup{.label = "region"}` then groups by only the
`region` level of that composite label, ignoring `year` entirely — so both `North`
rows (from 2023 and 2024) collapse into one group, and likewise for `South`. This is
useful when the grouping identity already lives on the case axis and you do not want
to materialize it as ordinary columns first.

## Failure Modes

- empty feature-key list -> validation error
- duplicate feature key names -> validation error
- unknown feature key names -> validation error at grouped execution time
- `LabelGroup` with neither `label` nor `names` -> validation error
- `LabelGroup` with both `label` and `names` -> validation error
- `LabelGroup` on a frame with no case labels -> validation error

## Notes

- The initializer-list and `RangeView` forms are semantically the same; the
  `RangeView` form is for dynamic key lists.
- `LabelGroup` is stricter than ordinary key lists: it requires the source frame to
  already have case labels, and it rejects "both `label` and `names`" at once.
- Group order is not chosen by `by(...)` alone. It also depends on the `sort` option
  captured earlier by `group(...)`: unsorted groups appear in first-occurrence order;
  sorted groups appear in ascending key order.
- For multi-key grouping, "ascending key order" means lexicographic tuple order:
  first key primary, second key tie-breaker, and so on.

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
