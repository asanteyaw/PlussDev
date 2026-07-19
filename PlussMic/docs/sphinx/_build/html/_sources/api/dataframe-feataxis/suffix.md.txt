<!-- pluss-hand-authored -->

# DataFrameFeatureAxisAccessor::suffix

[Back to DataFrameFeatureAxisAccessor API Reference](../../dataframe_feataxis_accessor_api_reference)

## Signature

```cpp
DataFrameFeatureAxisAccessor::suffix(std::string_view suffix) const -> DataFrame
```

## What It Does

Appends `suffix` to **every** feature name on the frame. Values, case labels,
and row order are all unaffected — only the names change.

## Parameters

**suffix** (`std::string_view`)

Text appended after every existing feature name.

## Returns

`DataFrame`
    A new frame with every feature renamed to `original_name + suffix`.

## Examples

Example 1. Suffix every feature name on a mixed-kind frame.

```cpp
using namespace pluss::table;

auto orders = DataFrame(
    {{"sales_units", Thread(pluss::make_series({100.0, 120.0, 90.0}))},
     {"sales_price", Thread(pluss::make_series({1.5, 2.0, 1.8}))},
     {"city",        Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))}});
std::cout << orders;

auto out = orders.feataxis().suffix("_v2");
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
==============================================
     | sales_units_v2  sales_price_v2  city_v2
----------------------------------------------
0    | 100             1.5             Accra  
1    | 120             2               Tema   
2    | 90              1.8             Kumasi 
----------------------------------------------
[3 cs x 3 feat]
```

Every feature name gets `"_v2"` appended — there's no way to target only some
of them with this method.

## Notes

- Applies to **every** feature, unconditionally — combine with
  [`like()`](like.md)/[`match()`](match.md) first (and rejoin) if only some
  features should be renamed.
- Values, case labels, and row order are completely unaffected.

## See Also

- [`DataFrameFeatureAxisAccessor::prefix`](prefix.md): the mirror-image
  operation, prepending instead of appending.
- [`DataFrameLabelAccessor::rename`](../dataframe-label/rename.md): rename
  case-label *levels* rather than feature columns.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="series.html">
    <span class="label">Previous</span>
    <span class="title">series</span>
  </a>
  <a class="next-link" href="timeline.html">
    <span class="label">Next</span>
    <span class="title">timeline</span>
  </a>
</div>
