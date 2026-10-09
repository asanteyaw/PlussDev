<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: da48b97365ae -->

# DataFrame::drop_duplicates

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signatures

```cpp
DataFrame::drop_duplicates(bool keep_first = true, bool keep_last = false) const -> DataFrame
DataFrame::drop_duplicates(pluss::RangeView<std::string_view> features, bool keep_first = true, bool keep_last = false) const -> DataFrame
```

## What It Does

Removes duplicate rows and returns a new `DataFrame` containing only the survivors —
exactly the rows [`duplicated()`](duplicated.md) would flag `false` under the same
`keep_first` / `keep_last` policy. Surviving rows keep their original feature set,
feature order, and case labels; nothing is renumbered or reordered.

Called with no `features`, every column on the frame is compared. Unlike the other
selector-bearing reductions in this family — [`any()`](any.md), [`all()`](all.md),
[`ndistinct()`](ndistinct.md), [`duplicated()`](duplicated.md),
[`repeated()`](repeated.md), [`corr()`](corr.md) — where narrowing first with
[`features()`](features.md) is a direct, lossless replacement for an explicit selector,
`drop_duplicates()` keeps its own `features` overload. Narrowing first with
`.features({...})` before calling `drop_duplicates()` would also narrow the
*survivors'* columns, since `drop_duplicates()` returns whatever frame it was called
on — not what's usually wanted when comparing on a subset of columns. Passing
`features` directly to `drop_duplicates()` instead compares on just those columns
while every other column on the frame survives untouched (Example 1).

## Parameters

**features** (`pluss::RangeView<std::string_view>`, optional)

The feature(s) to compare when deciding whether two rows are duplicates. When
omitted, every feature on the frame is compared. Every other feature on the frame is
left untouched on the surviving rows either way.

**keep_first** (`bool`, default `true`)

When `true`, the first row in each duplicate group survives and every later row in
that group is dropped.

**keep_last** (`bool`, default `false`)

When `true`, the last row in each duplicate group survives and every earlier row in
that group is dropped. Cannot be combined with `keep_first = true`.

## Returns

`DataFrame`
    A new frame containing one row per distinct combination of the compared
    features — the survivor chosen by `keep_first` / `keep_last` — with the original
    feature set, feature order, and case labels of the surviving rows.

## Examples

Example 1. Keep the first occurrence of each `(city, product)` combination, the
default policy — comparing on a subset of columns while every other column on the
frame survives untouched.

```cpp
using namespace pluss::table;

auto orders = DataFrame({
    {"order_id", Slate(pluss::make_series({101, 102, 103, 104, 105, 106}))},
    {"city",     Slate(pluss::make_note({"Accra", "Accra", "Tema", "Accra", "Kumasi", "Tema"}))},
    {"product",  Slate(pluss::make_note({"Laptop", "Laptop", "Phone", "Laptop", "Laptop", "Phone"}))},
});

auto unique_combos = orders.drop_duplicates({"city", "product"});
std::cout << unique_combos;
```

### Output

```text
================================
     | order_id  city    product
--------------------------------
0    | 101       Accra   Laptop 
2    | 103       Tema    Phone  
4    | 105       Kumasi  Laptop 
--------------------------------
[3 cs x 3 feat]
```

Three distinct `(city, product)` combinations exist in the frame; one representative
of each survives, and it's the **first** row encountered for that combination — case
`0` for `(Accra, Laptop)`, case `2` for `(Tema, Phone)`, case `4` for the one-off
`(Kumasi, Laptop)`. Case labels `0`, `2`, and `4` are preserved from the source frame,
not renumbered to `0`, `1`, `2` — and `order_id` survives on every row even though it
was never part of the comparison, because `features` only narrows what's *compared*,
not what's returned.

Example 2. Use `keep_last = true` to keep the *last* occurrence of each combination
instead.

```cpp
auto unique_combos_last = orders.drop_duplicates({"city", "product"}, false, true);
std::cout << unique_combos_last;
```

### Output

```text
================================
     | order_id  city    product
--------------------------------
3    | 104       Accra   Laptop 
4    | 105       Kumasi  Laptop 
5    | 106       Tema    Phone  
--------------------------------
[3 cs x 3 feat]
```

The same three combinations survive, but now it's case `3` representing
`(Accra, Laptop)` and case `5` representing `(Tema, Phone)` — the last occurrence of
each, rather than the first. Rows are still reported in their original case order,
not grouped by combination.

## Notes

- `drop_duplicates()`'s result keeps the original feature set, feature order, and case
  labels of the surviving rows — surviving rows are never renumbered.
- Called with no `features`, every column on the frame is compared, including any
  unique identifier — which means nothing will ever be dropped if such a column is
  present and uncompared. Calling it plain (`orders.drop_duplicates()`) is almost
  always a mistake on a frame with an ID column; pass the subset of columns that
  should define a "duplicate" instead (Example 1).
- Internally, `orders.drop_duplicates(features, keep_first, keep_last)` is exactly
  `orders.filter(!orders.duplicated(features, keep_first, keep_last))` — anything
  [`duplicated()`](duplicated.md) would flag `true` for the compared features is
  excluded, while every other column rides along unchanged on the survivors.
- `keep_first = true` and `keep_last = true` together is a validation error.

## See Also

- [`DataFrame::duplicated`](duplicated.md): the mask this method filters by, useful
  when you want to inspect or count duplicates before deciding whether to drop them.
- [`DataFrame::filter`](filter.md): the general row-selection method this is built on.
- [`DataFrame::ndistinct`](ndistinct.md): counts unique combinations without
  constructing a filtered frame.
- [`DataFrame::features`](features.md): narrow to a subset of columns for other
  reductions in this family — `duplicated()`, `any()`, `all()`, `ndistinct()`,
  `repeated()`, `corr()` — where narrowing first is a direct, lossless replacement for
  an explicit selector. `drop_duplicates()` is the exception: pass `features` to it
  directly instead of narrowing first.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="drop.html">
    <span class="label">Previous</span>
    <span class="title">drop</span>
  </a>
  <a class="next-link" href="dropnull.html">
    <span class="label">Next</span>
    <span class="title">dropnull</span>
  </a>
</div>
