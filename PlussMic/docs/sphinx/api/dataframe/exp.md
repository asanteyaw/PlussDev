<!-- pluss-hand-authored -->

# DataFrame::exp

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signature

```cpp
DataFrame::exp() const -> DataFrame
```

## What It Does

Applies the natural exponential function (`e^x`) elementwise to every numeric feature,
leaving non-numeric features (`Note`, `Timeline`, `TimeIndex`) unchanged — the same
delegation pattern as [`abs()`](abs.md) and [`round()`](round.md). Non-float numeric
features are upcast to `float64` first, since `e^x` is rarely a whole number even for
integer `x`.

## Parameters

`None`

## Returns

`DataFrame`
    A new frame, same shape as the source, with every numeric feature replaced by
    `e` raised to that value. Non-numeric features are left unchanged.

## Examples

Example 1. Exponentiate a numeric feature.

```cpp
using namespace pluss::table;

auto values = DataFrame({
    {"x", pluss::make_series({0.0, 1.0, 2.0})},
}, pluss::make_note({"a", "b", "c"}));

auto out = values.exp();
std::cout << out;
```

### Output

```text
==================
     | x          
------------------
a    | 1          
b    | 2.718281828
c    | 7.389056099
------------------
[3 cs x 1 feat]
```

`e^0 = 1`, `e^1 ≈ 2.718281828`, and `e^2 ≈ 7.389056099` — exactly the inverse of
[`log()`](log.md) applied to these same three results.

## Notes

- Non-numeric features are always left unchanged.
- Integer-typed features are upcast to `float64` before exponentiating; the result is
  always a floating-point `Series`.
- No domain restriction: `exp()` is defined for every finite input. Very large inputs
  overflow to `inf` following ordinary floating-point behavior, rather than throwing.
- `exp()` is the inverse of [`log()`](log.md): `x.log().exp()` reconstructs `x` (up to
  floating-point precision) for every positive value in `x`.

## See Also

- [`DataFrame::log`](log.md): the inverse operation, and where domain restrictions
  (negative/zero inputs) actually apply.
- [`DataFrame::abs`](abs.md), [`DataFrame::round`](round.md): other elementwise
  numeric-only transforms sharing this same non-numeric passthrough behavior.
- [`DataFrame::pow`](pow.md): exponentiate by an arbitrary base/exponent pair instead
  of fixing the base at `e`.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="equals.html">
    <span class="label">Previous</span>
    <span class="title">equals</span>
  </a>
  <a class="next-link" href="extend.html">
    <span class="label">Next</span>
    <span class="title">extend</span>
  </a>
</div>
