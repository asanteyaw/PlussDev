<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 4ca964d10de7 -->

# DataFrame::filter

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signatures

```cpp
DataFrame::filter(const pluss::Series& bool_mask) const -> DataFrame
DataFrame::filter(const Slate& bool_mask) const -> DataFrame
DataFrame::filter(const Slate& bool_mask, const AlignOptions& options) const -> DataFrame
```

## What It Does

Keeps only the cases where `bool_mask` is `true`, dropping every other row, and returns
the resulting frame with all features intact. This is the row-filtering counterpart to
[`query()`](query.md): `query()` takes a string expression and builds the boolean mask
for you, while `filter()` takes an already-built mask — a boolean `Series`, or a boolean
`Slate` produced by comparing a feature against a value or another feature (`df.get("units") >= 3`, for example).

Reach for `filter()` when the mask is more naturally expressed in C++ than in a query
string — built from a helper function, combined from several intermediate boolean
slates with `&&` / `||`, or reused across several calls — and reach for `query()` when a
single self-contained string predicate reads more clearly at the call site.

Masks are usually built by comparing the `Slate` returned from
[`features(name)`](features.md) against a scalar or another feature, for example
`df.features("units") >= 3`. Note that [`get(name)`](get.md) returns
`std::optional<Slate>` rather than a bare `Slate` (it is the "does this feature exist"
lookup), so it needs `.value()` before it supports comparison operators —
`features(name)` is the more direct accessor when you already know the feature exists.

## Parameters

**bool_mask** (`const pluss::Series&` / `const Slate&`)

A boolean mask with one entry per case on the current frame. `true` keeps the
corresponding row; `false` (or a null entry, which is treated as not-true) drops it.
Both a raw `pluss::Series` and a `Slate` wrapping one are accepted directly — most
masks arise naturally as a `Slate` from a feature comparison, such as
`df.get("unit_price") > 100.0`.

**options** (`const AlignOptions&`, mask-`Slate` overload only)

Controls how the mask is aligned to the frame's case axis when the mask's own case axis
does not already match the frame exactly (for example, a mask computed from a
differently-ordered or partially-overlapping frame). Omit this parameter when the mask
was computed directly from the same frame you are filtering, which is the common case
and needs no special alignment policy.

## Returns

`DataFrame`
    A new frame containing only the cases where `bool_mask` was `true`, with the
    original feature set, feature order, and case labels of the surviving rows
    preserved.

## Examples

Example 1. Filter using a mask built from comparing a feature to a scalar.

```cpp
using namespace pluss::table;

auto orders = DataFrame({
    {"order_id",   Slate(pluss::make_series({101, 102, 103, 104, 105, 106}))},
    {"city",       Slate(pluss::make_note({"Accra", "Accra", "Tema", "Tema", "Kumasi", "Kumasi"}))},
    {"product",    Slate(pluss::make_note({"Laptop", "Headphones", "Laptop", "Monitor", "Headphones", "Monitor"}))},
    {"units",      Slate(pluss::make_series({2, 5, 1, 3, 4, 2}))},
    {"unit_price", Slate(pluss::make_series({1200.0, 45.5, 1150.0, 320.0, 42.0, std::numeric_limits<double>::quiet_NaN()}))},
    {"discount",   Slate(pluss::make_series({0.1, 0.05, std::numeric_limits<double>::quiet_NaN(), 0.08, 0.05, 0.12}))},
});

auto mask = orders.features("unit_price") > 100.0;
auto premium = orders.filter(mask);
std::cout << premium;
```

### Output

```text
============================================================
     | order_id  city   product  units  unit_price  discount
------------------------------------------------------------
0    | 101       Accra  Laptop   2      1200        0.1     
2    | 103       Tema   Laptop   1      1150        NULL    
3    | 104       Tema   Monitor  3      320         0.08    
------------------------------------------------------------
[3 cs x 6 feat]
```

Case `5`, whose `unit_price` is `NULL`, is excluded from the result along with every row
where `unit_price <= 100.0` — a null comparison never evaluates to `true`, so rows with a
missing value in the compared feature never survive a `filter()` unless the mask
explicitly accounts for that (for example, by combining the comparison with
`orders.features("unit_price").isnull()`).

Example 2. Build a compound mask in C++ instead of a query string.

```cpp
auto mask = (orders.features("units") >= 3) && (orders.features("city") != "Kumasi");
auto out = orders.filter(mask);
```

Composing boolean slates with `&&` / `||` reads naturally once a mask needs more than
one condition, and keeps the logic in ordinary C++ rather than a string that only gets
checked at runtime.

## Notes

- `filter()` performs no reordering — surviving rows keep both their original relative
  order and their original case labels.
- A mask entry that is null (rather than `true` or `false`) behaves like `false`: the
  corresponding row is dropped, not kept "just in case."
- `filter()` is non-mutating; it always returns a new frame rather than editing the
  current one.

## See Also

- [`DataFrame::query`](query.md): filter rows using a string expression instead of a
  precomputed mask.
- [`DataFrame::isnull`](isnull.md) / [`DataFrame::notnull`](notnull.md): build masks
  around missing values specifically.
- [`DataFrame::mask`](mask.md): the inverse operation — replace values where a
  condition holds instead of dropping rows.
- [`DataFrame::case_study`](case_study.md): select rows by position or label instead of
  a boolean condition.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="fillnull_.html">
    <span class="label">Previous</span>
    <span class="title">fillnull_</span>
  </a>
  <a class="next-link" href="floordiv.html">
    <span class="label">Next</span>
    <span class="title">floordiv</span>
  </a>
</div>
