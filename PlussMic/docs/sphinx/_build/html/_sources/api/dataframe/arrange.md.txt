<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: b17db301675f -->

# DataFrame::arrange

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signatures

```cpp
DataFrame::arrange(pluss::RangeView<DataFrame::ArrangeSpec> specs) const -> DataFrame

DataFrame::arrange(pluss::RangeView<std::string_view> features,
                   bool ascending = true,
                   bool nulls_last = true) const -> DataFrame

template <detail::StringViewNameRange NameRange>
DataFrame::arrange(const NameRange& features,
                   bool ascending = true,
                   bool nulls_last = true) const -> DataFrame
```

## What It Does

Reorders the cases of a frame using one or more feature values as sort keys, and returns
the reordered frame. Every feature is carried along unchanged; only case order moves.

Two overloads cover the common shapes:

- The simple overload accepts feature names plus one shared direction and null-placement
  policy.
- The `ArrangeSpec` overload accepts per-feature direction and null-placement settings.

With more than one key, `arrange()` uses ordinary lexicographic / tuple ordering:
the first feature is primary, the second only breaks ties within equal first-feature
values, the third only breaks remaining ties, and so on.

## Parameters

**specs** (`pluss::RangeView<ArrangeSpec>`)

An ordered collection of per-feature sort specifications. Earlier specs take priority
over later ones, the same way a SQL `ORDER BY a DESC, b ASC` works.

**features** (`pluss::RangeView<std::string_view>`)

Feature names to sort by, in priority order, for the simple overload. Any container of
`std::string_view`-convertible names (`std::vector<std::string>`, `std::set<std::string>`,
and similar) works too, via a generic template overload — not just a braced list or an
explicit `RangeView`.

**ascending** (`bool`, default `true`)

Applies to every key in the simple overload. `true` sorts smallest or earliest values
first; `false` reverses the order.

**nulls_last** (`bool`, default `true`)

Applies to every key in the simple overload. `true` places null-key rows after observed
values; `false` places them first.

## Returns

`DataFrame`
    A new frame with the same features and dtypes, reordered by the requested sort
    keys. Case labels travel with their row; only row position changes.

## Examples

Example 1. Sort by a single numeric feature, descending, with missing prices at the end.

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

auto by_price = orders.arrange({"unit_price"}, /*ascending=*/false, /*nulls_last=*/true);
std::cout << by_price;
```

### Output

```text
================================================================
     | order_id  city    product     units  unit_price  discount
----------------------------------------------------------------
0    | 101       Accra   Laptop      2      1200        0.1
2    | 103       Tema    Laptop      1      1150        NULL
3    | 104       Tema    Monitor     3      320         0.08
1    | 102       Accra   Headphones  5      45.5        0.05
4    | 105       Kumasi  Headphones  4      42          0.05
5    | 106       Kumasi  Monitor     2      NULL        0.12
----------------------------------------------------------------
[6 cs x 6 feat]
```

The leftmost case-label column now reads `0, 2, 3, 1, 4, 5` instead of `0, 1, 2, 3, 4,
5`. `arrange()` reordered rows; it did not relabel them.

Example 2. Sort by two keys with independent directions using `ArrangeSpec`.

```cpp
auto by_city_then_price = orders.arrange({
    ArrangeSpec{.feature = "city", .ascending = true},
    ArrangeSpec{.feature = "unit_price", .ascending = false, .nulls_last = true},
});
```
### Output

```text
================================================================
     | order_id  city    product     units  unit_price  discount
----------------------------------------------------------------
0    | 101       Accra   Laptop      2      1200        0.1     
1    | 102       Accra   Headphones  5      45.5        0.05    
4    | 105       Kumasi  Headphones  4      42          0.05    
5    | 106       Kumasi  Monitor     2      NULL        0.12    
2    | 103       Tema    Laptop      1      1150        NULL     
3    | 104       Tema    Monitor     3      320         0.08    
----------------------------------------------------------------
[6 cs x 6 feat]
```
Rows are grouped by `city` alphabetically first, and within each city the higher
`unit_price` rows come first.

Example 3. The simple overload applies one shared direction to every key.

```cpp
auto uniform_desc = orders.arrange({"city", "unit_price"}, /*ascending=*/false, /*nulls_last=*/true);
std::cout << uniform_desc;
```

### Output

```text
================================================================
     | order_id  city    product     units  unit_price  discount
----------------------------------------------------------------
3    | 104       Tema    Monitor     3      320         0.08
2    | 103       Tema    Laptop      1      1150        NULL
5    | 106       Kumasi  Monitor     2      NULL        0.12
4    | 105       Kumasi  Headphones  4      42          0.05
1    | 102       Accra   Headphones  5      45.5        0.05
0    | 101       Accra   Laptop      2      1200        0.1
----------------------------------------------------------------
[6 cs x 6 feat]
```

Because this is the simple overload, both keys inherit the same `ascending = false`
setting. That means `city` sorts descending (`Tema`, then `Kumasi`, then `Accra`),
and `unit_price` sorts descending inside each city as the tie-break key. If you want
mixed directions like "city ascending, price descending", you must use the
`ArrangeSpec` overload from Example 2.

## Notes

- `arrange()` is the general row-ordering surface for `DataFrame`.
- `top()` and `bottom()` are shorthand for `arrange(...).head(...)`.
- `arrange()` does not deduplicate ties; equal-key rows keep their stable relative order.
- When you pass several feature names plus one `ascending` flag, that direction is
  shared by every key. Per-key direction requires `ArrangeSpec`.
- If you need to sort case labels rather than feature values, use [`label().sort()`](label.md).
