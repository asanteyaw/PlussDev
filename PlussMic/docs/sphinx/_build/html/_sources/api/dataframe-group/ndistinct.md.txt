<!-- pluss-hand-authored -->

# DataFrameGrouped::ndistinct

[Back to DataFrameGroupBuilder API Reference](../../dataframe_group_accessor_api_reference)

## Signature

```cpp
DataFrameGrouped::ndistinct() const -> DataFrame
```

## What It Does

Counts the number of distinct observed (non-null) values per group for each non-key
value feature. This is the grouped convenience terminal equivalent to calling
[`agg()`](agg.md) with a `GroupOp::Ndistinct` spec for every remaining value feature.

## Parameters

This method takes no parameters.

## Returns

`DataFrame`
    One row per group. The group key column(s) come first, followed by one
    `{feature}_ndistinct` column per non-key value feature.

## Examples

Example 1. Count distinct products ordered per city.

```cpp
using namespace pluss::table;

auto orders = DataFrame({
    {"city", Thread(pluss::make_note({"Accra", "Accra", "Accra", "Tema", "Tema"}))},
    {"product", Thread(pluss::make_note({"Tea", "Tea", "Coffee", "Tea", "Tea"}))},
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
