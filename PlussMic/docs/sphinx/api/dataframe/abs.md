<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: e90840dde82e -->

# DataFrame::abs

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signature

```cpp
DataFrame DataFrame::abs() const
```

## What It Does

Replaces every numeric feature with its absolute value, cell by cell. A negative value
becomes its positive counterpart, a positive value is left as-is, and a null cell stays
null — `abs()` only ever changes the sign of an observed number, never invents a value
for a missing one.

## Parameters

`None`

## Returns

`DataFrame`
    A new frame, same shape as the source, with every numeric feature replaced by its
    absolute value. Non-numeric features (`Note`, `Timeline`, `TimeIndex`) are left
    exactly as they were.

## Examples

Example 1. Take the absolute value of a signed numeric feature.

```cpp
using namespace pluss::table;

auto readings = DataFrame({
    {"delta", pluss::make_series({-5.0, 3.0, std::numeric_limits<double>::quiet_NaN(), -2.5})},
    {"city",  pluss::make_note({"Accra", "Tema", "Kumasi", "Ho"})},
});

std::cout << readings;
```

### Output (before)

```text
====================
     | delta  city  
--------------------
0    | -5     Accra 
1    | 3      Tema  
2    | NULL   Kumasi
3    | -2.5   Ho    
--------------------
[4 cs x 2 feat]
```

```cpp
auto magnitudes = readings.abs();
std::cout << magnitudes;
```

### Output (after)

```text
====================
     | delta  city  
--------------------
0    | 5      Accra 
1    | 3      Tema  
2    | NULL   Kumasi
3    | 2.5    Ho    
--------------------
[4 cs x 2 feat]
```

Both negative cells (`-5` at case `0`, `-2.5` at case `3`) flipped sign; the already-positive
`3` at case `1` is untouched. The `NULL` at case `2` stayed `NULL` — `abs()` never fills
a missing value — and `city`, a `Note` feature, was never a candidate for the operation.

## Notes

- `abs()` applies to **every** numeric feature on the frame at once; there is no
  per-feature selector. Apply it to one feature at a time via
  [`assign()`](assign.md) if only some columns should change.
- Non-numeric features pass through completely unchanged.
- Nulls pass through completely unchanged — `abs()` is not a substitute for
  [`fillnull()`](fillnull.md).

## See Also

- [`DataFrame::round`](round.md): a different elementwise numeric cleanup, rounding
  instead of taking a magnitude.
- [`DataFrame::clip`](clip.md): bound values into a range instead of flipping their sign.
- [`DataFrame::replace`](replace.md): swap specific values rather than transforming every
  cell the same way.

<div class="pluss-inline-prev-next">
  <a class="next-link" href="add.html">
    <span class="label">Next</span>
    <span class="title">add</span>
  </a>
</div>
