<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 259b0b36e375 -->

# DataFrameGrouped::count

[Back to DataFrameGroupBuilder API Reference](../../dataframe_group_accessor_api_reference)

## Signature

```cpp
DataFrame DataFrameGrouped::count() const
DataFrame DataFrameGrouped::count(std::string_view feature) const
DataFrame DataFrameGrouped::count(std::initializer_list<std::string_view> features) const
DataFrame DataFrameGrouped::count(pluss::RangeView<std::string_view> features) const
```

## What It Does

Counts non-null observed values per group for the current non-key value features.

This is the grouped convenience terminal for "how many observed entries does each
value feature contribute inside each group?" — a shortcut for calling
[`agg()`](agg.md) with a `GroupOp::Count` spec for every remaining value feature.

When you pass one or more feature names, `count(...)` only counts those selected
value features instead of every remaining non-key feature.

## Parameters

**feature** / **features** (`std::string_view` / `initializer_list<string_view>` /
`RangeView<string_view>`, optional)

Select one or more non-key value features to count. Omit them to count every
remaining non-key value feature on the current frame.

## Returns

`DataFrame`
    One row per group. The group key column(s) come first, followed by one
    `{feature}_count` column per non-key value feature still present on the frame —
    or, if no value features remain, a single column literally named `count`.

## Examples

Example 1. Count observed values per city, with one missing `amount`.

```cpp
using namespace pluss::table;

auto sales = DataFrame({
    {"city", Slate(pluss::make_note({"Accra", "Accra", "Tema", "Tema"}))},
    {"amount", Slate(pluss::make_series(
        {100.0, std::numeric_limits<double>::quiet_NaN(), 50.0, 40.0}))},
    {"units", Slate(pluss::make_series({2.0, 1.0, 3.0, 1.0}))},
});
std::cout << sales;

auto out = sales.features({"city", "amount", "units"})
               .group()
               .by({"city"})
               .count();
std::cout << out;
```

### Output (sales)

```text
===========================
     | city   amount  units
---------------------------
0    | Accra  100     2    
1    | Accra  NULL    1    
2    | Tema   50      3    
3    | Tema   40      1    
---------------------------
[4 cs x 3 feat]
```

### Output (out)

```text
=======================================
     | city   amount_count  units_count
---------------------------------------
0    | Accra  1             2          
1    | Tema   2             2          
---------------------------------------
[2 cs x 3 feat]
```

`"Accra"` has two rows, but one `amount` value is `NaN` (missing), so
`amount_count` is `1`, not `2`. `units_count` is unaffected because neither of
`"Accra"`'s `units` values is missing.

Example 2. Count only one selected value feature.

```cpp
auto out2 = sales.group()
                .by({"city"})
                .count({"amount"});
std::cout << out2;
```

### Output (out2)

```text
========================
     | city   amount_count
------------------------
0    | Accra  1
1    | Tema   2
------------------------
[2 cs x 2 feat]
```

## Notes

- Convenience terminals exclude ordinary grouping keys from the value side before
  aggregating — narrow with [`DataFrame::features`](../dataframe/features.md) first
  if the source frame has columns you do not want counted.
- Passing `count({"amount"})` or `count("amount")` is the more direct targeted form
  when you only want a subset of the value side.
- If no non-key value features remain, `.count()` still returns an explicit row-count
  column named `count`, counting rows rather than any particular feature's non-null
  entries.
- If you need row count beside other explicit grouped metrics, prefer
  [`agg()`](agg.md) with `{"n", "", GroupOp::Count}`.

## See Also

- [`DataFrameGrouped::agg`](agg.md)
- [`DataFrameGrouped::ndistinct`](ndistinct.md)

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="filter.html">
    <span class="label">Previous</span>
    <span class="title">filter</span>
  </a>
  <a class="next-link" href="ndistinct.html">
    <span class="label">Next</span>
    <span class="title">ndistinct</span>
  </a>
</div>
