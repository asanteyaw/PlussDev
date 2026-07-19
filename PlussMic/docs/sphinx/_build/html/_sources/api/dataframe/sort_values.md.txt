<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: cfd259b0a4f6 -->

# DataFrame::sort_values

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signatures

```cpp
DataFrame::sort_values(pluss::RangeView<ArrangeSpec> specs) const -> DataFrame

DataFrame::sort_values(pluss::RangeView<std::string_view> features,
                        bool ascending = true,
                        bool nulls_last = true) const -> DataFrame
```

## What It Does

Reorders the cases of a frame using one or more feature values as sort keys, and returns
the reordered frame. Every feature is carried along unchanged — only case order moves.

Two overloads cover the two shapes this need usually takes:

- The **simple overload** (`features`, `ascending`, `nulls_last`) sorts by a list of
  feature names with one shared direction and one shared null-placement policy applied
  to every key. Reach for this when every sort key should behave the same way, for
  example "sort by `city` then `product`, both ascending."
- The **`ArrangeSpec` overload** sorts by an explicit list of per-column specs, each of
  which can set its own direction and null-placement independently. Reach for this when
  different keys need different treatment, for example "`unit_price` descending, but
  `city` ascending, with nulls always sorted last regardless of key."

## Parameters

**specs** (`pluss::RangeView<ArrangeSpec>`)

An ordered collection of per-feature sort specifications. Earlier specs take priority
over later ones — ties on the first spec's feature are broken by the second spec, and so
on, the same way a SQL `ORDER BY a DESC, b ASC` works. Each `ArrangeSpec` typically
carries a `feature` name plus its own `ascending` and `nulls_last` flags (see the
`ArrangeSpec` aggregate initializer in the example below).

**features** (`pluss::RangeView<std::string_view>`)

Feature names to sort by, in priority order, for the simple overload. All of them share
the single `ascending` / `nulls_last` policy passed alongside them.

**ascending** (`bool`, default `true`)

Applies to every key in the simple overload. `true` sorts smallest/earliest values
first; `false` reverses the order, largest/latest first.

**nulls_last** (`bool`, default `true`)

Applies to every key in the simple overload. `true` places rows with a null in the sort
key after every row with an observed value for that key; `false` places null rows first
instead.

## Returns

`DataFrame`
    A new frame with the same features and dtypes, reordered by the requested sort
    keys. Case labels travel with their row, so a row's case label after sorting is
    still whatever it was on the source frame — only the row's position changes.

## Examples

Example 1. Sort by a single numeric feature, descending, with missing prices sorted to
the end.

```cpp
using namespace pluss::table;

auto orders = DataFrame({
    {"order_id",   Thread(pluss::make_series({101, 102, 103, 104, 105, 106}))},
    {"city",       Thread(pluss::make_note({"Accra", "Accra", "Tema", "Tema", "Kumasi", "Kumasi"}))},
    {"product",    Thread(pluss::make_note({"Laptop", "Headphones", "Laptop", "Monitor", "Headphones", "Monitor"}))},
    {"units",      Thread(pluss::make_series({2, 5, 1, 3, 4, 2}))},
    {"unit_price", Thread(pluss::make_series({1200.0, 45.5, 1150.0, 320.0, 42.0, std::numeric_limits<double>::quiet_NaN()}))},
    {"discount",   Thread(pluss::make_series({0.1, 0.05, std::numeric_limits<double>::quiet_NaN(), 0.08, 0.05, 0.12}))},
});

auto by_price = orders.sort_values({"unit_price"}, /*ascending=*/false, /*nulls_last=*/true);
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

Look at the leftmost, blank-header case-index column: it now reads `0, 2, 3, 1, 4, 5`
instead of `0, 1, 2, 3, 4, 5`. That column is the original case label traveling with each
row — `sort_values()` reordered the rows, it did not relabel or renumber them. Case `5`
(the missing `unit_price`) sorted to the very end even in descending order, because
`nulls_last=true` places null keys after every observed value regardless of sort
direction.

Example 2. Sort by two keys with independent directions using `ArrangeSpec`.

```cpp
auto by_city_then_price = orders.sort_values({
    ArrangeSpec{.feature = "city", .ascending = true},
    ArrangeSpec{.feature = "unit_price", .ascending = false, .nulls_last = true},
});
```

Rows are grouped by `city` alphabetically first (`Accra`, `Kumasi`, `Tema`), and within
each city group, ordered by `unit_price` from highest to lowest. This is the shape to
reach for whenever a "sort ascending" and a "sort descending" need to happen in the same
call — the simple `features` overload cannot express mixed directions across keys.

## Notes

- `sort_values()` is a stable-feeling composition point: it reads naturally as
  `orders.sort_values(...).head(3)` for "top 3 by X," or
  `orders.filter(mask).sort_values(...)` for "sort only the rows that matched."
- The simple overload's `ascending` / `nulls_last` apply identically to *every* feature
  in `features`. If even one key needs a different policy, use the `ArrangeSpec`
  overload instead of trying to work around the shared-policy limitation.
- `sort_values()` does not deduplicate ties — rows with equal sort-key values keep their
  relative source order (a stable sort), which matters when you chain multiple keys or
  rely on tie order downstream.

## See Also

- [`DataFrame::rank`](rank.md): compute each row's rank within a feature instead of
  reordering the frame.
- [`DataFrame::head`](head.md) / [`DataFrame::tail`](tail.md): commonly chained after
  `sort_values()` to get a "top N" or "bottom N" slice.
- [`DataFrame::query`](query.md) / [`DataFrame::filter`](filter.md): narrow down rows
  before or after sorting.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="slice.html">
    <span class="label">Previous</span>
    <span class="title">slice</span>
  </a>
  <a class="next-link" href="sql.html">
    <span class="label">Next</span>
    <span class="title">sql</span>
  </a>
</div>
