<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: d59279a549e8 -->

# DataFrameGrouped::median

[Back to DataFrameGroupBuilder API Reference](../../dataframe_group_accessor_api_reference)

## Signatures

```cpp
DataFrame DataFrameGrouped::median() const
DataFrame DataFrameGrouped::median(std::string_view feature) const
DataFrame DataFrameGrouped::median(std::initializer_list<std::string_view> features) const
DataFrame DataFrameGrouped::median(pluss::RangeView<std::string_view> features) const
```

## What It Does

Computes group-wise median for each non-key value feature — the grouped convenience
terminal equivalent to calling [`agg()`](agg.md) with a `GroupOp::Median` spec for
every remaining value feature. For a group with an odd number of observed values,
the median is the middle value once sorted; for an even count, it is the average of
the two middle values.

When you pass one or more feature names, `median(...)` only aggregates those selected
value features instead of every remaining non-key value feature.

## Parameters

**feature** / **features** (`std::string_view` / `initializer_list<string_view>` /
`RangeView<string_view>`, optional)

Select one or more non-key value features to median-reduce. Omit them to reduce every
remaining non-key value feature on the current frame.

## Returns

`DataFrame`
    One row per group. The group key column(s) come first, followed by one
    `{feature}_median` column per non-key value feature.

## Examples

Example 1. One group with an odd count, one with an even count.

```cpp
using namespace pluss::table;

auto orders = DataFrame({
    {"city", Slate(pluss::make_note({"Accra", "Accra", "Accra", "Tema", "Tema"}))},
    {"amount", Slate(pluss::make_series({10.0, 50.0, 20.0, 30.0, 70.0}))},
});
std::cout << orders;

auto out = orders.features({"city", "amount"})
               .group()
               .by({"city"})
               .median();
std::cout << out;
```

### Output (orders)

```text
====================
     | city   amount
--------------------
0    | Accra  10    
1    | Accra  50    
2    | Accra  20    
3    | Tema   30    
4    | Tema   70    
--------------------
[5 cs x 2 feat]
```

### Output (out)

```text
===========================
     | city   amount_median
---------------------------
0    | Accra  20           
1    | Tema   50           
---------------------------
[2 cs x 2 feat]
```

`"Accra"` has three values; sorted, `10, 20, 50`, the middle one is `20`.
`"Tema"` has two values, `30` and `70`; with an even count the median is their
average, `(30 + 70) / 2 = 50`.

Example 2. Median only one selected value feature.

```cpp
auto wide = DataFrame({
    {"city", Slate(pluss::make_note({"Accra", "Accra", "Tema", "Tema"}))},
    {"amount", Slate(pluss::make_series({10.0, 50.0, 30.0, 70.0}))},
    {"units", Slate(pluss::make_series({2.0, 9.0, 1.0, 3.0}))},
});

auto out2 = wide.group().by({"city"}).median({"amount"});
std::cout << out2;
```

### Output (out2)

```text
===========================
     | city   amount_median
---------------------------
0    | Accra  30
1    | Tema   50
---------------------------
[2 cs x 2 feat]
```

## Notes

- Median is robust to extreme values in ways mean is not.
- Like the other grouped terminals, it can either apply uniformly to every remaining
  non-key value feature or to a targeted selected subset.
- A group where the value feature is entirely null has a null median, not `0`.

## See Also

- [`DataFrameGrouped::mean`](mean.md)
- [`DataFrameGrouped::mode`](mode.md)

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="mean.html">
    <span class="label">Previous</span>
    <span class="title">mean</span>
  </a>
  <a class="next-link" href="min.html">
    <span class="label">Next</span>
    <span class="title">min</span>
  </a>
</div>
