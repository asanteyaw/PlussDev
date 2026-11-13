<!-- pluss-hand-authored -->

# Series::sinh

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series Series::sinh() const
Series& Series::sinh_()
```

## What It Does

Applies the hyperbolic sine elementwise.

`sinh()` is the odd hyperbolic companion to [`Series::cosh`](cosh.md). It is
defined for all real inputs and preserves the sign of the input:

- negative inputs stay negative
- zero stays zero
- positive inputs stay positive

Unlike ordinary sine, `sinh()` does not oscillate. Its magnitude grows as the
input moves away from zero.

## Parameters

None.

This is a zero-argument transform on `self`.

## Returns

`Series`
    A new `Series` with the same shape as `self`, where each element has been
    replaced by its hyperbolic sine.

## Gradient

Mathematically, `sinh(x)` has derivative `cosh(x)`, so the backward multiplier
is always at least `1` for real inputs.

On the current verified public surface used here, the reverse-mode example
returns zero sensitivities. So the value transform is verified, but this method
path should not currently be treated as backward-ready.

## In-Place Form

`sinh_()` is the in-place sibling of [`sinh()`](sinh.md): it computes the same
result and writes it back into `self` directly — mutating it (and any other
handle sharing the same storage) instead of allocating a new `Series` — then
returns a reference to `self`.

```cpp
auto x = pluss::make_series({0.0, 1.0});

auto& ref = x.sinh_();
std::cout << "x=" << x << "\n";
std::cout << std::boolalpha << "same object: " << (&ref == &x);
```

### Output

```text
x=pluss::Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[0, 1.1752012]
same object: true
```

## Examples

Example 1. Small signed inputs show the odd symmetry clearly.

```cpp
std::cout << pluss::series::print(
    pluss::make_series({-1.0, 0.0, 1.0}).sinh());
```

### Output

```text
Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[-1.1752, 0.0000, 1.1752]
```

Example 2. Shape is preserved on matrices too.

```cpp
pluss::series::print(
    std::cout,
    pluss::make_series({
        {-1.0, -0.5},
        {0.5, 1.0},
    }).sinh());
```

### Output

```text
Series(shape=[2,2], strides=[2,1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=
⎡ -1.1752 -0.5211 ⎤
⎣  0.5211  1.1752 ⎦
```

Example 3. The current reverse-mode path yields zero sensitivities on this
surface.

```cpp
auto x = pluss::make_series({-1.0, 0.0, 1.0});
x.tracks_slope_(true);

auto out = x.sinh();
out.sum().reverse();

std::cout << "out=" << out << "\n";
std::cout << "dx=" << x.sensitivity();
```

### Output

```text
out=pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[-1.1752012, 0, 1.1752012]
dx=pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[0, 0, 0]
```

Example 4. `sinh()` is odd, so negating the input negates the output.

```cpp
std::cout << pluss::make_series({-2.0, -1.0, 0.0, 1.0, 2.0}).sinh();
```

### Output

```text
pluss::Series(shape=[5], strides=[1], dtype=float64, platform=cpu, numel=5, contiguous=true)
data=[-3.6268604, -1.1752012, 0, 1.1752012, 3.6268604]
```

## Gotchas And Failure Modes

- `sinh()` grows quickly for larger magnitudes; it is not bounded.
- Do not confuse its odd symmetry with ordinary sine's periodic behavior.
- The current verified backward path returns zero sensitivities here.

## Summary

Use `sinh()` when you need the odd hyperbolic transform: sign-preserving,
non-periodic, and rapidly growing away from zero. The forward values are
verified; the current backward route on this public surface is not.

## See Also

- [`Series::cosh`](cosh.md): even hyperbolic companion.
- [`Series::tanh`](tanh.md): bounded hyperbolic ratio transform.
- [`Series::sin`](sin.md): bounded periodic sine.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="sinc.html">
    <span class="label">Previous</span>
    <span class="title">sinc</span>
  </a>
  <a class="next-link" href="size.html">
    <span class="label">Next</span>
    <span class="title">size</span>
  </a>
</div>
