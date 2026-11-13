<!-- pluss-hand-authored -->

# Series::ceil

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series Series::ceil() const
Series& Series::ceil_()
```

## What It Does

`ceil()` rounds each element upward to the least integer value that is greater
than or equal to that element.

This is an elementwise transform:

- shape is preserved
- dtype stays numeric
- each lane is processed independently

For negative numbers, "upward" still means toward positive infinity. That is
why `-1.2` becomes `-1` rather than `-2`.

## Returns

`Series`
    A new `Series` whose elements have been rounded upward.

## Gradient

Away from integer jump points, `ceil(x)` is locally constant, so its derivative
is `0`.

That is the right mental model for reverse-mode too: once a scalar objective
passes through `ceil()`, small perturbations to the input do not change the
rounded value until an integer boundary is crossed.

## In-Place Form

`ceil_()` is the in-place sibling of [`ceil()`](ceil.md): it computes the same
result and writes it back into `self` directly — mutating it (and any other
handle sharing the same storage) instead of allocating a new `Series` — then
returns a reference to `self`.

```cpp
auto x = pluss::make_series({1.2, -1.2, 2.0});

auto& ref = x.ceil_();
std::cout << "x=" << x << "\n";
std::cout << std::boolalpha << "same object: " << (&ref == &x);
```

### Output

```text
x=pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[2, -1, 2]
same object: true
```

## Examples

Example 1. Negative and positive values both round toward positive infinity.

```cpp
std::cout << pluss::make_series({-1.2, -0.1, 0.0, 1.2, 2.1}).ceil();
```

### Output

```text
pluss::Series(shape=[5], strides=[1], dtype=float64, platform=cpu, numel=5, contiguous=true)
data=[-1, -0, 0, 2, 3]
```

The `-0` display is still zero numerically. It is just the signed floating
representation surfacing in the printout.

Example 2. Reverse-mode sensitivity is zero through the rounded lanes in this
ordinary case.

```cpp
auto x = pluss::make_series({-1.2, 0.0, 3.8});
x.tracks_slope_(true);

auto y = x.ceil();
y.sum().reverse();

std::cout << "out=" << y << "\n";
std::cout << "dL_dx=" << x.sensitivity();
```

### Output

```text
out=pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[-1, 0, 4]
dL_dx=pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[0, 0, 0]
```

## Gotchas And Failure Modes

- `ceil()` is not rounding-to-nearest; it always moves upward.
- Negative values can move toward zero rather than away from it.
- The transform is discontinuous at integers.

## Summary

Use `ceil()` when every value should be promoted to the next admissible integer
ceiling, with exact integers left unchanged.

## See Also

- [`Series::floor`](floor.md): rounds downward instead.
- [`Series::round`](round.md): rounds to the nearest integer instead of always
  upward.
- [`Series::trunc`](trunc.md): discards the fractional part toward zero.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="cauchy_.html">
    <span class="label">Previous</span>
    <span class="title">cauchy_</span>
  </a>
  <a class="next-link" href="clamp_max.html">
    <span class="label">Next</span>
    <span class="title">clamp_max</span>
  </a>
</div>
