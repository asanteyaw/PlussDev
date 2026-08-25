<!-- pluss-hand-authored -->

# Series::clamp_min

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series::clamp_min(double min) const -> Series
Series& Series::clamp_min_(double min)
Series::clamp_min(const Series& min) const -> Series
Series& Series::clamp_min_(const Series& min)
```

## What It Does

Applies a lower floor to every element.

Conceptually:

- keep `self[i]` if it is already `>= min`
- otherwise replace it with `min`

This is the one-sided "raise small values upward" member of the clamp family.
The non-underscore overloads return a new series. The underscore overloads
mutate the current series in place and return `Series&`.

## Parameters

**min** (`double` or `const Series&`)

The lower bound.

For `clamp_min(double min)` and `clamp_min_(double min)`:

- one scalar floor is applied everywhere
- every position compares against the same bound

For `clamp_min(const Series& min)` and `clamp_min_(const Series& min)`:

- `min` is broadcast against `self`
- each broadcasted position can therefore have its own effective floor

## Returns

`Series` or `Series&`
    The clamped result, either as a fresh value or the mutated current series
    for the underscore overloads.

## Gradient

The out-of-place overloads are differentiable.

For the scalar form:

- gradient flows to `self` where `self >= min`
- gradient is zero where `self < min`

For the `Series`-bound form:

- gradient flows to `self` where `self >= min`
- gradient flows to the bound series where `self < min`

The boundary is inclusive:

```{note} Inclusive boundary
At `self[i] == min`, the derivative with respect to `self[i]` is `1`, not `0`.
That matches the comparison-family rule used by
[`maximum`](maximum.md)/[`minimum`](minimum.md)/[`clamp_max`](clamp_max.md) and
keeps shared boundaries consistent.
```

The underscore overloads mutate storage directly and should not be treated as
the differentiable surface of the clamp itself. Use the value-returning
`clamp_min()` forms inside computations you intend to backpropagate through.

## Examples

Example 1. A scalar floor raises only the positions below the bound.

```cpp
auto line = pluss::make_series({-2.0, -0.5, 0.0, 1.0, 3.0}, {5});

std::cout << line.clamp_min(0.0);
```

### Output

```text
pluss::Series(shape=[5], strides=[1], dtype=float64, platform=cpu, numel=5, contiguous=true)
data=[0, 0, 0, 1, 3]
```

This is the familiar ReLU-style floor: everything below zero is lifted to zero,
while values already above zero pass through untouched.

Example 2. The boundary at exactly the floor is inclusive in the backward rule.

```cpp
auto clamp_min_grad = line.clone();
clamp_min_grad.tracks_slope_(true);
auto clamp_min_y = clamp_min_grad.clamp_min(0.0);
clamp_min_y.sum().reverse();
std::cout << "y=" << clamp_min_y << "\n";
std::cout << "dx=" << clamp_min_grad.sensitivity() << "\n";
```

### Output

```text
y=pluss::Series(shape=[5], strides=[1], dtype=float64, platform=cpu, numel=5, contiguous=true)
data=[0, 0, 0, 1, 3]
dx=pluss::Series(shape=[5], strides=[1], dtype=float64, platform=cpu, numel=5, contiguous=true)
data=[0, 0, 1, 1, 1]
```

The third position started exactly at `0.0`, and it still receives derivative
`1`.

Example 3. The `Series` overload broadcasts a row-shaped lower bound across the
leading dimension.

```cpp
auto lows = pluss::make_series({2, 4, 3}, {3});
auto grid = pluss::make_series({1, 5, 3, 0, 7, 2},
    {2, 3});

std::cout << grid.clamp_min(lows);
```

### Output

```text
pluss::Series(shape=[2,3], strides=[3,1], dtype=float64, platform=cpu, numel=6, contiguous=true)
data=[[2, 5, 3],
  [2, 7, 3]]
```

Each row of `grid` is compared against the same `[2, 4, 3]` floor pattern.

Example 4. The scalar underscore overload mutates the current handle in place.

```cpp
auto scalar_inplace = line.clone();
scalar_inplace.clamp_min_(1.5);
std::cout << scalar_inplace;
```

### Output

```text
pluss::Series(shape=[5], strides=[1], dtype=float64, platform=cpu, numel=5, contiguous=true)
data=[1.5, 1.5, 1.5, 1.5, 3]
```

No new `Series` is returned here; the original handle now owns the clamped
values.

Example 5. The `Series` underscore overload mutates through the same broadcasted
bound pattern.

```cpp
auto series_inplace = grid.clone();
series_inplace.clamp_min_(lows);
std::cout << series_inplace;
```

### Output

```text
pluss::Series(shape=[2,3], strides=[3,1], dtype=float64, platform=cpu, numel=6, contiguous=true)
data=[[2, 5, 3],
  [2, 7, 3]]
```

## Notes

- `clamp_min(x, 0.0)` is the same numerical idea as a ReLU floor.
- The `Series`-bound overload is semantically close to [`maximum`](maximum.md):
  `x.clamp_min(lo)` and `x.maximum(lo)` express the same comparison, but
  `clamp_min` makes the "this operand is the floor" intent explicit.
- Use the underscore forms when you specifically want mutation, not when you
  need the clamp itself to be the differentiable node in the graph.

## See Also

- [`Series::clamp_max`](clamp_max.md): the mirrored upper ceiling.
- [`Series::maximum`](maximum.md): equivalent comparison phrased without bound
  terminology.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="clamp_max.html">
    <span class="label">Previous</span>
    <span class="title">clamp_max</span>
  </a>
  <a class="next-link" href="copy_.html">
    <span class="label">Next</span>
    <span class="title">copy_</span>
  </a>
</div>
