<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: a1efe1224539 -->

# DataFrameGrouped::mode

[Back to DataFrameGroupBuilder API Reference](../../dataframe_group_accessor_api_reference)

## Signatures

```cpp
DataFrameGrouped::mode() const -> DataFrame
DataFrameGrouped::mode(std::string_view feature) const -> DataFrame
DataFrameGrouped::mode(std::initializer_list<std::string_view> features) const -> DataFrame
DataFrameGrouped::mode(pluss::RangeView<std::string_view> features) const -> DataFrame
```

## What It Does

Computes the grouped mode (most frequent value) for each non-key value feature —
the grouped convenience terminal equivalent to calling [`agg()`](agg.md) with a
`GroupOp::Mode` spec for every remaining value feature.

This is the grouped "most common value inside each group" terminal. It is especially
useful for categorical/value-coded features such as status, region code, product
family, or segment, but it also works on numeric value features.

When you pass one or more feature names, `mode(...)` only aggregates those selected
value features instead of every remaining non-key value feature.

## Parameters

**feature** / **features** (`std::string_view` / `initializer_list<string_view>` /
`RangeView<string_view>`, optional)

Select one or more non-key value features to mode-reduce. Omit them to reduce every
remaining non-key value feature on the current frame.

## Returns

`DataFrame`
    One row per group. The group key column(s) come first, followed by one
    `{feature}_mode` column per non-key value feature.

## Examples

Example 1. Most common loan status per region, including a tie.

```cpp
using namespace pluss::table;

auto loans = DataFrame({
    {"region", Slate(pluss::make_note({"North", "North", "South", "South"}))},
    {"status", Slate(pluss::make_note({"good", "good", "bad", "good"}))},
});
std::cout << loans;

auto out = loans.features({"region", "status"})
               .group()
               .by({"region"})
               .mode();
std::cout << out;
```

### Output (loans)

```text
=====================
     | region  status
---------------------
0    | North   good  
1    | North   good  
2    | South   bad   
3    | South   good  
---------------------
[4 cs x 2 feat]
```

### Output (out)

```text
==========================
     | region  status_mode
--------------------------
0    | North   good       
1    | South   bad        
--------------------------
[2 cs x 2 feat]
```

`"North"`'s two statuses are both `"good"`, so its mode is unambiguous. `"South"` has
one `"bad"` and one `"good"` — tied at one occurrence each — and `status_mode` picks
`"bad"` because it appears first in the source data (row `2`), not because it sorts
first alphabetically. This is the same first-occurrence tie-break used by
[`Slate::freq()`](../slate/freq.md).

Example 2. Target one selected feature on a wider value side.

```cpp
auto wide = DataFrame({
    {"region", Slate(pluss::make_note({"North", "North", "South", "South"}))},
    {"status", Slate(pluss::make_note({"good", "good", "bad", "good"}))},
    {"segment", Slate(pluss::make_note({"retail", "retail", "sme", "sme"}))},
});

auto out2 = wide.group().by({"region"}).mode({"segment"});
std::cout << out2;
```

### Output (out2)

```text
============================
     | region  segment_mode
----------------------------
0    | North   retail
1    | South   sme
----------------------------
[2 cs x 2 feat]
```

## Notes

- Use [`agg()`](agg.md) instead if you want grouped mode beside numeric grouped
  outputs in the same result with explicit names.
- Mode is often the most natural grouped terminal for note/categorical features,
  whereas `mean()`/`std()`/`var()` are more naturally numeric — but `mode()` also
  works on numeric value features, with the same first-occurrence tie-break.
- A group where the value feature is entirely null has a null mode, not an arbitrary
  value.

## See Also

- [`DataFrameGrouped::mode_ties`](mode_ties.md): returns every tied value instead of
  picking one winner — use it when a tie should not be silently resolved.
- [`DataFrameGrouped::median`](median.md)
- [`DataFrameGrouped::agg`](agg.md)

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="var.html">
    <span class="label">Previous</span>
    <span class="title">var</span>
  </a>
  <a class="next-link" href="mode_ties.html">
    <span class="label">Next</span>
    <span class="title">mode_ties</span>
  </a>
</div>
