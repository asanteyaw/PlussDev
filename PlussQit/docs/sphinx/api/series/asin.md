<!-- pluss-hand-authored -->

# Series::asin

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series::asin() const -> Series
```

## What It Does

Applies the inverse sine elementwise.

Each input value is interpreted as a sine value and mapped back to its
principal angle in radians. The principal range is `[-pi / 2, pi / 2]`, so
valid real outputs stay inside that interval.

`asin()` is often used after a normalization or ratio step where the result is
already known to live in `[-1, 1]`. When that assumption is wrong, the output
shows it immediately because out-of-domain values become `nan`.

## Parameters

None.

This is a zero-argument transform on `self`.

## Returns

`Series`
    A new `Series` with the same shape as `self`, where each element has been
    replaced by its principal inverse sine in radians.

## Gradient

Mathematically, `asin(x)` has derivative `1 / sqrt(1 - x^2)` for interior
points `|x| < 1`.

That means:

- the derivative is always non-negative on the real interior
- it grows larger as the input approaches `-1` or `1`

On the current verified public surface used here, the reverse-mode example
returns zero sensitivities. So the forward values are correct, but you should
not yet rely on this method path for backward propagation.

## Examples

Example 1. Familiar domain anchors map back to familiar angles.

```cpp
std::cout << pluss::series::print(
    pluss::make_series({-1.0, -0.5, 0.0, 0.5, 1.0}).asin());
```

### Output

```text
Series(shape=[5], strides=[1], dtype=float64, platform=cpu, numel=5, contiguous=true)
data=[-1.5708, -0.5236, 0.0000, 0.5236, 1.5708]
```

Example 2. Shape is preserved for higher-rank inputs too.

```cpp
pluss::series::print(
    std::cout,
    pluss::make_series({
        {-1.0, 0.0},
        {0.5, 1.0},
    }).asin());
```

### Output

```text
Series(shape=[2,2], strides=[2,1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=
⎡ -1.5708 0.0000 ⎤
⎣  0.5236 1.5708 ⎦
```

Example 3. The current reverse-mode path yields zero sensitivities on this
surface, which should be treated as an implementation caveat rather than a
mathematical property.

```cpp
auto x = pluss::make_series({-0.5, 0.0, 0.5});
x.tracks_slope_(true);

auto out = x.asin();
out.sum().reverse();

std::cout << "out=" << out << "\n";
std::cout << "dx=" << x.sensitivity();
```

### Output

```text
out=pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[-0.52359878, 0, 0.52359878]
dx=pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[0, 0, 0]
```

Example 4. Domain violations are visible in the result.

```cpp
std::cout << pluss::make_series({-1.25, -1.0, 0.0, 1.0, 1.25}).asin();
```

### Output

```text
pluss::Series(shape=[5], strides=[1], dtype=float64, platform=cpu, numel=5, contiguous=true)
data=[nan, -1.5707963, 0, 1.5707963, nan]
```

## Gotchas And Failure Modes

- `asin()` is only real-valued on `[-1, 1]`.
- Endpoints are valid outputs, but they are numerically sharp points for the
  mathematical derivative.
- The current verified backward path returns zero sensitivities here.

## Summary

Use `asin()` when the data already represents sine-like values in `[-1, 1]`
and you need principal angles back. The two practical checks are domain safety
and whether your workflow needs verified gradient propagation.

## See Also

- [`Series::acos`](acos.md): inverse cosine on the same value domain.
- [`Series::atan`](atan.md): inverse tangent with unrestricted real input.
- [`Series::sin`](sin.md): forward sine transform.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="argmin.html">
    <span class="label">Previous</span>
    <span class="title">argmin</span>
  </a>
  <a class="next-link" href="atan.html">
    <span class="label">Next</span>
    <span class="title">atan</span>
  </a>
</div>
