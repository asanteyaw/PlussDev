<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 3e1b392a43a7 -->

# DataFrame::isnull

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signatures

```cpp
DataFrame::isnull(pluss::RangeView<std::string_view> features = {}) const -> DataFrame

template <detail::StringViewNameRange NameRange>
DataFrame::isnull(const NameRange& features) const -> DataFrame
```

## What It Does

Returns a same-shape frame of booleans marking exactly which cells are missing: `true`
where the source cell is null, `false` where a value was observed. This is the
elementwise inspection primitive behind most of the missing-data workflow —
[`fillnull()`](fillnull.md), [`dropnull()`](dropnull.md), and `describe()`'s null-skipping
all answer questions that `isnull()` lets you answer explicitly and visually first.

Leave `features` empty (the default) to check every feature on the frame; pass an
explicit subset when only certain columns matter for a downstream decision (a join key,
say, where nulls elsewhere on the frame are expected and irrelevant).

## Parameters

**features** (`pluss::RangeView<std::string_view>`, default empty)

Feature names to check for missingness. Leave empty to check every feature on the frame;
pass a subset to restrict the resulting mask to just those columns. Any container of
`std::string_view`-convertible names (`std::vector<std::string>`, `std::set<std::string>`,
and similar) works too, via a generic template overload — not just a braced list or an
explicit `RangeView`.

## Returns

`DataFrame`
    A frame with the same case axis as the source and one boolean feature per checked
    column, `true` wherever that cell was null in the source.

## Examples

Example 1. Check every feature on a frame with two independently missing cells.

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

auto mask = orders.isnull();
std::cout << mask;
```

### Output

```text
============================================================
     | order_id  city   product  units  unit_price  discount
------------------------------------------------------------
0    | false     false  false    false  false       false   
1    | false     false  false    false  false       false   
2    | false     false  false    false  false       true    
3    | false     false  false    false  false       false   
4    | false     false  false    false  false       false   
5    | false     false  false    false  true        false   
------------------------------------------------------------
[6 cs x 6 feat]
```

Only two cells in the entire mask are `true`: `discount` at case `2`, and `unit_price` at
case `5` — matching exactly the two nulls that show up in [`info()`](info.md)'s non-null
counts for this frame. Every other feature and case is `false`, including the `Note`
columns (`city`, `product`), which participate in the missingness check even though
they aren't numeric.

Example 2. Restrict the check to one feature you specifically care about.

```cpp
auto price_mask = orders.isnull({"unit_price"});
```

`price_mask` is a one-column frame — only `unit_price`'s missingness, ignoring whether
anything else on the frame is null. This is the shape to reach for when you are about to
make a decision based on one specific column (skip a row in a loop, guard a downstream
calculation) and do not want to accidentally react to nulls elsewhere on the frame.

## Notes

- `isnull()` never mutates the frame or changes which cells are null; it is a pure
  read-only inspection.
- Combine the result with [`any(Axis::Case)`](any.md) to collapse the per-cell mask down
  to one boolean per row (`orders.isnull().any(pluss::table::Axis::Case)`), which is
  exactly how [`info()`](info.md) computes its `Null Case Rows` count.
- `isnull()` and [`notnull()`](notnull.md) are exact complements of each other,
  cell-for-cell.

## See Also

- [`DataFrame::notnull`](notnull.md): the inverted mask (`true` where observed).
- [`DataFrame::fillnull`](fillnull.md): replace nulls with a value instead of just
  locating them.
- [`DataFrame::dropnull`](dropnull.md): drop rows based on a missingness rule instead of
  inspecting cell-by-cell.
- [`DataFrame::filter`](filter.md): a natural next step once you have a mask you want to
  filter rows with.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="invalid.html">
    <span class="label">Previous</span>
    <span class="title">invalid</span>
  </a>
  <a class="next-link" href="join.html">
    <span class="label">Next</span>
    <span class="title">join</span>
  </a>
</div>
