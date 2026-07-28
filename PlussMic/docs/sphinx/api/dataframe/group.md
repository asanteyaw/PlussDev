<!-- pluss-hand-authored -->

# DataFrame::group

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signatures

```cpp
DataFrame::group() const -> DataFrameGroupBuilder
DataFrame::group(const GroupOptions& options) const -> DataFrameGroupBuilder
```

## What It Does

Enters the grouped-workflow builder for the current frame. `group()` itself does not
aggregate, broadcast, or filter yet. It only captures **group-level execution policy**
through `GroupOptions`, after which you choose:

- `.by({...})` for ordinary feature keys
- `.by(LabelGroup{...})` for case-label levels
- `.agg({...})` for explicit named per-group reductions
- `.transform({...})` for broadcasted grouped values back to original row order
- `.filter(keep)` for keeping or dropping whole groups
- grouped convenience terminals such as `.count()`, `.sum()`, `.mean()`, `.median()`,
  `.min()`, `.max()`, `.std()`, `.var()`, `.mode()`, and `.ndistinct()`

The mental model is:

1. `group(...)` chooses grouping policy.
2. `by(...)` chooses what defines a group.
3. A grouped terminal actually executes the work.

This keeps grouping declarative and stable. You do not pass keys, specs, null policy,
sort policy, and execution mode into one giant call; the builder separates those
concerns cleanly.

This page previews the builder's most common shape. Every grouped method
(`by`, `agg`, `transform`, `filter`, and the convenience terminals) has its own
dedicated, example-rich reference page — start at the
[DataFrameGroupBuilder API Reference](../../dataframe_group_accessor_api_reference)
and use its left sidebar to jump straight to any method.

## Parameters

**options** (`const GroupOptions&`, optional)

- `dropnull`: drop rows with null group keys before grouping
- `sort`: sort groups lexicographically by key instead of preserving first-seen order
- `layout`: grouped result layout; currently only `GroupLayout::Flat` is supported

The zero-argument overload is shorthand for `GroupOptions{}`.

## Returns

`DataFrameGroupBuilder`
    A lightweight builder bound to the current frame. Calling `by(...)` returns a
    grouped handle, and grouped terminals return a new `DataFrame`.

## Examples

Example 1. Group by one key and aggregate explicitly.

```cpp
using namespace pluss::table;

auto orders = DataFrame({
    {"city",   Thread(pluss::make_note({"Accra", "Accra", "Tema", "Tema"}))},
    {"units",  Thread(pluss::make_series({2, 5, 1, 3}))},
    {"price",  Thread(pluss::make_series({1200.0, 45.5, 1150.0, 320.0}))},
});

auto by_city = orders.group({.dropnull = true})
                   .by({"city"})
                   .agg({
                       {"units_total", "units", DataFrame::GroupOp::Sum},
                       {"avg_price", "price", DataFrame::GroupOp::Mean},
                   });
std::cout << by_city;
```

### Output

```text
====================================
     | city   units_total  avg_price
------------------------------------
0    | Accra  7            622.75
1    | Tema   4            735
------------------------------------
[2 cs x 3 feat]
```

Example 2. Use a grouped convenience terminal after selecting value columns.

```cpp
auto means = orders.features({"city", "units", "price"})
                 .group()
                 .by({"city"})
                 .mean();
std::cout << means;
```

### Output

```text
============================
     | city   units_mean  price_mean
------------------------------------
0    | Accra  3.5         622.75
1    | Tema   2           735
------------------------------------
[2 cs x 3 feat]
```

Example 3. Ask for row counts explicitly.

```cpp
auto rows = orders.features({"city"})
               .group()
               .by({"city"})
               .agg({
                   {"count", "", DataFrame::GroupOp::Count},
               });
std::cout << rows;
```

### Output

```text
================
     | city  count
----------------
0    | Accra 2
1    | Tema  2
----------------
[2 cs x 2 feat]
```

Example 4. Broadcast a grouped statistic back to the original row space.

```cpp
auto city_mean = orders.group()
                    .by({"city"})
                    .transform({
                        {"price_mean", "price", DataFrame::GroupOp::Mean},
                    });
std::cout << city_mean;
```

### Output

```text
================
     | price_mean
----------------
0    | 622.75
1    | 622.75
2    | 735
3    | 735
----------------
[4 cs x 1 feat]
```

Example 5. Keep whole groups from a grouped boolean mask.

```cpp
auto totals = orders.group()
                  .by({"city"})
                  .agg({
                      {"units_total", "units", DataFrame::GroupOp::Sum},
                  });
auto keep = totals.features("units_total").gt(4.0);
auto only_large = orders.group().by({"city"}).filter(keep);
std::cout << only_large;
```

### Output

```text
==========================
     | city   units  price
--------------------------
0    | Accra  2      1200
1    | Accra  5      45.5
--------------------------
[2 cs x 3 feat]
```

## Notes

- Explicit `.agg({...})` is the most precise form when you want mixed operations or
  custom output names.
- Convenience terminals such as `.mean()` and `.sum()` operate on the current frame's
  non-key value features.
- Convenience `.count()` counts non-null values for the current non-key value features.
  Use `.agg({{"count", "", DataFrame::GroupOp::Count}})` when you want row count per
  group as an explicit aggregate.
- `.transform({...})` returns only the broadcasted grouped output features, not the
  key columns and not the original frame.
- `.filter(keep)` expects one boolean per group, in the same group order produced by
  `.agg(...)` on the same grouped handle.

## Workflow Map

Use this accessor when you need one of three grouped execution shapes:

- **Collapse to one row per group**: use [`agg()`](../dataframe-group/agg.md)
- **Broadcast grouped values back to row space**: use [`transform()`](../dataframe-group/transform.md)
- **Retain/drop whole groups from the original frame**: use [`filter()`](../dataframe-group/filter.md)

`by(...)` itself has three overload families:

- [`by({"city", "product"})`](../dataframe-group/by.md): ordinary feature-key grouping
- `by(RangeView<...>)`: the same thing from a dynamic key list
- `by(LabelGroup{...})`: group by case-label levels instead of ordinary features

## See Also

- [`DataFrame::aggregate`](aggregate.md): reduce the whole frame to one summary row.
- [`DataFrame::pivot_table`](pivot_table.md): grouped aggregation reshaped into columns.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="get.html">
    <span class="label">Previous</span>
    <span class="title">get</span>
  </a>
  <a class="next-link" href="gt.html">
    <span class="label">Next</span>
    <span class="title">gt</span>
  </a>
</div>
