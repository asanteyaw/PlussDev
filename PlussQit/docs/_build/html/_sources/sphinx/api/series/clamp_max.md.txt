<!-- pluss-hand-authored -->

# Series::clamp_max

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series::clamp_max(double max) const -> Series
Series& Series::clamp_max_(double max)
Series::clamp_max(const Series& max) const -> Series
Series& Series::clamp_max_(const Series& max)
```

## What It Does

Applies an upper ceiling to every element.

Conceptually:

- keep `self[i]` if it is already `<= max`
- otherwise replace it with `max`

This is the one-sided "pull large values downward" member of the clamp family.
The non-underscore overloads return a new series. The underscore overloads
mutate the current series in place and return `Series&`.

## Parameters

**max** (`double` or `const Series&`)

The upper bound.

For `clamp_max(double max)` and `clamp_max_(double max)`:

- one scalar ceiling is applied everywhere

For `clamp_max(const Series& max)` and `clamp_max_(const Series& max)`:

- `max` is broadcast against `self`
- different broadcasted positions can therefore have different ceilings

## Returns

`Series` or `Series&`
    The clamped result, either as a fresh value or as the mutated current
    series for the underscore overloads.

## Gradient

The out-of-place overloads are differentiable.

For the scalar form:

- gradient flows to `self` where `self <= max`
- gradient is zero where `self > max`

For the `Series`-bound form:

- gradient flows to `self` where `self <= max`
- gradient flows to the bound series where `self > max`

The boundary is inclusive:

```{note} Inclusive boundary
At `self[i] == max`, the derivative with respect to `self[i]` is `1`. This is
not an arbitrary choice; it keeps the comparison and clamp family consistent at
shared boundaries.
```

The underscore overloads are storage mutations. Use the out-of-place
`clamp_max()` forms when you need the clamp itself to sit on the differentiable
path.

## Examples

Example 1. A scalar ceiling pulls only the values above the bound downward.

```cpp
auto line = pluss::make_series({-2.0, -0.5, 0.0, 1.0, 3.0}, {5});

std::cout << line.clamp_max(0.0);
```

### Output

```text
pluss::Series(shape=[5], strides=[1], dtype=float64, platform=cpu, numel=5, contiguous=true)
data=[-2, -0.5, 0, 0, 0]
```

Everything already below zero passes through. Only the positive tail is capped.

Example 2. The boundary at exactly the ceiling still attributes derivative to
`self`.

```cpp
auto clamp_max_grad = line.clone();
clamp_max_grad.tracks_slope_(true);
auto clamp_max_y = clamp_max_grad.clamp_max(0.0);
clamp_max_y.sum().reverse();
std::cout << "y=" << clamp_max_y << "\n";
std::cout << "dx=" << clamp_max_grad.sensitivity() << "\n";
```

### Output

```text
y=pluss::Series(shape=[5], strides=[1], dtype=float64, platform=cpu, numel=5, contiguous=true)
data=[-2, -0.5, 0, 0, 0]
dx=pluss::Series(shape=[5], strides=[1], dtype=float64, platform=cpu, numel=5, contiguous=true)
data=[1, 1, 1, 0, 0]
```

The third position started exactly at `0.0`, and it keeps derivative `1`.

Example 3. The `Series` overload broadcasts a row-shaped ceiling across the
leading dimension.

```cpp
auto highs = pluss::make_series({2, 4, 3}, {3});
auto grid = pluss::make_series({1, 5, 3, 0, 7, 2},
    {2, 3});

std::cout << grid.clamp_max(highs);
```

### Output

```text
pluss::Series(shape=[2,3], strides=[3,1], dtype=float64, platform=cpu, numel=6, contiguous=true)
data=[[1, 4, 3],
  [0, 4, 2]]
```

Each row is compared against the same `[2, 4, 3]` ceiling pattern.

Example 4. The scalar underscore overload mutates the current handle in place.

```cpp
auto scalar_inplace = line.clone();
scalar_inplace.clamp_max_(1.5);
std::cout << scalar_inplace;
```

### Output

```text
pluss::Series(shape=[5], strides=[1], dtype=float64, platform=cpu, numel=5, contiguous=true)
data=[-2, -0.5, 0, 1, 1.5]
```

Example 5. The `Series` underscore overload mutates through the same broadcasted
ceiling pattern.

```cpp
auto series_inplace = grid.clone();
series_inplace.clamp_max_(highs);
std::cout << series_inplace;
```

### Output

```text
pluss::Series(shape=[2,3], strides=[3,1], dtype=float64, platform=cpu, numel=6, contiguous=true)
data=[[1, 4, 3],
  [0, 4, 2]]
```

## Notes

- `clamp_max()` is the ceiling-side counterpart to [`clamp_min()`](clamp_min.md).
- The `Series`-bound overload is semantically close to [`minimum`](minimum.md):
  `x.clamp_max(hi)` and `x.minimum(hi)` express the same comparison, but
  `clamp_max` makes the ceiling role explicit.
- Use the underscore forms when you want storage mutation as the outcome, not
  when you want the clamp itself to be the differentiable operation.

## See Also

- [`Series::clamp_min`](clamp_min.md): the mirrored lower floor.
- [`Series::minimum`](minimum.md): equivalent comparison phrased without bound
  terminology.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="put_.html">
    <span class="label">Previous</span>
    <span class="title">put_</span>
  </a>
  <a class="next-link" href="clamp_min.html">
    <span class="label">Next</span>
    <span class="title">clamp_min</span>
  </a>
</div>
