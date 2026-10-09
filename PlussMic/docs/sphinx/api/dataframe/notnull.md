<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 4e71b03b0d08 -->

# DataFrame::notnull

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signatures

```cpp
DataFrame::notnull(pluss::RangeView<std::string_view> features = {}) const -> DataFrame

template <detail::StringViewNameRange NameRange>
DataFrame::notnull(const NameRange& features) const -> DataFrame
```

## What It Does

Returns a same-shape frame of booleans marking exactly which cells were **observed**:
`true` where the source cell has a value, `false` where it is null — the exact
complement of [`isnull()`](isnull.md), cell for cell. It exists as its own named method
rather than forcing every caller to write `!isnull()`-style negation, because "which
cells are usable" reads more directly than "which cells are not missing" at most call
sites (building a mask to `filter()` on, for instance).

## Parameters

**features** (`pluss::RangeView<std::string_view>`, default empty)

Feature names to check. Leave empty to check every feature on the frame; pass a subset
to restrict the resulting mask to just those columns. Any container of
`std::string_view`-convertible names (`std::vector<std::string>`, `std::set<std::string>`,
and similar) works too, via a generic template overload — not just a braced list or an
explicit `RangeView`.

## Returns

`DataFrame`
    A frame with the same case axis as the source and one boolean feature per checked
    column, `true` wherever that cell held an observed (non-null) value in the source.

## Examples

Example 1. Build a mask of which `unit_price` values are usable.

```cpp
using namespace pluss::table;

auto orders = DataFrame({
    {"order_id",   Slate(pluss::make_series({101, 102, 103, 104, 105, 106}))},
    {"unit_price", Slate(pluss::make_series({1200.0, 45.5, 1150.0, 320.0, 42.0, std::numeric_limits<double>::quiet_NaN()}))},
});

auto price_observed = orders.notnull({"unit_price"});
std::cout << price_observed;
```

### Output

```text
=================
     | unit_price
-----------------
0    | true      
1    | true      
2    | true      
3    | true      
4    | true      
5    | false     
-----------------
[6 cs x 1 feat]
```

Only case `5` reads `false` — its `unit_price` is the one missing value in this column.
A common next step is feeding this straight into [`filter()`](filter.md):
`orders.filter(orders.notnull({"unit_price"}).features("unit_price"))` keeps only the
rows where `unit_price` is usable, without needing a separate comparison against a
sentinel value.

## Notes

- `notnull()` and `isnull()` are exact complements: for any cell,
  `notnull()` reports `true` exactly where `isnull()` reports `false`.
- Some codebases spell this `notna()`; in PlussMic, `notnull()` is the name, and it does
  not have a separate `notna()` alias — use `notnull()` consistently.
- Like `isnull()`, this is a pure inspection method; it never mutates the frame or
  changes any values.

## See Also

- [`DataFrame::isnull`](isnull.md): the inverted mask (`true` where missing).
- [`DataFrame::dropnull`](dropnull.md): drop rows based on a missingness rule directly,
  without building the mask yourself first.
- [`DataFrame::filter`](filter.md): use a `notnull()`/`isnull()` mask to select rows.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="note.html">
    <span class="label">Previous</span>
    <span class="title">note</span>
  </a>
  <a class="next-link" href="observe.html">
    <span class="label">Next</span>
    <span class="title">observe</span>
  </a>
</div>
