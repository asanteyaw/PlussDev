<!-- pluss-hand-authored -->

# Series::abs

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series::abs() const -> Series
```

## What It Does

Returns the elementwise absolute value.

`abs()` keeps each element's magnitude and discards its sign. Negative values
become positive, positive values stay positive, and zero stays zero. The shape
does not change. This is one of the most common unary cleanup transforms when
you want "distance from zero" rather than signed direction.

Because `abs()` is unary, there is no broadcasting question here. Everything is
about how the existing values are transformed in place conceptually, while the
method itself still returns a fresh `Series`.

## Parameters

None.

This is a zero-argument transform on `self`.

## Returns

`Series`
    A new `Series` with the same shape as `self`, where each element has been
    replaced by its magnitude.

## Gradient

Away from zero, `abs()` behaves the way you expect:

- negative inputs contribute a gradient of `-1`
- positive inputs contribute a gradient of `1`

That makes `abs()` a useful example for value-vs-gradient distinction:
the forward values are all non-negative, but the backward sensitivity still
remembers which side of zero each input came from.

## Examples

Example 1. Basic elementwise magnitudes on a 2D `Series`.

```cpp
auto base = pluss::make_series({
    {-3.0, -2.0, -1.0},
    {0.0, 1.0, 4.0},
});

pluss::series::print(std::cout, base.abs());
```

### Output

```text
Series(shape=[2,3], strides=[3,1], dtype=float64, platform=cpu, numel=6, contiguous=true)
data=
⎡ 3.0000 2.0000 1.0000 ⎤
⎣ 0.0000 1.0000 4.0000 ⎦
```

Example 2. Integer inputs stay integer when no promotion is needed.

```cpp
std::cout << pluss::make_series({-3, 0, 7}, pluss::ScalarType::Int32).abs();
```

### Output

```text
pluss::Series(shape=[3], strides=[1], dtype=int32, platform=cpu, numel=3, contiguous=true)
data=[3, 0, 7]
```

Example 3. `abs()` also works on non-contiguous views; the returned value here
is a regular contiguous result.

```cpp
std::cout << base.transpose(0, 1).abs();
```

### Output

```text
pluss::Series(shape=[3,2], strides=[2,1], dtype=float64, platform=cpu, numel=6, contiguous=true)
data=[[3, 0],
  [2, 1],
  [1, 4]]
```

Example 4. Reverse-mode sensitivity reflects the original sign of each input.

```cpp
auto x = pluss::make_series({-3.0, -2.0, 4.0});
x.tracks_slope_(true);

auto out = x.abs();
out.sum().reverse();

std::cout << "out=" << out << "\n";
std::cout << "dx=" << x.sensitivity();
```

### Output

```text
out=pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[3, 2, 4]
dx=pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[-1, -1, 1]
```

## Gotchas And Failure Modes

- `abs()` removes sign information from the values. If sign matters later, keep
  the original `Series` or compute [`Series::sign`](sign.md) separately.
- The output shape is unchanged, but a view input can still yield a fresh
  contiguous result value.
- Near zero, the value is well-defined, but the "which side of zero"
  intuition that explains the gradient examples no longer applies as cleanly.

## Summary

Use `abs()` when the quantity you care about is magnitude rather than
direction. The simplest mental model is "same shape, same dtype family when
possible, all values reflected onto the non-negative side."

## See Also

- [`Series::neg`](neg.md): flips sign instead of discarding it.
- [`Series::sign`](sign.md): keeps direction and discards magnitude.
- [`Series::sqrt`](sqrt.md): another unary transform on non-negative values.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="../../series_api_reference.html">
    <span class="label">Previous</span>
    <span class="title">Series API Reference</span>
  </a>
  <a class="next-link" href="acos.html">
    <span class="label">Next</span>
    <span class="title">acos</span>
  </a>
</div>
