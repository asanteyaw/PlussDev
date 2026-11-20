<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 557e1b4f6b7f -->

# Shelf::isfinite

[Back to Shelf API Reference](../../shelf_api_reference)

## Signature

```cpp
Shelf Shelf::isfinite() const
```

## What It Does

Returns a `Bool`-typed `Series` shelf, same length as `*this`, marking `true`
wherever the value is both a real finite number (not `NaN`, not `±Inf`) **and**
valid (not `NULL`). Requires the shelf to be `Series`-kind. Unlike
[`isnan()`](isnan.md), which checks the numeric value alone, `isfinite()` combines
the numeric check with the validity mask — a masked-`NULL` cell is `false` here even
if its underlying stored value happens to be an ordinary finite number.

## Parameters

`None`

## Returns

`Shelf`
    A new `Bool`-typed `Series` shelf, same length as `*this`.

## Examples

Example 1. `isfinite()` is `false` for `NaN`, `Inf`, and masked-`NULL` cells alike —
only a genuinely valid, finite value is `true`.

```cpp
using namespace pluss::table;

auto readings = Shelf(
    pluss::make_series({1.0, std::nan(""), std::numeric_limits<double>::infinity(), 4.0}),
    pluss::make_series(std::vector<bool>{true, true, true, false}));
std::cout << readings.isfinite();
```

### Output

```text
0  true
1  false
2  false
3  false

ptype: Series
```

Position `1` (`NaN`) and position `2` (`Inf`) are both `false` because they aren't
finite numbers; position `3` is `false` because it's masked `NULL`, even though its
underlying stored value (`4.0`) is an ordinary finite number. Only position `0` is
genuinely finite *and* valid.

Example 2. A non-`Series` shelf throws.

```cpp
auto notes = Shelf(pluss::make_note({"a", "b"}));
try {
  auto bad = notes.isfinite();
} catch (const pluss::Error& e) {
  std::cout << e.message();
}
```

### Output

```text
Shelf::isfinite is only valid for Series payload
```

## Notes

- `isfinite` is `false` for `NaN`, for both infinities and for a missing cell, and `true` for every ordinary number; it is the complement of [`invalid`](invalid.md). See [Missing, NaN and infinity](../../guide/missing_nan_and_inf.md).
- Requires the calling shelf to be `Series`-kind.
- Combines a numeric finiteness check with the validity mask — `false` for `NaN`,
  `±Inf`, *and* masked-`NULL` cells.
- The exact opposite question `NaN`-only checks like [`isnan()`](isnan.md) answer:
  `isfinite()` folds in validity too, `isnan()` doesn't.

## See Also

- [`Shelf::isnan`](isnan.md): the numeric-value-only check, without folding in the
  validity mask.
- [`Shelf::isnull`](isnull.md): the validity-mask-only check, without inspecting
  the numeric value.
- [`Shelf::invalid`](invalid.md): the logical inverse of this method (for
  `Series`-kind shelves).

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="invalid.html">
    <span class="label">Previous</span>
    <span class="title">invalid</span>
  </a>
  <a class="next-link" href="isinf.html">
    <span class="label">Next</span>
    <span class="title">isinf</span>
  </a>
</div>
