<!-- pluss-hand-authored -->

# DataFrame::invalid

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signature

```cpp
DataFrame::invalid(pluss::RangeView<std::string_view> features = {}) const -> DataFrame
```

## What It Does

A same-shape boolean mask, like [`isnull()`](isnull.md), but broader for numeric
`Series` features: a cell is `true` if it is either genuinely missing (masked `NULL`,
the only thing `isnull()` catches) **or** a non-finite floating-point value — `NaN` or
`+/-Infinity` — even when that value carries no validity mask at all. For every
non-`Series` feature kind (`Note`, `Timeline`, `TimeIndex`), `invalid()` is identical
to `isnull()` — there is no equivalent "non-finite" concept for those kinds.

This distinction matters in practice: a raw `NaN` sitting in a `Series` column that was
never explicitly masked as missing (for example, produced by a `0.0 / 0.0` upstream) is
invisible to `isnull()`, but is caught by `invalid()`.

## Parameters

**features** (`pluss::RangeView<std::string_view>`, default `{}`)

Feature names to check. Leave empty (the default) to check every feature on the frame;
pass a subset to restrict the result to just those columns.

## Returns

`DataFrame`
    A frame with the same case axis as the source and one boolean feature per checked
    column: `true` where that cell is either masked-`NULL` or (for a `Series` feature)
    non-finite; `false` elsewhere.

## Examples

Example 1. Contrast `invalid()` against `isnull()` on a frame with both an unmasked
`NaN` and a genuinely masked null.

```cpp
using namespace pluss::table;

auto readings = DataFrame({
    {"a", Thread(pluss::make_series({1.0, std::numeric_limits<double>::quiet_NaN(), 3.0, 0.0}),
                pluss::make_series({true, true, true, false}, pluss::ScalarType::Bool))},
    {"b", Thread(pluss::make_note({"x", "y", "", "z"}),
                pluss::make_series({true, true, false, true}, pluss::ScalarType::Bool))},
});
std::cout << readings;
```

### Output

```text
=================
     | a     b   
-----------------
0    | 1     x   
1    | nan   y   
2    | 3     NULL
3    | NULL  z   
-----------------
[4 cs x 2 feat]
```

```cpp
std::cout << readings.isnull();
```

### Output

```text
===================
     | a      b    
-------------------
0    | false  false
1    | false  false
2    | false  true 
3    | true   false
-------------------
[4 cs x 2 feat]
```

```cpp
std::cout << readings.invalid();
```

### Output

```text
===================
     | a      b    
-------------------
0    | false  false
1    | true   false
2    | false  true 
3    | true   false
-------------------
[4 cs x 2 feat]
```

`b` (a `Note` feature) is identical between the two masks — case `2`'s masked-empty
cell is the only invalid entry either way. `a` (a `Series` feature) differs at case
`1`: the raw `NaN` there was never masked as missing, so `isnull()` reports `false`,
but `invalid()` reports `true` — it treats a non-finite float as invalid regardless of
whether a validity mask was ever attached to that cell.

## Notes

- For any non-`Series` feature, `invalid()` and `isnull()` produce identical results —
  the "non-finite" extension only applies to numeric `Series` data, since concepts like
  `NaN` and `Infinity` are specific to floating-point representation.
- A masked-`NULL` `Series` cell is always `true` under `invalid()` too, whatever its
  underlying (masked-over) raw bits happen to be — being masked is sufficient on its
  own, independent of the finiteness check.
- Use `invalid()` instead of `isnull()` whenever upstream numeric computation (division,
  logarithms, and similar) could have produced `NaN` or `Infinity` without an explicit
  validity mask ever being attached.

## See Also

- [`DataFrame::isnull`](isnull.md): the narrower mask — masked-`NULL` only, with no
  finiteness check on `Series` features.
- [`DataFrame::notnull`](notnull.md): the inverse of `isnull()` specifically (not the
  inverse of `invalid()`).
- [`DataFrame::fillnull`](fillnull.md) / [`DataFrame::dropnull`](dropnull.md): act on
  missing cells once you've located them.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="interp.html">
    <span class="label">Previous</span>
    <span class="title">interp</span>
  </a>
  <a class="next-link" href="isnull.html">
    <span class="label">Next</span>
    <span class="title">isnull</span>
  </a>
</div>
