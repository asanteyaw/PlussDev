<!-- pluss-hand-authored -->

# Series::dst

[Back to Series API Reference](../../series_api_reference)

## Signatures

```cpp
Series Series::dst() const
Series Series::dst(std::int64_t dim) const
```

## Resolve A Signal Into Sine Components

`dst()` applies an unnormalized type-II discrete sine transform along one
axis. For an axis of length `N`,

```text
Y[k] = sum from t=0 to N-1 of
       x[t] * sin(pi * (t + 1/2) * (k + 1) / N)
```

Unlike the cosine transform, there is no constant basis row. Every coefficient
measures a sine pattern, including the final alternating pattern. This makes
the transform appropriate when an odd continuation, and therefore a zero-like
boundary condition, describes the sampled quantity better than an even
continuation.

The no-argument form uses the last axis. `dst(dim)` transforms the selected
axis, accepts negative axes, and independently processes every slice over the
remaining dimensions. Shape and rank are preserved.

Computation uses real `float64` on CPU. Integral and Boolean input promote to
`float64`; complex input is rejected. Results return to the receiver's
platform, with `float32` storage on the current MPS route. Power-of-two `2N`
lengths use the batched fast transform path and other lengths use the same
formula through a direct vectorized kernel.

`idst()` is the matching inverse. The highest sine basis row has twice the
squared norm of the other rows, so the inverse gives that coefficient half
their weight. That distinction matters for odd as well as even lengths and is
handled internally; callers should not pre-scale the final coefficient.

## Reverse Mode

`dst()` is linear. Its reverse rule applies the transpose of the sine matrix to
the incoming sensitivity. It preserves the receiver's shape and accumulates
into tracked sources. The transpose rule is not a request to call the
normalized public inverse on the sensitivity.

## Examples

Example 1. A one-dimensional signal produces one sine coefficient per sample.

```cpp
pluss::series::print(
    std::cout,
    pluss::make_series({1.0, 2.0, 3.0, 4.0}).dst());
```

### Output

```text
Series(shape=[4], strides=[1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[6.5328, -2.8284, 2.7060, -2.0000]
```

Example 2. The default transforms the last axis of every row.

```cpp
auto x = pluss::make_series({{1.0, 2.0, 3.0}, {4.0, 5.0, 6.0}});
pluss::series::print(std::cout, x.dst());
```

### Output

```text
Series(shape=[2,3], strides=[3,1], dtype=float64, platform=cpu, numel=6, contiguous=true)
data=
⎡  4.0000 -1.7321 2.0000 ⎤
⎣ 10.0000 -1.7321 5.0000 ⎦
```

Example 3. Selecting `dim=0` transforms each column across the two rows.

```cpp
auto x = pluss::make_series({{1.0, 2.0, 3.0}, {4.0, 5.0, 6.0}});
pluss::series::print(std::cout, x.dst(0));
```

### Output

```text
Series(shape=[2,3], strides=[3,1], dtype=float64, platform=cpu, numel=6, contiguous=true)
data=
⎡  3.5355  4.9497  6.3640 ⎤
⎣ -3.0000 -3.0000 -3.0000 ⎦
```

Example 4. The inverse reconstructs an even-length signal.

```cpp
pluss::series::print(
    std::cout,
    pluss::make_series({1.0, 2.0, 3.0, 4.0}).dst().idst());
```

### Output

```text
Series(shape=[4], strides=[1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[1.0000, 2.0000, 3.0000, 4.0000]
```

Example 5. The same normalization reconstructs odd-length transformed axes.

```cpp
auto x = pluss::make_series({{1.0, 2.0, 3.0}, {4.0, 5.0, 6.0}});
pluss::series::print(std::cout, x.dst().idst());
```

### Output

```text
Series(shape=[2,3], strides=[3,1], dtype=float64, platform=cpu, numel=6, contiguous=true)
data=
⎡ 1.0000 2.0000 3.0000 ⎤
⎣ 4.0000 5.0000 6.0000 ⎦
```

Example 6. Weighted reverse mode exposes the transposed sine basis.

```cpp
auto x = pluss::make_series({1.0, 2.0, 3.0, 4.0});
x.tracks_slope_(true);
x.dst().mul(pluss::make_series({1.0, 0.0, -1.0, 2.0})).sum().reverse();
pluss::series::print(std::cout, x.sensitivity());
```

### Output

```text
Series(shape=[4], strides=[1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[1.4588, -0.6934, 3.3066, -2.5412]
```

Example 7. Negative axes are accepted, but an axis beyond the rank is not.

```cpp
auto x = pluss::make_series({{1.0, 2.0}, {3.0, 4.0}});
std::cout << std::boolalpha << x.dst(-1).equal(x.dst(1)) << '\n';
try {
  (void)x.dst(2);
} catch (const std::exception& e) {
  const std::string message = e.what();
  std::cout << message.substr(0, message.find('\n'));
}
```

### Output

```text
true
dct/idct/dst/idst: dim out of range
```

## See Also

- [`idst()`](idst.md) is the normalization-matched inverse.
- [`dct()`](dct.md) uses a cosine basis and even boundary continuation.
- [`fftshift()`](fftshift.md) changes frequency-bin presentation, not values.
