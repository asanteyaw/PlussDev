<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 3a69869c1ec5 -->

# DataFrameFeatureAxisAccessor::series

[Back to DataFrameFeatureAxisAccessor API Reference](../../dataframe_feataxis_accessor_api_reference)

## Signature

```cpp
DataFrame DataFrameFeatureAxisAccessor::series() const
```

## What It Does

Selects every feature whose `Slate` kind is `Series` (numeric or boolean) —
dropping text (`Note`) and temporal (`Timeline`/`TimeIndex`) features. Case
labels and row order are preserved unchanged.

## Returns

`DataFrame`
    A new frame with only the `Series`-kind features; same case labels and
    rows as the source.

## Examples

Example 1. Keep only the numeric features out of a mixed frame.

```cpp
using namespace pluss::table;

auto orders = DataFrame(
    {{"sales_units", Slate(pluss::make_series({100.0, 120.0, 90.0}))},
     {"sales_price", Slate(pluss::make_series({1.5, 2.0, 1.8}))},
     {"city",        Slate(pluss::make_note({"Accra", "Tema", "Kumasi"}))}});
std::cout << orders;

auto out = orders.feataxis().series();
std::cout << out;
```

### Output (before)

```text
=======================================
     | sales_units  sales_price  city  
---------------------------------------
0    | 100          1.5          Accra 
1    | 120          2            Tema  
2    | 90           1.8          Kumasi
---------------------------------------
[3 cs x 3 feat]
```

### Output (after)

```text
===============================
     | sales_units  sales_price
-------------------------------
0    | 100          1.5        
1    | 120          2          
2    | 90           1.8        
-------------------------------
[3 cs x 2 feat]
```

`"sales_units"` and `"sales_price"` are both numeric `Series` columns;
`"city"` is `Note`-kind and is dropped.

Example 2. A boolean feature counts as `Series` too, so it survives
alongside numeric features.

```cpp
using namespace pluss::table;

auto orders = DataFrame(
    {{"sales_units", Slate(pluss::make_series({100.0, 120.0, 90.0}))},
     {"in_stock",     Slate(pluss::make_series({true, false, true}))}});

auto out = orders.feataxis().series();
std::cout << out;
```

### Output

```text
============================
     | sales_units  in_stock
----------------------------
0    | 100          true    
1    | 120          false   
2    | 90           true    
----------------------------
[3 cs x 2 feat]
```

Nothing is dropped — both `"sales_units"` (numeric) and `"in_stock"`
(boolean) are `Series`-kind, so `series()` doesn't split them apart by
scalar type.

## Notes

- If no feature is `Series`-kind, the result has zero features but the same
  case labels and row count.
- `Series` covers every numeric and boolean dtype alike — there's no further
  split by scalar type here.

## See Also

- [`DataFrameFeatureAxisAccessor::note`](note.md),
  [`DataFrameFeatureAxisAccessor::timeline`](timeline.md): the analogous
  filters for the other two payload kinds.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="prefix.html">
    <span class="label">Previous</span>
    <span class="title">prefix</span>
  </a>
  <a class="next-link" href="suffix.html">
    <span class="label">Next</span>
    <span class="title">suffix</span>
  </a>
</div>
