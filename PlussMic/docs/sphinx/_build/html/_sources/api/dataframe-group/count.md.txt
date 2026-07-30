<!-- pluss-hand-authored -->

# DataFrameGrouped::count

[Back to DataFrameGroupBuilder API Reference](../../dataframe_group_accessor_api_reference)

## Signature

```cpp
DataFrameGrouped::count() const -> DataFrame
```

## What It Does

Counts non-null observed values per group for the current non-key value features.

This is the grouped convenience terminal for "how many observed entries does each
value feature contribute inside each group?" — a shortcut for calling
[`agg()`](agg.md) with a `GroupOp::Count` spec for every remaining value feature.

## Parameters

This method takes no parameters.

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
    {"city", Thread(pluss::make_note({"Accra", "Accra", "Tema", "Tema"}))},
    {"amount", Thread(pluss::make_series(
        {100.0, std::numeric_limits<double>::quiet_NaN(), 50.0, 40.0}))},
    {"units", Thread(pluss::make_series({2.0, 1.0, 3.0, 1.0}))},
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

## Notes

- Convenience terminals exclude ordinary grouping keys from the value side before
  aggregating — narrow with [`DataFrame::features`](../dataframe/features.md) first
  if the source frame has columns you do not want counted.
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
