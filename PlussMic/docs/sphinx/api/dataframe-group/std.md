<!-- pluss-hand-authored -->

# DataFrameGrouped::std

[Back to DataFrameGroupBuilder API Reference](../../dataframe_group_accessor_api_reference)

## Signature

```cpp
DataFrameGrouped::std() const -> DataFrame
```

## What It Does

Computes grouped sample standard deviation (`n - 1` denominator) for each non-key
value feature — the grouped convenience terminal equivalent to calling
[`agg()`](agg.md) with a `GroupOp::Std` spec for every remaining value feature. A
group needs at least **two** observed values to produce a defined standard
deviation; a group with zero or one observed value produces null instead of `0`.

## Parameters

This method takes no parameters.

## Returns

`DataFrame`
    One row per group. The group key column(s) come first, followed by one
    `{feature}_std` column per non-key value feature.

## Examples

Example 1. One region with three observations, one with a single observation.

```cpp
using namespace pluss::table;

auto loans = DataFrame({
    {"region", Thread(pluss::make_note({"North", "North", "North", "South"}))},
    {"amount", Thread(pluss::make_series({10.0, 20.0, 30.0, 50.0}))},
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
