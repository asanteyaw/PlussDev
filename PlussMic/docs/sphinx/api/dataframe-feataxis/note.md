<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 02292526d20b -->

# DataFrameFeatureAxisAccessor::note

[Back to DataFrameFeatureAxisAccessor API Reference](../../dataframe_feataxis_accessor_api_reference)

## Signature

```cpp
DataFrame DataFrameFeatureAxisAccessor::note() const
```

## What It Does

Selects every feature whose `Slate` kind is `Note` (text) — dropping numeric
(`Series`) and temporal (`Timeline`/`TimeIndex`) features. Case labels and row
order are preserved unchanged.

## Returns

`DataFrame`
    A new frame with only the `Note`-kind features; same case labels and rows
    as the source.

## Examples

Example 1. Keep only the text feature out of a mixed frame.

```cpp
using namespace pluss::table;

auto orders = DataFrame(
    {{"sales_units", Slate(pluss::make_series({100.0, 120.0, 90.0}))},
     {"sales_price", Slate(pluss::make_series({1.5, 2.0, 1.8}))},
     {"city",        Slate(pluss::make_note({"Accra", "Tema", "Kumasi"}))}});
std::cout << orders;

auto out = orders.feataxis().note();
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
=============
     | city  
-------------
0    | Accra 
1    | Tema  
2    | Kumasi
-------------
[3 cs x 1 feat]
```

Example 2. Call `note()` on a frame with no `Note`-kind feature at all.

```cpp
using namespace pluss::table;

auto readings = DataFrame(
    {{"sales_units", Slate(pluss::make_series({100.0, 120.0, 90.0}))},
     {"sales_price", Slate(pluss::make_series({1.5, 2.0, 1.8}))}});

auto out = readings.feataxis().note();
std::cout << out;
```

### Output

```text
====
    
----
0   
1   
2   
----
[3 cs x 0 feat]
```

Neither feature is `Note`-kind, so the result keeps all three case labels but
has zero features — it doesn't throw just because nothing qualified.

## Notes

- If no feature is `Note`-kind, the result has zero features but the same
  case labels and row count.
- Feature order among the survivors matches their original order in the
  source frame.

## See Also

- [`DataFrameFeatureAxisAccessor::series`](series.md),
  [`DataFrameFeatureAxisAccessor::timeline`](timeline.md): the analogous
  filters for the other two payload kinds.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="match.html">
    <span class="label">Previous</span>
    <span class="title">match</span>
  </a>
  <a class="next-link" href="prefix.html">
    <span class="label">Next</span>
    <span class="title">prefix</span>
  </a>
</div>
