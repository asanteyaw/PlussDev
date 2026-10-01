<!-- pluss-hand-authored -->

# Series::sqrt

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series::sqrt() const -> Series
Series& Series::sqrt_()
```

## What It Does

Applies the square-root transform elementwise.

For non-negative inputs, `sqrt()` returns the principal square root of each
element. The shape is preserved and no broadcasting is involved.

Like [`Series::log`](log.md), `sqrt()` is strongly domain-sensitive:

- non-negative inputs produce ordinary real outputs
- negative inputs produce `nan`

So this method presents two ideas at once: a familiar numeric transform and the
importance of validating the value domain before applying it blindly.

## Parameters

None.

## Returns

`Series`
    A new `Series` with the same shape as `self`, where each element has been
    replaced by its square root.

## Gradient

For positive inputs, `sqrt()` has a decreasing derivative:

- the forward value is `sqrt(x)`
- the backward multiplier is `1 / (2 * sqrt(x))`

That means sensitivities are larger near zero and smaller for larger inputs.

## In-Place Form

`sqrt_()` is the in-place sibling of [`sqrt()`](sqrt.md): it computes the same
result and writes it back into `self` directly — mutating it (and any other
handle sharing the same storage) instead of allocating a new `Series` — then
returns a reference to `self`.

```cpp
auto x = pluss::make_series({1.0, 4.0, 9.0});

auto& ref = x.sqrt_();
std::cout << "x=" << x << "\n";
std::cout << std::boolalpha << "same object: " << (&ref == &x);
```

### Output

```text
x=pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[1, 2, 3]
same object: true
```

## Examples

Example 1. Perfect squares produce exact whole-number roots.

```cpp
pluss::series::print(
    std::cout,
    pluss::make_series({0.0, 1.0, 4.0, 9.0}).sqrt());
```

### Output

```text
Series(shape=[4], strides=[1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[0.0000, 1.0000, 2.0000, 3.0000]
```

Example 2. The transform remains shape-preserving on matrices.

```cpp
std::cout << pluss::make_series({
    {1.0, 4.0},
    {9.0, 16.0},
}).sqrt();
```

### Output

```text
pluss::Series(shape=[2,2], strides=[2,1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[[1, 2],
  [3, 4]]
```

Example 3. Reverse-mode sensitivity shrinks as the input grows.

```cpp
auto x = pluss::make_series({1.0, 4.0, 9.0});
x.tracks_slope_(true);

auto out = x.sqrt();
out.sum().reverse();

std::cout << "out=" << out << "\n";
std::cout << "dx=" << x.sensitivity();
```

### Output

```text
out=pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[1, 2, 3]
dx=pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[0.5, 0.25, 0.16666667]
```

Example 4. Domain-edge behavior surfaces directly in the result.

```cpp
std::cout << pluss::make_series({4.0, 0.0, -1.0}).sqrt();
```

### Output

```text
pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[2, 0, nan]
```

## Gotchas And Failure Modes

- Negative inputs do not raise an exception in this verified path; they produce
  `nan`.
- Zero is in-domain for the forward value, but it is still a numerically
  delicate point when you think about derivative behavior.
- Because the derivative decreases with larger inputs, `sqrt()` can compress
  both values and sensitivities.

## Summary

Use `sqrt()` as a shape-preserving unary transform with a real-valued domain
of non-negative inputs. It is a good method for showing how mathematically
familiar functions still need explicit domain awareness in practical code.

## See Also

- [`Series::log`](log.md): another domain-sensitive unary transform.
- [`Series::exp`](exp.md): unary transform with the opposite growth profile.
- [`Series::abs`](abs.md): often used upstream when you need non-negative
  magnitudes before further transforms.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="sparse_resize.html">
    <span class="label">Previous</span>
    <span class="title">sparse_resize</span>
  </a>
  <a class="next-link" href="square.html">
    <span class="label">Next</span>
    <span class="title">square</span>
  </a>
</div>
