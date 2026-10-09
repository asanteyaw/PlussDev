<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 5f5b17cb7421 -->

# DataFrameGrouped::ndistinct

[Back to DataFrameGroupBuilder API Reference](../../dataframe_group_accessor_api_reference)

## Signatures

```cpp
DataFrameGrouped::ndistinct() const -> DataFrame
DataFrameGrouped::ndistinct(std::string_view feature) const -> DataFrame
DataFrameGrouped::ndistinct(std::initializer_list<std::string_view> features) const -> DataFrame
DataFrameGrouped::ndistinct(pluss::RangeView<std::string_view> features) const -> DataFrame
```

## What It Does

Counts the number of distinct observed (non-null) values per group for each non-key
value feature. This is the grouped convenience terminal equivalent to calling
[`agg()`](agg.md) with a `GroupOp::Ndistinct` spec for every remaining value feature.

When you pass one or more feature names, `ndistinct(...)` only aggregates those
selected value features instead of every remaining non-key value feature.

## Parameters

**feature** / **features** (`std::string_view` / `initializer_list<string_view>` /
`RangeView<string_view>`, optional)

Select one or more non-key value features to distinct-count. Omit them to aggregate
every remaining non-key value feature on the current frame.

## Returns

`DataFrame`
    One row per group. The group key column(s) come first, followed by one
    `{feature}_ndistinct` column per non-key value feature.

## Examples

Example 1. Count distinct products ordered per city.

```cpp
using namespace pluss::table;

auto orders = DataFrame({
    {"city", Slate(pluss::make_note({"Accra", "Accra", "Accra", "Tema", "Tema"}))},
    {"product", Slate(pluss::make_note({"Tea", "Tea", "Coffee", "Tea", "Tea"}))},
});
std::cout << orders;

auto out = orders.features({"city", "product"})
               .group()
               .by({"city"})
               .ndistinct();
std::cout << out;
```

### Output (orders)

```text
=====================
     | city   product
---------------------
0    | Accra  Tea    
1    | Accra  Tea    
2    | Accra  Coffee 
3    | Tema   Tea    
4    | Tema   Tea    
---------------------
[5 cs x 2 feat]
```

### Output (out)

```text
===============================
     | city   product_ndistinct
-------------------------------
0    | Accra  2                
1    | Tema   1                
-------------------------------
[2 cs x 2 feat]
```

`"Accra"` ordered `"Tea"` twice and `"Coffee"` once — two distinct products, so
`product_ndistinct` is `2`. `"Tema"` ordered `"Tea"` both times, so its
`product_ndistinct` is `1`, even though it also has two rows.

Example 2. Count distinctness only for one selected value feature.

```cpp
auto wide = DataFrame({
    {"city", Slate(pluss::make_note({"Accra", "Accra", "Tema", "Tema"}))},
    {"product", Slate(pluss::make_note({"Tea", "Coffee", "Tea", "Tea"}))},
    {"rep", Slate(pluss::make_note({"Ama", "Ama", "Kojo", "Esi"}))},
});

auto out2 = wide.group().by({"city"}).ndistinct({"rep"});
std::cout << out2;
```

### Output (out2)

```text
===========================
     | city   rep_ndistinct
---------------------------
0    | Accra  1
1    | Tema   2
---------------------------
[2 cs x 2 feat]
```

## Notes

- This is per-feature distinctness, not tuple distinctness across several value
  features at once — each remaining value feature gets its own independent
  distinct-count column.
- Null values are never counted as a distinct value.
- It is a convenience grouped terminal; use [`agg()`](agg.md) when you need explicit
  mixed grouped specs alongside it.

## See Also

- [`DataFrame::ndistinct`](../dataframe/ndistinct.md): whole-frame distinct counting.
- [`DataFrameGrouped::count`](count.md)

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="count.html">
    <span class="label">Previous</span>
    <span class="title">count</span>
  </a>
  <a class="next-link" href="sum.html">
    <span class="label">Next</span>
    <span class="title">sum</span>
  </a>
</div>
