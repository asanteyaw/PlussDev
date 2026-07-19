<!-- pluss-hand-authored -->

# DataFrame::where

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signatures

```cpp
DataFrame::where(const pluss::Series& bool_mask) const -> DataFrame
DataFrame::where(const Thread& bool_mask) const -> DataFrame
DataFrame::where(const Thread& bool_mask, const AlignOptions& options) const -> DataFrame
```

## What It Does

An exact alias for [`filter()`](filter.md) — `where(bool_mask, ...)` literally
delegates to `filter(bool_mask, ...)` for both the `pluss::Series` and `Thread` mask
overloads, keeping only the rows where the mask is `true` and dropping the rest.
Note this is **not** numpy/pandas `.where()`'s replace-in-place semantics (which
keeps every row and only replaces individual masked-out *values*) — for that kind
of masked replacement within a single feature, reach for [`put()`](put.md) instead.

## Parameters

**bool_mask** (`const pluss::Series&` or `const Thread&`)

A boolean mask whose length must equal the frame's case count. Rows where the mask
is `true` are kept; rows where it's `false` (or `NULL`) are dropped.

**options** (`const AlignOptions&`, `Thread`-mask overload only)

Alignment options controlling how a case-label-bearing `Thread` mask is matched
against this frame's own case labels before filtering.

## Returns

`DataFrame`
    A new frame containing only the rows where `bool_mask` is `true`.

## Examples

Example 1. Filter rows with an explicit boolean mask.

```cpp
using namespace pluss::table;

auto orders = DataFrame({
    {"id",    Thread(pluss::make_series({1, 2, 3}))},
    {"sales", Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city",  Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});
std::cout << orders;
```

### Output (before)

```text
==========================
     | id    sales  city  
--------------------------
0    | 1     100    Accra 
1    | 2     120    Tema  
2    | 3     90     Kumasi
--------------------------
[3 cs x 3 feat]
```

```cpp
auto mask = pluss::make_series({true, false, true}, pluss::ScalarType::Bool);
auto kept = orders.where(mask);
std::cout << kept;
```

### Output (after)

```text
==========================
     | id    sales  city  
--------------------------
0    | 1     100    Accra 
2    | 3     90     Kumasi
--------------------------
[2 cs x 3 feat]
```

Case `1` (`Tema`, `sales = 120`) is dropped since `mask[1]` is `false`; the two
`true` cases keep their original case labels (`0` and `2`), not renumbered.

Example 2. Build the mask from a frame expression instead of a literal.

```cpp
auto mask_expr = orders.features("sales") >= 95.0;
auto kept2 = orders.where(mask_expr);
std::cout << kept2;
```

### Output

```text
=========================
     | id    sales  city 
-------------------------
0    | 1     100    Accra
1    | 2     120    Tema 
-------------------------
[2 cs x 3 feat]
```

`sales >= 95.0` is `true` for cases `0` (`100`) and `1` (`120`), and `false` for
case `2` (`90`), so this mask keeps a different pair of rows than Example 1's
literal mask did.

## Notes

- `where()` is a pure alias for [`filter()`](filter.md) — anything documented there
  (mask-length validation, `NULL`-in-mask handling, case-label preservation) applies
  identically here.
- This is **not** numpy/pandas-style `.where()` replace semantics — it drops rows
  rather than replacing individual masked values while keeping every row. Use
  [`put()`](put.md) for masked, in-place-style value replacement within one feature.
- Kept rows retain their original case labels; the result is never re-indexed.

## See Also

- [`DataFrame::filter`](filter.md): the underlying implementation this method
  delegates to — functionally identical.
- [`DataFrame::put`](put.md): masked value replacement within a feature, the closer
  analogue to numpy/pandas `.where()`'s actual semantics.
- [`DataFrame::query`](query.md): a string-expression-driven alternative to building
  an explicit boolean mask by hand.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="view.html">
    <span class="label">Previous</span>
    <span class="title">view</span>
  </a>
</div>
