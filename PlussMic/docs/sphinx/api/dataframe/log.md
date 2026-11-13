<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 55095b828ba9 -->

# DataFrame::log

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signature

```cpp
DataFrame DataFrame::log() const
```

## What It Does

Applies the natural logarithm (`ln`) elementwise to every numeric feature, leaving
non-numeric features (`Note`, `Timeline`, `TimeIndex`) unchanged — the same
delegation pattern as [`abs()`](abs.md) and [`exp()`](exp.md). Non-float numeric
features are upcast to `float64` first.

```{caution} No domain check — negative or zero inputs produce NaN/-inf, not an error
`log()` does not validate its input's sign before computing. `log(0)` produces `-inf`
and `log(negative)` produces `NaN`, following ordinary floating-point semantics — the
cell's validity mask is untouched, so a `NaN` payload from an out-of-domain input
still reports as a *valid* (non-`NULL`) cell. If you need to guard against this,
check with [`gt()`](gt.md)/[`ge()`](ge.md) before calling `log()`, rather than relying
on the result to flag the problem for you.
```

## Parameters

`None`

## Returns

`DataFrame`
    A new frame, same shape as the source, with every numeric feature replaced by its
    natural logarithm. Non-numeric features are left unchanged.

## Examples

Example 1. Take the natural log of a numeric feature.

```cpp
using namespace pluss::table;

auto values = DataFrame({
    {"x", pluss::make_series({1.0, 2.718281828459045, 7.38905609893065})},
}, pluss::make_note({"a", "b", "c"}));

auto out = values.log();
std::cout << out;
```

### Output

```text
===========
     | x   
-----------
a    | 0   
b    | 1   
c    | 2   
-----------
[3 cs x 1 feat]
```

`ln(1) = 0`, `ln(e) = 1`, and `ln(e^2) = 2` — the input values here are `1`, `e`, and
`e^2`, chosen so the result comes out to clean whole numbers.

## Notes

- Non-numeric features are always left unchanged.
- Integer-typed features are upcast to `float64` before taking the log; the result is
  always a floating-point `Series`.
- `log(0)` yields `-inf` and `log(negative)` yields `NaN` — neither is converted to a
  missing (`NULL`) cell. A pre-existing `NULL` cell in the input stays `NULL` in the
  output, but `log()` never introduces new nulls on its own.
- `log()` is the inverse of [`exp()`](exp.md): `x.exp().log()` reconstructs `x` (up to
  floating-point precision) for every input.

## See Also

- [`DataFrame::exp`](exp.md): the inverse operation.
- [`DataFrame::abs`](abs.md), [`DataFrame::round`](round.md): other elementwise
  numeric-only transforms sharing this same non-numeric passthrough behavior.
- [`DataFrame::gt`](gt.md) / [`DataFrame::ge`](ge.md): guard against non-positive
  inputs before calling `log()`, since `log()` itself won't flag them.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="load_txt.html">
    <span class="label">Previous</span>
    <span class="title">load_txt</span>
  </a>
  <a class="next-link" href="lor.html">
    <span class="label">Next</span>
    <span class="title">lor</span>
  </a>
</div>
