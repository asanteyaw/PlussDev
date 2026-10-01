<!-- pluss-hand-authored -->

# Series::unwrap

[Back to Series API Reference](../../series_api_reference)

## Signatures

```cpp
Series Series::unwrap() const
Series Series::unwrap(double discont) const
Series Series::unwrap(double discont, std::int64_t dim) const
```

## Recovering A Continuous Phase Path

Wrapped phase is commonly reported in a principal interval even when the
underlying angle evolves continuously. That representation creates apparent
jumps near a full turn. `unwrap()` removes those artificial jumps by adding
integer multiples of `2*pi` along one axis while preserving genuine local
movement.

For adjacent values with difference `delta`, the method finds the congruent
difference in `[-pi, pi]` and accumulates the required multiple of `2*pi` into
all subsequent lanes. Differences smaller than `discont` are left alone.
A positive boundary difference of exactly `pi` remains positive rather than
being changed to `-pi`.

The overloads mean:

| Call | Discontinuity threshold | Axis |
|---|---|---|
| `unwrap()` | `pi` | Final axis |
| `unwrap(discont)` | Caller supplied | Final axis |
| `unwrap(discont, dim)` | Caller supplied | Caller selected; negative axes are accepted |

Choosing `discont < pi` does not force corrections for smaller jumps: a jump
must still have a shorter complement across the `2*pi` boundary. Choosing a
value greater than `pi` suppresses corrections until that larger threshold is
crossed. `discont` must be finite and non-negative.

## Shape, Dtype, Empty Inputs, And Reverse Mode

The receiver must have at least one axis. Floating and integral inputs are
accepted; integral values are promoted to `float64` because the correction
contains `pi`. Complex inputs are rejected. Shape and platform are preserved.
An empty selected axis returns an empty Series of the resolved floating dtype.

The chosen wrap count is discrete. Away from wrap-decision boundaries, adding
a constant multiple of `2*pi` has derivative one, so reverse mode passes each
output sensitivity to the corresponding input lane. At a discontinuity
decision boundary the mathematical operation is not differentiable; the
recorded piecewise branch is used.

## Examples

### 1. Unwrap the final axis with the default threshold

```cpp
const auto phase =
    pluss::make_series({0.0, 1.0, 2.0, 4.0, -2.0, 0.0});
pluss::series::print(std::cout, phase.unwrap());
```

#### Output

```text
Series(shape=[6], strides=[1], dtype=float64, platform=cpu, numel=6, contiguous=true)
data=[0.0000, 1.0000, 2.0000, 4.0000, 4.2832, 6.2832]
```

The transition from `4` to `-2` is interpreted as a short positive movement
across the wrap boundary, so one full turn is added from that point onward.

### 2. Show why a threshold below `pi` has no extra effect

```cpp
pluss::series::print(std::cout, phase.unwrap(0.5));
```

#### Output

```text
Series(shape=[6], strides=[1], dtype=float64, platform=cpu, numel=6, contiguous=true)
data=[0.0000, 1.0000, 2.0000, 4.0000, 4.2832, 6.2832]
```

Although several ordinary differences exceed `0.5`, only differences with a
shorter modulo-`2*pi` complement are corrected.

### 3. Preserve a large jump deliberately

```cpp
pluss::series::print(std::cout, phase.unwrap(7.0));
```

#### Output

```text
Series(shape=[6], strides=[1], dtype=float64, platform=cpu, numel=6, contiguous=true)
data=[0.0000, 1.0000, 2.0000, 4.0000, -2.0000, 0.0000]
```

The apparent six-radian jump is below the requested discontinuity threshold,
so it is treated as intentional.

### 4. Select a non-final axis

```cpp
const auto matrix = pluss::make_series({{0.0, 4.0}, {0.0, -4.0}});
pluss::series::print(
    std::cout, matrix.unwrap(3.141592653589793, 0));
```

#### Output

```text
Series(shape=[2,2], strides=[2,1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=
⎡ 0.0000 4.0000 ⎤
⎣ 0.0000 2.2832 ⎦
```

Each column is an independent path because axis zero is selected. The first
column needs no correction; the second crosses its boundary.

### 5. Correct jumps spanning more than one full turn

```cpp
pluss::series::print(
    std::cout, pluss::make_series({0.0, 13.0, -13.0}).unwrap());
```

#### Output

```text
Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[0.0000, 0.4336, -0.4336]
```

The correction is not limited to one turn. It chooses the required integer
multiple of `2*pi` for each adjacent difference.

### 6. Promote integral phase samples

```cpp
pluss::series::print(
    std::cout, pluss::make_series({0, 4, -2}).unwrap());
```

#### Output

```text
Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[0.0000, -2.2832, -2.0000]
```

Promotion prevents the irrational turn correction from being truncated back
to integers.

### 7. Differentiate through the selected branch

```cpp
auto tracked = phase.clone();
tracked.tracks_slope_(true);
tracked.unwrap().sum().reverse();
pluss::series::print(std::cout, tracked.sensitivity());
```

#### Output

```text
Series(shape=[6], strides=[1], dtype=float64, platform=cpu, numel=6, contiguous=true)
data=[1.0000, 1.0000, 1.0000, 1.0000, 1.0000, 1.0000]
```

### 8. Preserve an empty selected axis

```cpp
const auto empty =
    pluss::series::empty({0}, pluss::ScalarType::Float64);
pluss::series::print(std::cout, empty.unwrap());
```

#### Output

```text
Series(shape=[0], strides=[1], dtype=float64, platform=cpu, numel=0, contiguous=true)
[]
```

### 9. Reject a scalar receiver

```cpp
try {
  (void)pluss::make_series(1.0).unwrap();
} catch (const std::exception& error) {
  std::cout << error.what() << '\n';
}
```

#### Output

```text
unwrap: dim out of range
```

A scalar has no axis along which adjacent phase samples can be compared.

## Performance

The operation performs one sequential accumulation along each selected-axis
path and parallelizes naturally across the remaining coordinates. Work and
new storage are linear in `numel()`. Choose the axis that already represents
the physical sample order; sorting or permuting phase samples after unwrapping
changes the problem rather than merely its layout.

## See Also

- [`angle()`](angle.md) obtains wrapped phase from complex values.
- [`diff()`](diff.md) computes adjacent differences without cumulative turn correction.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="unsqueeze.html">
    <span class="label">Previous</span>
    <span class="title">unsqueeze</span>
  </a>
  <a class="next-link" href="var.html">
    <span class="label">Next</span>
    <span class="title">var</span>
  </a>
</div>
