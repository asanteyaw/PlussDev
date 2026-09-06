<!-- pluss-hand-authored -->

# Series::diff

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series::diff(std::int64_t n) const -> Series
Series::diff(std::int64_t n, std::int64_t dim) const -> Series
```

## What It Does

`diff()` computes the `n`-th discrete difference along an axis.

The first difference replaces each pair of neighbours with their gap:
`diff(x)[i] = x[i + 1] - x[i]`. That shortens the axis by one. Applying it `n`
times gives the `n`-th difference — `n = 2` is the difference of the
differences (a discrete second derivative), and the axis shrinks by `n`.

Without `dim` the difference runs along the last axis.

Common uses: turning a cumulative series back into per-step increments (the
inverse of [`cumsum()`](cumsum.md)), spacing between sorted values, or a rough
curvature signal with `n = 2`.

If `n` is at least the axis length the result is empty.

## Parameters

**n** (`std::int64_t`)

How many times to difference. The axis loses `n` positions.

**dim** (`std::int64_t`, optional)

The axis to difference along. Omitted: the last axis.

## Returns

`Series`
    The input with the chosen axis shortened by `n`, holding the `n`-th
    differences.

## Gradient

`diff()` is differentiable. The first difference is linear, so its backward is
fixed: an interior input `x[i]` feeds `diff[i-1]` with weight `+1` and
`diff[i]` with weight `-1`, while the endpoints feed only one term. Summing a
first difference therefore telescopes — the gradient is `-1` at the front,
`+1` at the back, `0` in between. Higher `n` composes that rule with itself.

## Examples

Example 1. First differences of an evenly spaced row.

```cpp
pluss::series::print(
    std::cout,
    pluss::make_series({10.0, 20.0, 30.0, 40.0, 50.0, 60.0}).diff(1));
```

### Output

```text
Series(shape=[5], strides=[1], dtype=float64, platform=cpu, numel=5, contiguous=true)
data=[10.0000, 10.0000, 10.0000, 10.0000, 10.0000]
```

Example 2. The second difference of a linear series is all zeros — constant
slope, no curvature.

```cpp
pluss::series::print(
    std::cout,
    pluss::make_series({10.0, 20.0, 30.0, 40.0, 50.0, 60.0}).diff(2));
```

### Output

```text
Series(shape=[4], strides=[1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[0.0000, 0.0000, 0.0000, 0.0000]
```

Example 3. On a matrix, the difference runs along the last axis.

```cpp
auto m = pluss::make_series({{1.0, 2.0, 3.0}, {4.0, 5.0, 6.0}});

std::cout << m.diff(1);
```

### Output

```text
pluss::Series(shape=[2,2], strides=[2,1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[[1, 1],
  [1, 1]]
```

Example 4. Summing a first difference telescopes to `x[last] - x[first]`, so
the gradient is `-1` at the front and `+1` at the back.

```cpp
auto x = pluss::make_series({10.0, 20.0, 30.0, 40.0, 50.0, 60.0});
x.tracks_slope_(true);

x.diff(1).sum().reverse();

std::cout << "dx=" << x.sensitivity();
```

### Output

```text
dx=pluss::Series(shape=[6], strides=[1], dtype=float64, platform=cpu, numel=6, contiguous=true)
data=[-1, 0, 0, 0, 0, 1]
```

Example 5. With per-position weights the `+1 / -1` pattern shows through
directly.

```cpp
auto x = pluss::make_series({10.0, 20.0, 30.0, 40.0, 50.0, 60.0});
x.tracks_slope_(true);

auto weights = pluss::make_series({1.0, 10.0, 100.0, 1000.0, 10000.0});
(x.diff(1) * weights).sum().reverse();

std::cout << "dx=" << x.sensitivity();
```

### Output

```text
dx=pluss::Series(shape=[6], strides=[1], dtype=float64, platform=cpu, numel=6, contiguous=true)
data=[-1, -9, -90, -900, -9000, 10000]
```

`x[i]` picks up `-weights[i]` (from `diff[i]`) and `+weights[i-1]` (from
`diff[i-1]`).

## Gotchas And Failure Modes

- `n` is the number of times to difference, and the axis shrinks by `n`. It is
  not a lag or a stride.
- `n >= axis length` gives an empty series, not an error.
- There is no prepend/append option; the result is always `n` shorter.

## Summary

`diff(n [, dim])` is the `n`-th discrete difference along an axis:

- axis shrinks by `n`; default axis is the last
- differentiable; a first difference has a fixed `+1 / -1` telescoping backward
- the inverse operation is [`cumsum()`](cumsum.md)

## See Also

- [`Series::cumsum`](cumsum.md): the prefix sum that `diff(1)` undoes.
- [`Series::unfold`](unfold.md): local windows, another neighbourhood
  operation.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="diagonal_scatter.html">
    <span class="label">Previous</span>
    <span class="title">diagonal_scatter</span>
  </a>
  <a class="next-link" href="digamma.html">
    <span class="label">Next</span>
    <span class="title">digamma</span>
  </a>
</div>
