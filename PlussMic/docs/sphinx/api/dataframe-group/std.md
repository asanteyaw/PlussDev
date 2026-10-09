<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: d117a1f13867 -->

# DataFrameGrouped::std

[Back to DataFrameGroupBuilder API Reference](../../dataframe_group_accessor_api_reference)

## Signatures

```cpp
DataFrameGrouped::std() const -> DataFrame
DataFrameGrouped::std(std::string_view feature) const -> DataFrame
DataFrameGrouped::std(std::initializer_list<std::string_view> features) const -> DataFrame
DataFrameGrouped::std(pluss::RangeView<std::string_view> features) const -> DataFrame
```

## What It Does

Computes grouped sample standard deviation (`n - 1` denominator) for each non-key
value feature — the grouped convenience terminal equivalent to calling
[`agg()`](agg.md) with a `GroupOp::Std` spec for every remaining value feature. A
group needs at least **two** observed values to produce a defined standard
deviation; a group with zero or one observed value produces null instead of `0`.

When you pass one or more feature names, `std(...)` only aggregates those selected
value features instead of every remaining non-key value feature.

## Parameters

**feature** / **features** (`std::string_view` / `initializer_list<string_view>` /
`RangeView<string_view>`, optional)

Select one or more non-key value features to standardize by group. Omit them to reduce
every remaining non-key value feature on the current frame.

## Returns

`DataFrame`
    One row per group. The group key column(s) come first, followed by one
    `{feature}_std` column per non-key value feature.

## Examples

Example 1. One region with three observations, one with a single observation.

```cpp
using namespace pluss::table;

auto loans = DataFrame({
    {"region", Slate(pluss::make_note({"North", "North", "North", "South"}))},
    {"amount", Slate(pluss::make_series({10.0, 20.0, 30.0, 50.0}))},
});
std::cout << loans;

auto out = loans.features({"region", "amount"})
               .group()
               .by({"region"})
               .std();
std::cout << out;
```

### Output (loans)

```text
=====================
     | region  amount
---------------------
0    | North   10    
1    | North   20    
2    | North   30    
3    | South   50    
---------------------
[4 cs x 2 feat]
```

### Output (out)

```text
=========================
     | region  amount_std
-------------------------
0    | North   10        
1    | South   NULL      
-------------------------
[2 cs x 2 feat]
```

`"North"` has three values (`10, 20, 30`), mean `20`; the sample standard deviation
(`n - 1 = 2` in the denominator) works out to `10`. `"South"` has only one value, so
there is no defined spread — `amount_std` is `NULL`, not `0`.

Example 2. Standard deviation only for one selected feature.

```cpp
auto wide = DataFrame({
    {"region", Slate(pluss::make_note({"North", "North", "North", "South"}))},
    {"amount", Slate(pluss::make_series({10.0, 20.0, 30.0, 50.0}))},
    {"units", Slate(pluss::make_series({1.0, 3.0, 5.0, 7.0}))},
});

auto out2 = wide.group().by({"region"}).std({"units"});
std::cout << out2;
```

### Output (out2)

```text
========================
     | region  units_std
------------------------
0    | North   2
1    | South   NULL
------------------------
[2 cs x 2 feat]
```

## Notes

- This is the grouped dispersion companion to [`mean()`](mean.md).
- Requires at least two observed values per group; groups with `0` or `1` observed
  values are null in the result.
- Use it only on value features where standard deviation is meaningful (numeric,
  additive quantities).

## See Also

- [`DataFrameGrouped::var`](var.md)
- [`DataFrameGrouped::mean`](mean.md)

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="max.html">
    <span class="label">Previous</span>
    <span class="title">max</span>
  </a>
  <a class="next-link" href="var.html">
    <span class="label">Next</span>
    <span class="title">var</span>
  </a>
</div>
