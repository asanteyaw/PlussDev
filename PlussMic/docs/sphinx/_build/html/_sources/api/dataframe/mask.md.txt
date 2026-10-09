<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 5e0c60e3b41f -->

# DataFrame::mask

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signatures

```cpp
DataFrame::mask(const Slate& bool_mask, pluss::RangeView<std::string_view> features = {}) const -> DataFrame
DataFrame::mask(const Slate& bool_mask, const AlignOptions& options, pluss::RangeView<std::string_view> features = {}) const -> DataFrame
DataFrame::mask(const Slate& bool_mask, pluss::RangeView<std::string_view> features, const AlignOptions& options) const -> DataFrame
```

## What It Does

Replaces every cell with `NULL` wherever `bool_mask` is `true`, leaving every other
cell untouched. This is the inverse of [`filter()`](filter.md): `filter()` keeps or
drops entire **rows** based on a mask, while `mask()` blanks out individual **cells**
across the selected features without removing any row, and every feature keeps its
original shape and case count.

## Parameters

**bool_mask** (`const Slate&`)

A boolean mask with one entry per case. `true` nulls the corresponding row's selected
cells; `false` (or a missing mask entry, treated the same as `false`) leaves it alone.

**features** (`pluss::RangeView<std::string_view>`, default `{}`)

Which features `bool_mask` is applied to. Leave empty (the default) to null out the
masked rows across **every** feature on the frame; pass a subset to restrict the
effect to just those columns.

**options** (`const AlignOptions&`, optional)

Controls how `bool_mask` is aligned to the frame's case axis when it doesn't already
match exactly — the same alignment mechanism used throughout the arithmetic and
comparison families (see [`add()`](add.md)).

## Returns

`DataFrame`
    A new frame, same shape as the source, with `NULL` written into every selected
    feature wherever `bool_mask` was `true`.

## Examples

Example 1. Null out every feature on rows where a threshold is crossed.

```cpp
using namespace pluss::table;

auto orders = DataFrame({
    {"score", Slate(pluss::make_series({45.0, 88.0, 30.0}))},
    {"bonus", Slate(pluss::make_series({5.0, 10.0, 2.0}))},
});
std::cout << orders;
```

### Output (before)

```text
===================
     | score  bonus
-------------------
0    | 45     5    
1    | 88     10   
2    | 30     2    
-------------------
[3 cs x 2 feat]
```

```cpp
auto low = orders.features("score") < 50.0;
auto masked = orders.mask(low);
std::cout << masked;
```

### Output (after)

```text
===================
     | score  bonus
-------------------
0    | NULL   NULL 
1    | 88     10   
2    | NULL   NULL 
-------------------
[3 cs x 2 feat]
```

Cases `0` and `2` (both below the `50.0` threshold) are nulled out across **both**
features, since `features` was left at its default (every column). Case `1` (`88`,
above the threshold) is untouched.

Example 2. Restrict the effect to just one feature.

```cpp
auto scoped = orders.mask(low, {"score"});
std::cout << scoped;
```

### Output

```text
===================
     | score  bonus
-------------------
0    | NULL   5    
1    | 88     10   
2    | NULL   2    
-------------------
[3 cs x 2 feat]
```

Only `score` is nulled at cases `0` and `2`; `bonus` keeps its original values at
every case, since it wasn't included in `features`.

## Notes

- `mask()` never changes the frame's shape or case count — it only ever replaces
  values with `NULL`, the same way [`isnull()`](isnull.md) never removes rows either.
- A mask entry that is itself missing behaves like `false`: the corresponding row is
  left alone, not nulled "just in case."
- `mask()` is non-mutating; it always returns a new frame.

## See Also

- [`DataFrame::filter`](filter.md): the row-dropping counterpart — removes rows
  entirely instead of nulling cells in place.
- [`DataFrame::put`](put.md): write a specific replacement value where a condition
  holds, rather than always writing `NULL`.
- [`DataFrame::fillnull`](fillnull.md): the inverse direction — replace `NULL` cells
  with a value instead of creating them.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="map.html">
    <span class="label">Previous</span>
    <span class="title">map</span>
  </a>
  <a class="next-link" href="match.html">
    <span class="label">Next</span>
    <span class="title">match</span>
  </a>
</div>
