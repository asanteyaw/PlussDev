<!-- pluss-hand-authored -->

# Series::expm1

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series::expm1() const -> Series
```

## What It Does

`expm1()` applies `e^x - 1` elementwise.

Mathematically, that is the same quantity you would get from `exp(x) - 1`, but
it is exposed as its own method because it is a common transform in its own
right, especially around values close to zero.

## Parameters

None.

## Returns

`Series`
    A new `Series` with the same shape as `self`, containing `e^x - 1` lane by
    lane.

## Gradient

If `y = e^x - 1`, then:

- `dy/dx = e^x`
- equivalently, `dy/dx = y + 1`

So when the objective is `L = sum(y)`, each lane contributes its own
`e^{x_i}` as the local backward multiplier.

## Examples

Example 1. The transform preserves shape and shifts the ordinary exponential
down by one.

```cpp
std::cout << pluss::series::print(pluss::make_series({-1.0, 0.0, 1.0}).expm1());
```

### Output

```text
Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[-0.6321, 0.0000, 1.7183]
```

Example 2. Reverse-mode multiplies by `e^x`, not by `e^x - 1`.

```cpp
auto x = pluss::make_series({-1.0, 0.0, 1.0});
x.tracks_slope_(true);

auto y = x.expm1();
y.sum().reverse();

std::cout << "out=" << y << "\n";
std::cout << "dL_dx=" << x.sensitivity() << "\n";
```

### Output

```text
out=pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[-0.63212056, 0, 1.7182818]
dL_dx=pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[0.36787944, 1, 2.7182818]
```

## Gotchas And Failure Modes

- The returned values are `e^x - 1`, but the derivative is `e^x`.
- Large positive inputs still grow quickly because this is still exponential.
- Shape is preserved; there is no reduction, broadcasting, or axis toggle.

## Summary

Use `expm1()` when the exact transform is "natural exponential, then subtract
one" and you want that intent stated directly at the call site.

## See Also

- [`Series::exp`](exp.md): the unshifted natural exponential.
- [`Series::log1p`](log1p.md): the common logarithmic counterpart.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="exponential_.html">
    <span class="label">Previous</span>
    <span class="title">exponential_</span>
  </a>
  <a class="next-link" href="extract.html">
    <span class="label">Next</span>
    <span class="title">extract</span>
  </a>
</div>
