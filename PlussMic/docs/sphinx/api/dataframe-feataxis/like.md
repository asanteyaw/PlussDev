<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: b5f896e19043 -->

# DataFrameFeatureAxisAccessor::like

[Back to DataFrameFeatureAxisAccessor API Reference](../../dataframe_feataxis_accessor_api_reference)

## Signature

```cpp
DataFrameFeatureAxisAccessor::like(std::string_view needle, bool case_sensitive = true) const -> DataFrame
```

## What It Does

Selects every feature whose **name** contains `needle` as a substring —
matching against feature names, not their values. Case labels and row order
are preserved unchanged.

## Parameters

**needle** (`std::string_view`)

Substring to search for within feature names.

**case_sensitive** (`bool`, default `true`)

When `false`, both `needle` and each feature name are lowercased before
comparing.

## Returns

`DataFrame`
    A new frame with only the matching features; same case labels and rows as
    the source.

## Examples

Example 1. Select every feature whose name contains `"units"`.

```cpp
using namespace pluss::table;

auto orders = DataFrame(
    {{"sales_units", Slate(pluss::make_series({100.0, 120.0, 90.0}))},
     {"sales_price", Slate(pluss::make_series({1.5, 2.0, 1.8}))},
     {"city",        Slate(pluss::make_note({"Accra", "Tema", "Kumasi"}))}});
std::cout << orders;

auto out = orders.feataxis().like("units");
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
==================
     | sales_units
------------------
0    | 100        
1    | 120        
2    | 90         
------------------
[3 cs x 1 feat]
```

Only `"sales_units"` contains `"units"` as a substring; `"sales_price"` and
`"city"` don't, so they're dropped.

Example 2. Pass `case_sensitive = false` to match regardless of case.

```cpp
using namespace pluss::table;

auto orders = DataFrame(
    {{"sales_units", Slate(pluss::make_series({100.0, 120.0, 90.0}))},
     {"sales_price", Slate(pluss::make_series({1.5, 2.0, 1.8}))}});

auto out = orders.feataxis().like("UNITS", false);
std::cout << out;
```

### Output

```text
==================
     | sales_units
------------------
0    | 100        
1    | 120        
2    | 90         
------------------
[3 cs x 1 feat]
```

`"UNITS"` doesn't literally appear in `"sales_units"`, but with
`case_sensitive = false` both sides are lowercased before comparing, so it
still matches. Calling `.like("UNITS")` with the default `case_sensitive =
true` on this same frame would match nothing.

Example 3. A `needle` that matches no feature name returns a zero-feature
frame instead of throwing.

```cpp
using namespace pluss::table;

auto orders = DataFrame(
    {{"sales_units", Slate(pluss::make_series({100.0, 120.0, 90.0}))},
     {"sales_price", Slate(pluss::make_series({1.5, 2.0, 1.8}))}});

auto out = orders.feataxis().like("quantity");
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

No feature name contains `"quantity"`, so the result keeps all three case
labels but has zero features — `.like()` never throws just because nothing
matched.

## Notes

- Matches feature **names**, never cell values.
- A `needle` that matches nothing returns a frame with zero features but the
  same case labels and row count — it doesn't throw.
- Use [`match()`](match.md) instead for full regular-expression matching.

## See Also

- [`DataFrameFeatureAxisAccessor::match`](match.md): regex-based feature
  selection, for patterns a plain substring search can't express.
- [`DataFrame::features`](../dataframe/features.md): select by exact name(s)
  instead of a substring or pattern.

<div class="pluss-inline-prev-next">
  <a class="next-link" href="match.html">
    <span class="label">Next</span>
    <span class="title">match</span>
  </a>
</div>
