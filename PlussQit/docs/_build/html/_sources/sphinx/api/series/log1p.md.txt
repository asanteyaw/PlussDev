<!-- pluss-hand-authored -->

# Series::log1p

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series::log1p() const -> Series
Series& Series::log1p_()
```

## What It Does

`log1p()` applies `log(1 + x)` elementwise.

That one added `1` matters conceptually. The method is not the same thing as
plain [`log()`](log.md): it shifts the domain and changes the slope.

For real-valued use, the expression lives on the real line only when `1 + x` is
positive, which is why values near `-1` deserve attention.

## Parameters

None.

## Returns

`Series`
    A new `Series` with the same shape as `self`, containing `log(1 + x)` lane
    by lane.

## Gradient

If `y = log(1 + x)`, then:

- `dy/dx = 1 / (1 + x)`

So with the scalar objective `L = sum(y)`, each lane's contribution is the
reciprocal of `1 + x_i`.

## In-Place Form

`log1p_()` is the in-place sibling of [`log1p()`](log1p.md): it computes the same
result and writes it back into `self` directly — mutating it (and any other
handle sharing the same storage) instead of allocating a new `Series` — then
returns a reference to `self`.

```cpp
auto x = pluss::make_series({0.0, 1.0});

auto& ref = x.log1p_();
std::cout << "x=" << x << "\n";
std::cout << std::boolalpha << "same object: " << (&ref == &x);
```

### Output

```text
x=pluss::Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[0, 0.69314718]
same object: true
```

## Examples

Example 1. The forward values reflect `log(1 + x)`, not `log(x)`.

```cpp
std::cout << pluss::make_series({-0.5, 0.0, 3.0}).log1p() << "\n";
```

### Output

```text
pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[-0.69314718, 0, 1.3862944]
```

Example 2. The reverse pass uses `1 / (1 + x)` lane by lane.

```cpp
auto x = pluss::make_series({0.0, 1.0, 3.0});
x.tracks_slope_(true);

auto y = x.log1p();
y.sum().reverse();

std::cout << "out=" << y << "\n";
std::cout << "dL_dx=" << x.sensitivity() << "\n";
```

### Output

```text
out=pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[0, 0.69314718, 1.3862944]
dL_dx=pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[1, 0.5, 0.25]
```

## Gotchas And Failure Modes

- `log1p()` is not interchangeable with `log()`.
- The slope steepens as `x` approaches `-1` from the right because
  `1 / (1 + x)` grows there.
- This is an elementwise transform only; there are no axis or keepdim toggles.

## Summary

Use `log1p()` when the natural logarithm should be applied after shifting the
input by one.

## See Also

- [`Series::log`](log.md): plain natural logarithm.
- [`Series::expm1`](expm1.md): exponential-minus-one counterpart.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="log10.html">
    <span class="label">Previous</span>
    <span class="title">log10</span>
  </a>
  <a class="next-link" href="log2.html">
    <span class="label">Next</span>
    <span class="title">log2</span>
  </a>
</div>
