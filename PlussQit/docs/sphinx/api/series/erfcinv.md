<!-- pluss-hand-authored -->

# Series::erfcinv

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series Series::erfcinv() const
Series& Series::erfcinv_()
```

## What It Does

Computes the inverse complementary error function elementwise. `erfcinv` is the
functional inverse of [`erfc`](erfc.md) on the open interval `(0, 2)`:

```
erfc( erfcinv(y) ) = y        for y in (0, 2)
```

It is `erfinv` shifted onto the complementary scale:

```
erfcinv(y) = erfinv(1 - y)
```

The reason to reach for `erfcinv` rather than `erfinv(1 - y)` is the far tail.
When a routine works with a small upper-tail probability `y` directly —
p-values, exceedance probabilities, the survival function of a Gaussian —
forming `1 - y` first loses precision, and for `y` below about `1e-16` collapses
to exactly `1`. `erfcinv` keeps the small argument intact and stays accurate
where `erfinv(1 - y)` has already saturated.

Domain and endpoints:

- `erfcinv(1) = 0`.
- `erfcinv` is antisymmetric about `y = 1`: `erfcinv(2 - y) = -erfcinv(y)`.
- As `y -> 0` the value goes to `+inf`; as `y -> 2` it goes to `-inf`.
  `erfcinv(0)` returns `+inf` and `erfcinv(2)` returns `-inf` exactly.
- Outside `[0, 2]` the inverse does not exist and `erfcinv(y)` returns `NaN`.

`erfcinv()` is out-of-place; `erfcinv_()` overwrites the current storage.

## Parameters

None.

## Returns

`Series` or `Series&`
    A `Series` with the same shape as `self`. Interior arguments map to finite
    values, `0` and `2` to `+/-inf`, and `y < 0` or `y > 2` to `NaN`.
    `erfcinv_()` returns a reference to the mutated series.

An integral input is promoted to `float64`.

## Gradient

`erfcinv()` is differentiable on the interior. It is the negative of the
`erfinv` derivative evaluated at the same point, because
`d/dy (1 - y) = -1`:

```
d/dy erfcinv(y) = -sqrt(pi)/2 * exp( erfcinv(y)^2 )
```

The slope is `-sqrt(pi)/2 = -0.8862` at `y = 1` and diverges toward either
endpoint. At `y = 0` or `y = 2` the sensitivity is `-inf`; outside `[0, 2]` it
is `NaN`.

## Examples

Example 1. `erfcinv(1) = 0`; values below `1` are positive, above `1` negative.

```cpp
pluss::series::print(std::cout,
                     pluss::make_series({1.0, 0.5, 1.5, 0.1}).erfcinv());
```

### Output

```text
Series(shape=[4], strides=[1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[0.0000, 0.4769, -0.4769, 1.1631]
```

Example 2. The identity `erfcinv(y) = erfinv(1 - y)`, checked directly.

```cpp
pluss::series::print(std::cout, pluss::make_series({0.3, 0.8, 1.4}).erfcinv());
pluss::series::print(std::cout, pluss::make_series({0.7, 0.2, -0.4}).erfinv());
```

### Output

```text
Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[0.7329, 0.1791, -0.3708]
Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[0.7329, 0.1791, -0.3708]
```

Example 3. Endpoints and out-of-domain arguments. `0` and `2` give `+/-inf`;
anything outside `[0, 2]` gives `NaN`.

```cpp
std::cout << pluss::make_series({0.0, 2.0, -0.5, 2.5}).erfcinv();
```

### Output

```text
pluss::Series(shape=[4], strides=[1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[inf, -inf, nan, nan]
```

Example 4. The backward pass is `-sqrt(pi)/2 * exp(erfcinv(y)^2)`. Because the
factor depends on `erfcinv(y)^2`, arguments symmetric about `1` share a
magnitude.

```cpp
auto y = pluss::make_series({0.5, 1.0, 1.5});
y.tracks_slope_(true);
y.erfcinv().sum().reverse();

pluss::series::print(std::cout, y.sensitivity());
```

### Output

```text
Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[-1.1126, -0.8862, -1.1126]
```

Example 5. `erfcinv_()` overwrites the current storage in place.

```cpp
auto a = pluss::make_series({1.0, 0.5, 2.0});
a.erfcinv_();

pluss::series::print(std::cout, a);
```

### Output

```text
Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[0.0000, 0.4769, -inf]
```

## Gotchas And Failure Modes

- The domain is `[0, 2]`, not `[-1, 1]`. It is a *complementary*-scale inverse:
  the argument is a tail mass, not a signed value.
- `y < 0` or `y > 2` returns `NaN`; `0` and `2` return `+/-inf`.
- The whole reason to use `erfcinv` over `erfinv(1 - y)` is precision for tiny
  `y`. If you have already formed `1 - y`, that precision is gone — pass the
  small quantity straight in.
- The gradient diverges toward both endpoints.
- Integral inputs promote to `float64`.

## Summary

`erfcinv()` inverts [`erfc`](erfc.md) on `(0, 2)`:

- `erfcinv(y) = erfinv(1 - y)`, but accurate for small `y`
- `erfcinv(1) = 0`, `erfcinv(0) = +inf`, `erfcinv(2) = -inf`
- `y` outside `[0, 2]` returns `NaN`
- differentiable on the interior with backward
  `-sqrt(pi)/2 * exp(erfcinv(y)^2)`
- `erfcinv_()` is the in-place form

## See Also

- [`Series::erfc`](erfc.md): the function this inverts.
- [`Series::erfinv`](erfinv.md): the inverse on the `erf` scale.
- [`Series::erf`](erf.md): the error function.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="erfc.html">
    <span class="label">Previous</span>
    <span class="title">erfc</span>
  </a>
  <a class="next-link" href="erfinv.html">
    <span class="label">Next</span>
    <span class="title">erfinv</span>
  </a>
</div>
