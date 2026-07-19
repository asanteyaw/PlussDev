<!-- pluss-hand-authored -->

# DataFrameFeatureAxisAccessor::prefix

[Back to DataFrameFeatureAxisAccessor API Reference](../../dataframe_feataxis_accessor_api_reference)

## Signature

```cpp
DataFrameFeatureAxisAccessor::prefix(std::string_view prefix) const -> DataFrame
```

## What It Does

Prepends `prefix` to **every** feature name on the frame. Values, case labels,
and row order are all unaffected — only the names change.

## Parameters

**prefix** (`std::string_view`)

Text inserted before every existing feature name.

## Returns

`DataFrame`
    A new frame with every feature renamed to `prefix + original_name`.

## Examples

Example 1. Prefix every feature name on a mixed-kind frame.

```cpp
using namespace pluss::table;

auto orders = DataFrame(
    {{"sales_units", Thread(pluss::make_series({100.0, 120.0, 90.0}))},
     {"sales_price", Thread(pluss::make_series({1.5, 2.0, 1.8}))},
     {"city",        Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))}});
std::cout << orders;

auto out = orders.feataxis().prefix("raw_");
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
=================================================
     | raw_sales_units  raw_sales_price  raw_city
-------------------------------------------------
0    | 100              1.5              Accra   
1    | 120              2                Tema    
2    | 90               1.8              Kumasi  
-------------------------------------------------
[3 cs x 3 feat]
```

Every feature name gets `"raw_"` prepended — there's no way to target only
some of them with this method.

## Notes

- Applies to **every** feature, unconditionally — combine with
  [`like()`](like.md)/[`match()`](match.md) first (and rejoin) if only some
  features should be renamed.
- Values, case labels, and row order are completely unaffected.

## See Also

- [`DataFrameFeatureAxisAccessor::suffix`](suffix.md): the mirror-image
  operation, appending instead of prepending.
- [`DataFrameLabelAccessor::rename`](../dataframe-label/rename.md): rename
  case-label *levels* rather than feature columns.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="note.html">
    <span class="label">Previous</span>
    <span class="title">note</span>
  </a>
  <a class="next-link" href="series.html">
    <span class="label">Next</span>
    <span class="title">series</span>
  </a>
</div>
